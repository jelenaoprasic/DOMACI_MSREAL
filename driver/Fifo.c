#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/semaphore.h>

#define BUFF_SIZE 250

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

DECLARE_WAIT_QUEUE_HEAD(readQueue);
DECLARE_WAIT_QUEUE_HEAD(writeQueue);


struct semaphore sem;

int x=0;
unsigned int fifo[16];
int pos;
int num=0;
int endRead=0;
int stopRead=1;
int tmp=0;


int fifo_open(struct inode *pinode, struct file *pfile);
int fifo_close(struct inode *pinode, struct file *pfile);
ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
  .owner = THIS_MODULE,
  .open = fifo_open,
  .read = fifo_read,
  .write = fifo_write,
  .release = fifo_close,
  
};


int fifo_open(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully opened fifo\n");
    return 0;
}

int fifo_close(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully closed fifo\n");
    return 0;
}



ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
  int ret;

  long int len = 0;
  int i,j=0;
  char buff1[80];
  char buff[BUFF_SIZE];
  buff1[0] = '\0';
 
 if(wait_event_interruptible(readQueue,(tmp>0)))                                                                     
  {
    return -ERESTARTSYS;
  }


  if (num==0) {
    num=1;
  }

  stopRead = !stopRead;                                                                             
  
  if(stopRead==1)
  {
    return 0;
  } 

  
  if(tmp > (num-1))
  {
    tmp=tmp-num;
    
    for(j=0;j<num;j++)
    {
      
      len = snprintf(buff1+strlen(buff1), 80, "%#04x ", fifo[0]);                              
        
        for(i=0;i<15;i++)                                                             //fifo[] pomeramo u desno u fifo maniru i upisujemo vrednosti
        {                                                                                
          fifo[i]=fifo[i+1];
        }
      
      fifo[15] = -1;
    }

    ret = copy_to_user(buffer, buff1, len*num);
    if (ret)
      return -EFAULT;
    
    printk(KERN_INFO "Read from fifo!\n");
  }

  else
  {
    printk(KERN_WARNING "There are %d elements in FIFO, the value of n must be n<=%d\n",tmp,tmp);
  }
    
    //up(&sem);                                                                                              
    wake_up_interruptible(&writeQueue);                                                                    
    
    return len*num;
  
  }

/*
  else{

    if(endRead)
    {
      endRead = 0;
      return 0;
    }

    if(tmp > 0)
    {
      len = scnprintf(buff, BUFF_SIZE, "%#04x \n", fifo[0]);                            //korisnik potrazuje samo prvi upisan podatak
      ret = copy_to_user(buffer, buff, len);

      if(ret)
        return -EFAULT;
      
      printk(KERN_INFO "Succesfully read\n");
      endRead = 1;
    }

    
    else
    {
        printk(KERN_WARNING "Fifo is empty\n"); 
    }
    
    tmp=tmp-1;
    num=1;

    wake_up_interruptible(&writeQueue);   
    return len;
  }

*/

//}

 


ssize_t fifo_write(struct file *pfile, const char *buffer, size_t length, loff_t *offset)  
{
  int i=0;                     
  int ret;
  char buff[BUFF_SIZE];
  char* ptr;
  int j;
    
  ret=copy_from_user(buff, buffer, length);
  if(ret)
    return -EFAULT;
  
  buff[length-1]='\0';


  if(!strncmp(buff,"num=",4))                                                       //provera da li korisnik unosi nove vrednosti ili potrazuje num=n komandu
  {
    ret=sscanf(buff, "num=%d", &num);                                               //promenljiva num cuva podatak koliko clanova FIFO korisnik zeli procitati
    printk(KERN_INFO "num=%d", num);
  }

  else
  {
      ptr=strchr(buff,';');

      if(ptr==NULL)                                                                 //strchr ne pronalazi ; sto znaci da je poslata jedna vrednost i upisuje je u fifo
      {
        ret=kstrtouint(buff,0, &fifo[0]); 
        tmp=1;
        printk(KERN_INFO "Succesfully written value %#04x in  FIFO", fifo[0]);
      }
      
      else
      {
        
        char* const delim= ";";
        char *token, *cur=buff;
        int token1;
        i=tmp;

        while(token =strsep(&cur, delim))                                          //strsep funkcijom izdvajamo vrednosti odvojene delimiterom ;
        {

          ret=kstrtouint(token, 0, &fifo[i]);                                      //konvertujemo izdvojen token u unsigned int i smestamo na i-tu poziciju fifo bafera 
          i++; 
          
          //if(fifo[i] > 255)                                                        //proveravamo da uneta vrednost ne premasuje dozvoljen opseg(0xFF)
          //{
            //printk(KERN_WARNING "Input can not be greater than 0xff");         
            //return length;
          //}

          
         while(i > 15)
        {
          tmp=i-1;
          
          if(wait_event_interruptible(writeQueue,(tmp<16)))                          //Fifo je pun
              return -ERESTARTSYS;

          /*for(j=0; j<tmp+num; j++)
            {
              
              fifo[j]=fifo[j+num];
            } 

         */// i=i-num;
        }
      
      } 
        tmp=i;
      
      for(j=0; j<tmp; j++)
        {
          printk("Value %#04x written in FIFO", fifo[j]);
        }        

        

    }
                        
  }
  wake_up_interruptible(&readQueue);                                                                           
  return length;
}

static int __init fifo_init(void)
{
   int ret = 0;

   sema_init(&sem,1);

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "fifo_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

  my_cdev = cdev_alloc(); 
  my_cdev->ops = &my_fops;
  my_cdev->owner = THIS_MODULE;
  ret = cdev_add(my_cdev, my_dev_id, 1);
  if (ret)
  {
      printk(KERN_ERR "failed to add cdev\n");
    goto fail_2;
  }
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit fifo_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(fifo_init);
module_exit(fifo_exit);


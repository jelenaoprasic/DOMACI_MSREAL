
// asynctest.c: use async notification to read stdin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
int option=0;
int gotsignal=0;
int datacnt=0;
int end = 0;
int broj;
char num[30];
ssize_t bytes_written,bytes_read;
ssize_t nbytes;
int gotdata;
int endRead=0;
int broj=0;


void sighandler(int signo)
{
    if (signo==SIGIO)
    {
	gotsignal=1;
	datacnt++;
    }
    return;
}

char buffer[4096];
char buffer1[4096];
int main(int argc, char **argv)
{
    int count;
    struct sigaction action;
    int fd=open("/dev/fifo",O_RDWR|O_NONBLOCK);
    

    if (!fd)
    {
	   exit(1);
	   printf("Error opening file\n");
    }

    printf("pid of a current process is: %d\n", getpid());
   

    while(1)
    {
        printf("Meni: \nOdaberite opciju pod odgovarajucim brojem: \n");
        printf(" 1.Upisi u FIFO bafer\n 2.Procitaj iz FIFO bafera\n 3.Izadji\n");
        scanf("%d",&option);
    

    switch(option)
    {
        case 1: {
            printf("Upisati brojeve u hex formatu, upis se zavrsava dodavanjem Q i pritiskom enter\n");
            scanf("%s",buffer);
               
            if((strncmp(buffer, "0x", 2) != 0) && (strncmp(buffer, "Q", 2) != 0))
            {
                printf("Format mora biti heksadecimalan!\n");
            }
            nbytes=strlen(buffer);
            write(fd, buffer, nbytes);
            printf("Written: %s\n",buffer);
        }
        break;    
       
        case 2: {
            printf("Koliko brojeva zelite procitati?");
            scanf("%s", num);
            int n = atoi(num);  
            int x=0;

            count=strlen(num);
                //buffer[count]='\0';
            char str[10]="num=";
            strcat(str,num);
            char str1[10]="Q";
            strcat(str,str1);

            nbytes=strlen(str);
            write(fd, str, nbytes);
            fflush(stdout);
            bytes_read=read(fd, buffer1, 16);
            buffer1[bytes_read]='\0';
            printf("%s\n",buffer1);
            fflush(stdout);
            
            close(fd);
            return 0;
        }  

        break;    

       case 3: {     
         printf("Aplikacija uspesno zatvorena!\n");
            close(fd);
            return 0;
       }

        default:
        printf("Uneta vrednost mora biti od 1-3\n");
    }

}

    return 0;
   
}


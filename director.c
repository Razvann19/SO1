#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


int main(int argc, char *argv[]) {
    if(argc>6){
         fprintf(stderr,"Eroare: trebuie sa citim maxim 5 directoare!!");
         return 1;
    }
     struct stat status;
     int i;
     int pid;

     for(i=1;i<argc;i++)
     {
        if(stat(argv[i], &status)==-1){
        perror("Eroare la start");
        exit(1);
     }

    if(!S_ISDIR(status.st_mode)){
        fprintf(stderr,"%s nu este director!!",argv[i]);
        exit(1);
    }
    pid=fork();
        if(pid==-1){
        perror("Eroare la fork");
        exit(1);
     }
     else if(pid==0){
        execlp("ls","ls","-l",argv[i],NULL);
        perror("Eroare exec");
        exit(-1);
     }
    }
    while ((pid=wait(NULL))!=-1)
    {
        printf("Procesul fiu %d s-a terminat!\n",pid);
    }
    
    return 0;
}

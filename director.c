#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#define CITIRE 0
#define SCRIERE 1

int main(int argc, char *argv[]) {
    if (argc != 3) {
    printf("Eroare argumente!!\n");
    exit(1);
}

char *nume_fisier = argv[1];
char *cuvant_cheie = argv[2];

int pipe_cat_grep[2]; 
int pipe_grep_parinte[2]; 

if (pipe(pipe_cat_grep) <0 || pipe(pipe_grep_parinte) <0) {
    printf("Eroare la crearea pipe-ului\n");
    exit(1);
}
pid_t cat_pid = fork();

if (cat_pid == -1) {
    printf("Eroare la fork!!");
    exit(1);
}

if (cat_pid == 0) { 
    close(pipe_cat_grep[CITIRE]); 
    dup2(pipe_cat_grep[SCRIERE], 1); 
    close(pipe_cat_grep[SCRIERE]);
    
    execlp("cat", "cat", nume_fisier, NULL);
    printf("Eroare la execlp\n");
    exit(1);
}

pid_t grep_pid = fork();
if (grep_pid == -1) {
    printf("Eroare fork\n");
    return 1;
} 

if (grep_pid == 0) {
close(pipe_cat_grep[SCRIERE]); 
dup2(pipe_cat_grep[CITIRE], STDIN_FILENO); 
close(pipe_cat_grep[CITIRE]); 

close(pipe_grep_parinte[CITIRE]); 
dup2(pipe_grep_parinte[SCRIERE], 1);
close(pipe_grep_parinte[SCRIERE]); 

execlp("grep", "grep", cuvant_cheie, NULL);
printf("Eroare la exec\n");
exit(1);
}

close(pipe_cat_grep[CITIRE]); 
close(pipe_cat_grep[SCRIERE]); 
close(pipe_grep_parinte[SCRIERE]);


char buffer[10000];
ssize_t ceva;

printf("'%s' in '%s':\n", cuvant_cheie, nume_fisier);
while ((ceva = read(pipe_grep_parinte[CITIRE], buffer, sizeof(buffer))) > 0) {
    write(1, buffer, ceva);
}

close(pipe_grep_parinte[CITIRE]); 


waitpid(cat_pid, NULL, 0);
waitpid(grep_pid, NULL, 0);

return 0;

}
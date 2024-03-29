#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    char path[1024];
    off_t size; 
    time_t mtime; 
    ino_t inode; 
} FileSnapshot;


void direcor(const char *directoryPath) {
    DIR *dir = opendir(directoryPath);
    if (dir == NULL) {
        perror("Nu e bine");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directoryPath, entry->d_name);

        struct stat fileInfo;
        if (stat(fullPath, &fileInfo) == -1) continue;

        printf("Path: %s, Size: %ld, Last Modified: %ld, Inode: %ld\n",
               fullPath, fileInfo.st_size, fileInfo.st_mtime, fileInfo.st_ino);

        if (S_ISDIR(fileInfo.st_mode)) {
            direcor(fullPath); 
        }
    }

    closedir(dir);
}

int main(int argc,char *argv[])
{   struct stat patch_stat;

    //if(argc<2){
      //  fprintf(stderr, "Folosim directorul: ");
        //return 1;  
    //}  
    if(stat(argv[1], &patch_stat)!=0){
        perror("Eroare");
        return 1;  
    }  
    
    printf("%s este director!\n", argv[1]);
    direcor(argv[1]);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>

#define MAX_PATH_LENGTH 1024
#define BUFFER_SIZE 2048

typedef struct {
    int lower_case[27];
    int upper_case[27];
} Table;

void procLitere(const char *filename, Table *letter);
void scrieHistograma(const Table *letter, const char *output_file);
void scrieStatistica(const char *path, const char *type, int stat_fd);
int eDirector(const char *path);
void parcDirector(const char *directory, Table *letter, int stat_fd);

void procLitere(const char *filename, Table *letter) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ceva nu e bine!!");
        exit(-1);
    }

    char ch;
    while (read(fd, &ch, 1) > 0) {
        if (ch >= 'A' && ch <= 'Z')
            letter->upper_case[ch - 'A']++;
        else if (ch >= 'a' && ch <= 'z')
            letter->lower_case[ch - 'a']++;
    }

    close(fd);
}

void scrieHistograma(const Table *letter, const char *output_file) {
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Eroare la deschidere fisier histogram!!");
        exit(-1);
    }

    char buffer[BUFFER_SIZE];
    int offset = snprintf(buffer, sizeof(buffer), "Literele din histograma sunt:\n");

    for (int i = 0; i < 26; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%c : %d\n", 'a' + i, letter->lower_case[i]);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%c : %d\n", 'A' + i, letter->upper_case[i]);
    }

    if (write(fd, buffer, offset) < 0) {
        perror("Nu se poate scrie in histograma!!");
        exit(-1);
    }

    close(fd);
}

void scrieStatistica(const char *path, const char *type, int stat_fd) {
    char buffer[BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "%s %s\n", path, type);
    if (write(stat_fd, buffer, len) < 0) {
        perror("Nu se poate scrie in fisierul de statistica!!");
        exit(-1);
    }
}

int eDirector(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

void parcDirector(const char *directory, Table *letter, int stat_fd) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Nu se poate deschide directorul!!");
        exit(-1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[MAX_PATH_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

            if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
                procLitere(path, letter);
                scrieStatistica(path, "regular_file", stat_fd);
            } else if (entry->d_type == DT_LNK) {
                scrieStatistica(path, "symlink", stat_fd);
            } else if (entry->d_type == DT_DIR) {
                scrieStatistica(path, "directory", stat_fd);
                parcDirector(path, letter, stat_fd);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Trebuie sa folosim: %s <director> <fisier_histograma> <fisier_statistica_generala>\n", argv[0]);
        exit(-1);
    }

    if (!eDirector(argv[1])) {
        fprintf(stderr, "%s nu este director!\n", argv[1]);
        exit(-1);
    }

    int stat_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (stat_fd == -1) {
        perror("Nu se poate deschide fisierul de statistica!!");
        exit(-1);
    }

    Table letter = {0};
    parcDirector(argv[1], &letter, stat_fd);
    close(stat_fd);

    scrieHistograma(&letter, argv[2]);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define SNAPSHOT_FILE "snapshot.txt"
#define TEMP_SNAPSHOT_FILE "temp_snapshot.txt"
#define MAX_PATH 1024
#define BUFFER_SIZE 2048

int isDir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return 0;
    return S_ISDIR(statbuf.st_mode);
}

int endsWithTxt(const char *filename) {
    const char *dot = strrchr(filename, '.');
    return dot && !strcmp(dot, ".txt");
}

void printFileContent(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Nu s-a putut deschide fisierul: %s\n", path);
        return;
    }
    char buffer[BUFFER_SIZE];
    printf("Modificari in %s:\n", path);
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
    fclose(file);
    printf("\n");
}

void traverseDirectory(const char *basePath, FILE *tempSnapshot) {
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    if (!dir) return;

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;

        snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);

        if (isDir(path)) {
            fprintf(tempSnapshot, "%s,d\n", path);
            traverseDirectory(path, tempSnapshot);
        } else {
            FILE *file = fopen(path, "r");
            if (!file) continue;
            fseek(file, 0L, SEEK_END);
            long size = ftell(file);
            fclose(file);
            fprintf(tempSnapshot, "%s,f,%ld\n", path, size);
        }
    }
    closedir(dir);
}

void compareAndUpdateSnapshot() {
    char line[BUFFER_SIZE];
    char path[MAX_PATH], type;
    long size;
    char *snapshotLines[10000];
    int linesCount = 0;

    printf("Directoare adaugate sau modificate:\n");

    FILE *snapshot = fopen(SNAPSHOT_FILE, "r");
    if (snapshot) {
        while (fgets(line, sizeof(line), snapshot)) {
            line[strcspn(line, "\n")] = 0;
            snapshotLines[linesCount++] = strdup(line);
        }
        fclose(snapshot);
    }

    FILE *tempSnapshot = fopen(TEMP_SNAPSHOT_FILE, "r");
    if (!tempSnapshot) {
        perror("Eroare la deschiderea fisierului snapshot temporar");
        return;
    }

    printf("Fisiere adaugate:\n");
    while (fgets(line, sizeof(line), tempSnapshot)) {
        line[strcspn(line, "\n")] = 0;
        sscanf(line, "%[^,],%c,%ld", path, &type, &size);

        int found = 0;
        for (int i = 0; i < linesCount; i++) {
            if (strcmp(line, snapshotLines[i]) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            if (type == 'd') {
                printf("Director: %s\n", path);
            } else if (type == 'f') {
                printf("%s\n", path);
                if (endsWithTxt(path)) {
                    printFileContent(path);
                }
            }
        }
    }
    fclose(tempSnapshot);

    remove(SNAPSHOT_FILE);
    rename(TEMP_SNAPSHOT_FILE, SNAPSHOT_FILE);

    for (int i = 0; i < linesCount; i++) {
        free(snapshotLines[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Folosim: %s <directory_path>\n", argv[0]);
        return 1;
    }

    FILE *tempSnapshot = fopen(TEMP_SNAPSHOT_FILE, "w");
    if (!tempSnapshot) {
        perror("Eroare la deschiderea fisierului temporar");
        return 1;
    }

    traverseDirectory(argv[1], tempSnapshot);
    fclose(tempSnapshot);

    compareAndUpdateSnapshot();

    return 0;
}

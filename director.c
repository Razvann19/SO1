#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_DIR 10
#define MAX_PATH_LEN 2048
#define MAX_SNAPSHOT_SIZE 4096

typedef struct {
    char nume[256];
    char caleCompleta[1024];
    char tipFisier[10];
    long dimensiune;
    char areLinkSimbolic[3];
    char ultimaModificare[30];
    char permisiuni[11];
} Metadate;

void cePermisiuni(struct stat *statistici, char *permisiuni) {
    const char caractere[] = "rwxrwxrwx";
    for (int i = 0; i < 9; i++) {
        permisiuni[i] = (statistici->st_mode & (1 << (8-i))) ? caractere[i] : '-';
    }
    permisiuni[9] = '\0';
}

void colecteazaMetadate(const char *cale, Metadate *md) {
    struct stat statistici;
    if (lstat(cale, &statistici) == 0) {
        const char *numeFisier = strrchr(cale, '/');
        numeFisier = (numeFisier) ? numeFisier + 1 : cale;

        snprintf(md->nume, sizeof(md->nume), "%s", numeFisier);
        snprintf(md->caleCompleta, sizeof(md->caleCompleta), "%s", cale);
        snprintf(md->tipFisier, sizeof(md->tipFisier), "%s", (S_ISDIR(statistici.st_mode)) ? "Director" : "Fisier");
        md->dimensiune = statistici.st_size;
        snprintf(md->areLinkSimbolic, sizeof(md->areLinkSimbolic), "%s", (S_ISLNK(statistici.st_mode)) ? "Da" : "Nu");
        strftime(md->ultimaModificare, sizeof(md->ultimaModificare), "%c", localtime(&statistici.st_mtime));
        cePermisiuni(&statistici, md->permisiuni);
    }
}

void scrieMetadateInFisier(int fd, Metadate *md, int adancime) {
    char buffer[MAX_SNAPSHOT_SIZE];
    int lungime = sprintf(buffer,
        "%*sNume: %s\n"
        "%*sPatch: %s\n"
        "%*sTip fisier: %s\n"
        "%*sDimensiune: %ld bytes\n"
        "%*sAre link simbolic: %s\n"
        "%*sUltima modificare: %s\n"
        "%*sPermisiuni: %s\n",
        adancime, "", md->nume,
        adancime + 4, "", md->caleCompleta,
        adancime + 4, "", md->tipFisier,
        adancime + 4, "", md->dimensiune,
        adancime + 4, "", md->areLinkSimbolic,
        adancime + 4, "", md->ultimaModificare,
        adancime + 4, "", md->permisiuni
    );
    write(fd, buffer, lungime);
}

void scrieMetadate(const char *caleDeBaza, int fd, int adancime) {
    DIR *dir = opendir(caleDeBaza);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char cale[2048];
            snprintf(cale, sizeof(cale), "%s/%s", caleDeBaza, entry->d_name);
            Metadate md;
            colecteazaMetadate(cale, &md);
            scrieMetadateInFisier(fd, &md, adancime); 

            if (entry->d_type == DT_DIR) {
                scrieMetadate(cale, fd, adancime + 4); 
            }
        }
    }

    closedir(dir);
}


void creeazaSnapshotDetaliat(const char *numeDirector, const char *numeFisierSnapshotDetaliat) {
    int fdSnapshotDetaliat = open(numeFisierSnapshotDetaliat, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdSnapshotDetaliat == -1) {
        perror("Eroare la deschiderea fisierului snapshot!!");
        return;
    }

    scrieMetadate(numeDirector, fdSnapshotDetaliat, 0);
    close(fdSnapshotDetaliat);
}

void creeazaSnapshotSimplu(const char *numeDirector, const char *numeFisierSnapshotSimplu) {
    FILE *fSnapshotSimplu = fopen(numeFisierSnapshotSimplu, "w");
    if (!fSnapshotSimplu) {
        perror("Eroare la deschiderea fisierului snapshot!!");
        return;
    }

    DIR *dir = opendir(numeDirector);
    if (!dir) {
        perror("Nu se poate deschide directorul!!");
        fclose(fSnapshotSimplu);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char caleCompleta[1024];
            snprintf(caleCompleta, sizeof(caleCompleta), "%s/%s", numeDirector, entry->d_name);
            struct stat fileInfo;
            if (stat(caleCompleta, &fileInfo) == 0) {

                char ultimaModificare[32];
                strftime(ultimaModificare, sizeof(ultimaModificare), "%Y-%m-%d %H:%M:%S", localtime(&fileInfo.st_mtime));

                fprintf(fSnapshotSimplu, "%s - %s\n", ultimaModificare, entry->d_name);
            } else {
                fprintf(fSnapshotSimplu, "Nu se pot obtine informatiile - %s\n", entry->d_name);
            }
        }
    }

    closedir(dir);
    fclose(fSnapshotSimplu);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Folosim: %s -o <director_output> <director1> [<director2> ... <directorN>]\n", argv[0]);
        return 1;
    }

    char *directoare[MAX_DIR];
    int numarDirectoare = 0;
    char directorOutput[MAX_PATH_LEN] = {0};

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i < argc) {
                strncpy(directorOutput, argv[i], MAX_PATH_LEN - 1);
                directorOutput[MAX_PATH_LEN - 1] = '\0';
            } else {
                fprintf(stderr, "Optiunea '-o' doreste un nume de director.\n");
                return 1;
            }
        } else {
            if (numarDirectoare < MAX_DIR) {
                directoare[numarDirectoare++] = argv[i];
            } else {
                fprintf(stderr, "Numarul maxim de directoare este %d.\n", MAX_DIR);
                return 1;
            }
        }
    }

    if (strlen(directorOutput) == 0) {
        fprintf(stderr, "Nu a fost specificat un director de output.\n");
        return 1;
    }

    char numeFisierSnapshotDetaliat[MAX_PATH_LEN];
    int lungimeDetaliat = snprintf(numeFisierSnapshotDetaliat, MAX_PATH_LEN, "%s/snapshot_detaliat.txt", directorOutput);
    if (lungimeDetaliat >= MAX_PATH_LEN || lungimeDetaliat < 0) {
        fprintf(stderr, "Eroare la construirea snapshot-ul detaliat.\n");
        return 1;
    }
    creeazaSnapshotDetaliat(".", numeFisierSnapshotDetaliat);

    for (int i = 0; i < numarDirectoare; ++i) {
        char *numeDirector = strrchr(directoare[i], '/');
        if (numeDirector != NULL) {
            numeDirector++;
        } else {
            numeDirector = directoare[i];
        }

        char numeFisierSnapshotSimplu[MAX_PATH_LEN];
        int lungimeSimplu = snprintf(numeFisierSnapshotSimplu, MAX_PATH_LEN, "%s/snapshot_%s.txt", directorOutput, numeDirector);
        if (lungimeSimplu >= MAX_PATH_LEN || lungimeSimplu < 0) {
            fprintf(stderr, "Eroare la construirea snapshot-ul simplu %s.\n", numeDirector);
            continue;
        }
        creeazaSnapshotSimplu(directoare[i], numeFisierSnapshotSimplu);
    }

    return 0;
}

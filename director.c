#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h> 
#include <sys/errno.h>
#include <stdbool.h>
#include <sys/wait.h>

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

void listaFisiereDirector(char *directorOutput) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directorOutput);
    if (dir == NULL) {
        perror("Nu se poate deschide directorului de output!!");
        return;
    }

    printf("E bine -_- Toate detaliile se afla in directorul \"%s\":\n", directorOutput);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
}

bool esteDirectorDuplicat(char *director, char *directoare[], int numarDirectoare) {
    for (int i = 0; i < numarDirectoare; i++) {
        if (strcmp(directoare[i], director) == 0) {
            return true;
        }
    }
    return false;
}

void proceseazaDirector(char *numeDirector, char *directorOutput, int fdProcese) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Eroare la fork");
        exit(1);
    } else if (pid == 0) {
        char numeFisierSnapshotSimplu[MAX_PATH_LEN];
        snprintf(numeFisierSnapshotSimplu, MAX_PATH_LEN, "%s/snapshot_%s.txt", directorOutput, numeDirector);
        creeazaSnapshotSimplu(numeDirector, numeFisierSnapshotSimplu);
        dprintf(fdProcese, "Captura pentru directorul '%s' creata cu succes!\n", numeDirector);
        exit(0);
    }
}

void asteaptaProceseCopil(int numarDirectoare, int fdProcese) {
    int status;
    pid_t pid;
    int proceseFinalizate = 0;
    
    while (proceseFinalizate < numarDirectoare) {
        pid = wait(&status);
        if (pid > 0) {
            char buffer[256];
            int length = snprintf(buffer, sizeof(buffer),"Procesul copil cu PID %d s-a incheiat cu codul de iesire %d.\n",pid, WEXITSTATUS(status));
            write(fdProcese, buffer, length);
            proceseFinalizate++;
        }
    }
}

void scrieMetadateFormatate(const char *caleDeBaza, FILE *file, int adancime) {
    DIR *dir = opendir(caleDeBaza);
    if (!dir) {
        fprintf(file, "%*sNu se poate deschide directorul %s\n", adancime * 2, "", caleDeBaza);
        return;
    }

    struct dirent *entry;
    char cale[2048];
    struct stat statbuf;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(cale, sizeof(cale), "%s/%s", caleDeBaza, entry->d_name);
        if (stat(cale, &statbuf) == -1) {
            fprintf(file, "%*sEroare la obtinerea informatiilor pentru %s\n", adancime * 2, "", entry->d_name);
            continue;
        }

        if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(file, "%*s|_ %s (Drepturi: %c%c%c%c%c%c%c%c%c%c)\n",
        adancime * 2, "", entry->d_name,
        S_ISDIR(statbuf.st_mode) ? 'd' : '-',
        (statbuf.st_mode & S_IRUSR) ? 'r' : '-',
        (statbuf.st_mode & S_IWUSR) ? 'w' : '-',
        (statbuf.st_mode & S_IXUSR) ? 'x' : '-',
        (statbuf.st_mode & S_IRGRP) ? 'r' : '-',
        (statbuf.st_mode & S_IWGRP) ? 'w' : '-',
        (statbuf.st_mode & S_IXGRP) ? 'x' : '-',
        (statbuf.st_mode & S_IROTH) ? 'r' : '-',
        (statbuf.st_mode & S_IWOTH) ? 'w' : '-',
        (statbuf.st_mode & S_IXOTH) ? 'x' : '-');
        }
    }
    closedir(dir);
}

void genereazaRaportStructuraDirector(const char *caleDirector, FILE *file) {
    if (file == NULL) {
        fprintf(stderr, "Fisierul de raport nu este deschis.\n");
        return;
    }

    scrieMetadateFormatate(caleDirector, file, 0);
}

bool areDrepturiLipsa(const char *caleFisier) {
    struct stat statbuf;
    if (stat(caleFisier, &statbuf) != 0) {
        perror("Eroare la accesarea fisierului!!");
        return false;
    }
    return (statbuf.st_mode & 0777) == 0;
}

void izoleazaDacaMalitios(const char *caleFisier, const char *caleIzolata) {
    char command[1024];
    chmod(caleFisier, S_IRUSR);

    snprintf(command, sizeof(command), "./verify_for_malicious.sh %s", caleFisier);
    int result = system(command);
    chmod(caleFisier, 000);

    if (result != 0) {
        char moveCommand[2048];
        snprintf(moveCommand, sizeof(moveCommand), "mv %s %s/", caleFisier, caleIzolata);
        system(moveCommand);
        printf("Fisierul %s a fost mutat in izolare.\n", caleFisier);
    }
}

void proceseazaVirus(const char *caleDirector, const char *caleIzolata) {
    DIR *dir;
    struct dirent *entry;
    char caleCompleta[1024];
    struct stat statbuf;

    if ((dir = opendir(caleDirector)) == NULL) {
        perror("Eroare la deschidere director!!");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(caleCompleta, sizeof(caleCompleta), "%s/%s", caleDirector, entry->d_name);
        if (stat(caleCompleta, &statbuf) != 0) {
            perror("Eroare la status!!");
            continue;
        }

        if (!S_ISDIR(statbuf.st_mode) && areDrepturiLipsa(caleCompleta)) {
            izoleazaDacaMalitios(caleCompleta, caleIzolata);
        }
    }
    closedir(dir);
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
                if (!esteDirectorDuplicat(argv[i], directoare, numarDirectoare)) {
                    directoare[numarDirectoare++] = argv[i];
                } else {
                    fprintf(stderr, "Directorul '%s' a fost specificat de mai multe ori.\n", argv[i]);
                    return 1;
                }
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

    struct stat statbuf;
    if (stat(directorOutput, &statbuf) != 0) {
        if (errno == ENOENT) {
            if (mkdir(directorOutput, 0755) != 0) {
                perror("Nu se poate crea directorul de output!!");
                return 1;
            }
        } else {
            perror("Eroare la accesarea directorului de output!!");
            return 1;
        }
    }

    char numeFisierSnapshotDetaliat[MAX_PATH_LEN];
    int lungimeDetaliat = snprintf(numeFisierSnapshotDetaliat, MAX_PATH_LEN, "%s/snapshot_detaliat.txt", directorOutput);
    if (lungimeDetaliat >= MAX_PATH_LEN || lungimeDetaliat < 0) {
        fprintf(stderr, "Nu se poate construi snapshot-ul detaliat.\n");
        return 1;
    }
    creeazaSnapshotDetaliat(".", numeFisierSnapshotDetaliat);


    char numeFisierProcese[MAX_PATH_LEN];
    if (snprintf(numeFisierProcese, MAX_PATH_LEN, "%s/snapshot_procese.txt", directorOutput) >= MAX_PATH_LEN) {
        fprintf(stderr, "Calea este prea lunga pentru buffer.\n");
        return 1;
    }

    int fdProcese = open(numeFisierProcese, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdProcese == -1) {
        perror("Nu se poate deschide fisierului pentru procese!!");
        return 1;
    }

    for (int i = 0; i < numarDirectoare; ++i) {
        char *numeDirector = strrchr(directoare[i], '/');
        numeDirector = (numeDirector != NULL) ? numeDirector + 1 : directoare[i];
        proceseazaDirector(numeDirector, directorOutput, fdProcese);
    }

    asteaptaProceseCopil(numarDirectoare, fdProcese);
    close(fdProcese);
    
    listaFisiereDirector(directorOutput);

    char *directorIesire = argv[2];
    char caleFisierRaport[1024];
    snprintf(caleFisierRaport, sizeof(caleFisierRaport), "%s/snapshot_permisiuni.txt", directorIesire);

    FILE *file = fopen(caleFisierRaport, "w");
    if (!file) {
        perror("Nu s-a putut deschide fișierul pentru scriere");
        return 1;
    }

    for (int i = 4; i < argc; i++) {
        genereazaRaportStructuraDirector(argv[i], file);
    }

    char *caleIzolata = argv[3];

    for (int i = 4; i < argc; i++) {
        proceseazaVirus(argv[i], caleIzolata);
    }    

    fclose(file);
    return 0;
}

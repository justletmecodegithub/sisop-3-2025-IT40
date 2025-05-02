/* image_server.c - Program Sisop moudl 3 anjasssssssss walawe aku mumet */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define DB_FOLDER "server/database/"
#define LOG_FILE "server/server.log"


// fungsi nyatet semua aksi2 mencurigakan wakak
void tulisLog(const char *asal, const char *aksi, const char *info) {
    FILE *logFile = fopen(LOG_FILE, "a");
    if (!logFile) return;

    time_t skrg = time(NULL);
    struct tm *t = localtime(&skrg);
    char waktu[32];
    strftime(waktu, sizeof(waktu), "%Y-%m-%d %H:%M:%S", t);

    fprintf(logFile, "[%s][%s]: [%s] [%s]\n", asal, waktu, aksi, info);
    fclose(logFile);
}

// reverse string
void balikString(char *str) {
    int pnjg = strlen(str);
    for (int i = 0; i < pnjg / 2; i++) {
        char tmp = str[i];
        str[i] = str[pnjg - 1 - i];
        str[pnjg - 1 - i] = tmp;
    }
}

// hex to bytes converter
int hexKeByte(const char *hex, unsigned char *hasil) {
    int pnjg = strlen(hex);
    int jml = 0;
    for (int i = 0; i < pnjg; i += 2) {
        if (i + 1 >= pnjg) break;
        sscanf(hex + i, "%2hhx", &hasil[jml]);
        jml++;
    }
    return jml;
}

// thread client, yg hold tiap client yg connect 
void *handleClient(void *arg) {
    int sockClient = *(int *)arg;
    free(arg);

    char pesanDariClient[BUFFER_SIZE] = {0};
    recv(sockClient, pesanDariClient, sizeof(pesanDariClient), 0);

    char *perintah = strtok(pesanDariClient, "|");
    char *isiData = strtok(NULL, "");

    if (!perintah || !isiData) {
        send(sockClient, "Invalid command cuy\n", 21, 0);
        close(sockClient);
        return NULL;
    }

    if (strcmp(perintah, "DECRYPT") == 0) {
        tulisLog("Client", "DECRYPT", "data teks aneh");

        balikString(isiData);
        unsigned char hasilDecode[BUFFER_SIZE] = {0};
        int panjangData = hexKeByte(isiData, hasilDecode);

        time_t skrg = time(NULL);
        char namaFile[64];
        snprintf(namaFile, sizeof(namaFile), "%s%ld.jpeg", DB_FOLDER, skrg);

        FILE *fileKeluar = fopen(namaFile, "wb");
        if (fileKeluar) {
            fwrite(hasilDecode, 1, panjangData, fileKeluar);
            fclose(fileKeluar);
            tulisLog("Server", "SAVE", strrchr(namaFile, '/') + 1);
            send(sockClient, strrchr(namaFile, '/') + 1, strlen(strrchr(namaFile, '/') + 1), 0);
        } else {
            send(sockClient, "Gagal nyimpen file bro\n", 25, 0);
        }

    } else if (strcmp(perintah, "DOWNLOAD") == 0) {
        tulisLog("Client", "DOWNLOAD", isiData);

        char pathFile[128];
        snprintf(pathFile, sizeof(pathFile), "%s%s", DB_FOLDER, isiData);
        FILE *file = fopen(pathFile, "rb");

        if (!file) {
            send(sockClient, "File kaga ada cuy\n", 19, 0);
            tulisLog("Server", "ERROR", "File kaga ketemu");
        } else {
            unsigned char kontenFile[BUFFER_SIZE] = {0};
            int dibaca = fread(kontenFile, 1, sizeof(kontenFile), file);
            fclose(file);
            send(sockClient, kontenFile, dibaca, 0);
            tulisLog("Server", "UPLOAD", isiData);
        }
    } else {
        send(sockClient, "Perintah ga jelas sob\n", 23, 0);
    }

    close(sockClient);
    return NULL;
}

// penDaemon
void jadiDaemon() {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    umask(0);
    setsid();
    // chdir("/") tadinya ini dipake...ta ubah jadi komentar biar linenya gakepake..soalnya gaperlu keluar directory kan ya?
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

//main function
int main() {
    mkdir("server/database", 0755); // in case gada foldernya..bikin dulu

    jadiDaemon(); //daemon aktivator

    int fdServer, sockBaru;
    struct sockaddr_in alamat;
    socklen_t panjangAlamat = sizeof(alamat);
    int opsi = 1;

    fdServer = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &opsi, sizeof(opsi));

    alamat.sin_family = AF_INET;
    alamat.sin_addr.s_addr = INADDR_ANY;
    alamat.sin_port = htons(PORT);

    bind(fdServer, (struct sockaddr *)&alamat, sizeof(alamat));
    listen(fdServer, MAX_CLIENTS);

    while (1) {
        sockBaru = accept(fdServer, (struct sockaddr *)&alamat, &panjangAlamat);
        if (sockBaru >= 0) {
            pthread_t benang;
            int *pSock = malloc(sizeof(int));
            *pSock = sockBaru;
            pthread_create(&benang, NULL, handleClient, pSock);
            pthread_detach(benang);
        }
    }

    return 0;
}

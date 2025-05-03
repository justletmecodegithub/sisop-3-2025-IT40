// dispatcher.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>

#define MAKS 100

typedef struct {
    char nama[100];
    char alamat[100];
    char tipe[10];
    char status[100];
} Pesanan;

void catat_log_reguler(const char *agen, const char *nama, const char *tujuan) {
    FILE *f = fopen("delivery.log", "a");
    if (!f) {
        printf("Gagal membuka file log.\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(f, "[%02d/%02d/%04d %02d:%02d:%02d] [AGENT %s] Reguler package delivered to %s in %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agen, nama, tujuan);
    fclose(f);

    
    printf("[REGULER-DELIV] Agent %s mengirim ke %s (%s)\n", agen, nama, tujuan);
}

void unduh_csv() {
    const char *url = "https://drive.usercontent.google.com/u/0/uc?id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9&export=download";
    const char *nama_file = "delivery_order.csv";
    char perintah[300];

    snprintf(perintah, sizeof(perintah), "curl -L -o %s \"%s\"", nama_file, url);
    int hasil = system(perintah);
    if (hasil != 0) {
        fprintf(stderr, "Gagal mengunduh file.\n");
        exit(1);
    }
}

void tampil_log() {
    FILE *f = fopen("delivery.log", "r");
    if (!f) {
        puts("Log kosong atau belum dibuat.");
        return;
    }
    char buf[200];
    while (fgets(buf, sizeof(buf), f)) {
        printf("%s", buf);
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    key_t kunci = 1234;
    int shmid = shmget(kunci, sizeof(Pesanan) * MAKS, IPC_CREAT | 0666);
    Pesanan *data = (Pesanan *) shmat(shmid, NULL, 0);

    if (argc == 1) {
        unduh_csv();

        FILE *fp = fopen("delivery_order.csv", "r");
        if (!fp) {
            perror("gagal buka file CSV");
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (fscanf(fp, "%[^,],%[^,],%[^\n]\n", data[i].nama, data[i].alamat, data[i].tipe) == 3) {
            strcpy(data[i].status, "Pending");
            i++;
            if (i >= MAKS) break;
        }
        fclose(fp);

        printf("Sukses load %d pesanan ke shared memory\n", i);
        shmdt(data);
        return 0;
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        for (int i = 0; i < MAKS; i++) {
            if (strcmp(data[i].nama, argv[2]) == 0 &&
                strcmp(data[i].tipe, "Reguler") == 0 &&
                strcmp(data[i].status, "Pending") == 0) {

                pid_t pid = fork();
                if (pid == 0) {
                    strcpy(data[i].status, argv[2]);
                    catat_log_reguler(argv[2], data[i].nama, data[i].alamat);
                    exit(0);
                } else {
                    wait(NULL);
                    break;
                }
            }
        }
    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        for (int i = 0; i < MAKS; i++) {
            if (strcmp(data[i].nama, argv[2]) == 0) {
                if (strcmp(data[i].status, "Pending") == 0)
                    printf("Status untuk %s: Belum dikirim\n", data[i].nama);
                else
                    printf("Status untuk %s: Sudah dikirim oleh AGENT %s\n", data[i].nama, data[i].status);
            }
        }
    } else if (strcmp(argv[1], "-list") == 0) {
        for (int i = 0; i < MAKS; i++) {
            if (strlen(data[i].nama) == 0) break;
            printf("Pesanan dari %s [%s]\n", data[i].nama, data[i].status);
        }
    } else if (strcmp(argv[1], "-log") == 0) {
        tampil_log();
    } else {
        fprintf(stderr, "Argumen tidak valid. Gunakan: -deliver/-status/-list/-log\n");
    }

    shmdt(data);
    return 0;
}

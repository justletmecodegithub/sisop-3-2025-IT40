#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAKS_PESANAN 100
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char nama[100];
    char alamat[100];
    char tipe[10];
    char status[100];
} Pesanan;

Pesanan *semuadata;
int banyak_data = 0;
#define counter banyak_data 

void catat_ke_log(const char *agen, const char *orang, const char *ke) {
    FILE *f = fopen("delivery.log", "a");
    if (!f) {
        printf("Gagal buka log\n");
        return;
    }

    time_t sekarang = time(NULL);
    struct tm *waktu = localtime(&sekarang);

    fprintf(f, "[%02d/%02d/%d %02d:%02d:%02d] [%s] Express package delivered to %s in %s\n",
            waktu->tm_mday, waktu->tm_mon + 1, waktu->tm_year + 1900,
            waktu->tm_hour, waktu->tm_min, waktu->tm_sec,
            agen, orang, ke);

    fclose(f);
}


void* jalan_agen(void* arg) {
    char* agen = (char*) arg;
    int i, ada = 1;

    while (ada) {
        ada = 0;

        for (i = 0; i < counter; i++) {
            pthread_mutex_lock(&lock); 

            if (strcmp(semuadata[i].tipe, "Express") == 0 &&
                strcmp(semuadata[i].status, "Pending") == 0) {

                printf("-> %s memproses pesanan %d\n", agen, i + 1);
                strcpy(semuadata[i].status, agen); 
                catat_ke_log(agen, semuadata[i].nama, semuadata[i].alamat); 
                sleep(1); 

                ada = 1; 
            }

            pthread_mutex_unlock(&lock); 
        }
    }

    return NULL;
}

int main() {
    key_t kunci = 1234;
    int mem_id = shmget(kunci, sizeof(Pesanan) * MAKS_PESANAN, 0666);
    if (mem_id < 0) {
        perror("shmget gagal");
        return 1;
    }

    semuadata = (Pesanan*) shmat(mem_id, NULL, 0);
    if ((void*)semuadata == (void*)-1) {
        perror("shmat gagal");
        return 1;
    }

    
    for (int z = 0; z < MAKS_PESANAN; z++) {
        if (strlen(semuadata[z].nama) == 0) break;
        banyak_data++;
    }

    printf("[INFO] Jumlah pesanan ditemukan: %d\n", banyak_data);

    pthread_t ag1, ag2, ag3;
    pthread_create(&ag1, NULL, jalan_agen, "AGENT A");
    pthread_create(&ag2, NULL, jalan_agen, "AGENT B");
    pthread_create(&ag3, NULL, jalan_agen, "AGENT C");

    pthread_join(ag1, NULL);
    pthread_join(ag2, NULL);
    pthread_join(ag3, NULL);

    shmdt(semuadata);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50

typedef struct {
    char username[MAX_NAME];
    int level, exp, atk, hp, def;
    int banned;
    int notification_on;
    key_t shm_key;
} Hunter;

typedef struct {
    char name[MAX_NAME];
    int min_level, atk, hp, def, exp;
    key_t shm_key;
} Dungeon;

typedef struct {
    Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    sem_t hunter_sem;
    sem_t dungeon_sem;
    int system_active;
} SystemData;

SystemData *sys_data = NULL;
int shm_id = -1;

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

void init_shared_memory() {
    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("[System] shmget failed");
        exit(EXIT_FAILURE);
    }

    sys_data = (SystemData*)shmat(shm_id, NULL, 0);
    if (sys_data == (void*)-1) {
        perror("[System] shmat failed");
        exit(EXIT_FAILURE);
    }

    struct shmid_ds buf;
    shmctl(shm_id, IPC_STAT, &buf);
    if (buf.shm_nattch == 1) {
        memset(sys_data, 0, sizeof(SystemData));
        sem_init(&sys_data->hunter_sem, 1, 1);
        sem_init(&sys_data->dungeon_sem, 1, 1);
        sys_data->system_active = 1;
        printf("[System] Initialized shared memory\n");
    }
}

void list_hunters() {
    sem_wait(&sys_data->hunter_sem);
    printf("\n=== HUNTER INFO ===\n");
    for (int i = 0; i < sys_data->num_hunters; i++) {
        Hunter *h = &sys_data->hunters[i];
        printf("%d. %s | Lv:%d ATK:%d HP:%d DEF:%d EXP:%d [%s]\n",
               i + 1, h->username, h->level, h->atk, h->hp, h->def, h->exp,
               h->banned ? "BANNED" : "ACTIVE");
    }
    sem_post(&sys_data->hunter_sem);
}

void list_dungeons() {
    sem_wait(&sys_data->dungeon_sem);
    printf("\n=== DUNGEON INFO ===\n");
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        Dungeon *d = &sys_data->dungeons[i];
        printf("%d. %s | Lv %d+ | Key:%d\n", i + 1, d->name, d->min_level, d->shm_key);
        printf("    ATK:%d HP:%d DEF:%d EXP:%d\n", d->atk, d->hp, d->def, d->exp);
    }
    sem_post(&sys_data->dungeon_sem);
}

void generate_dungeon() {
    const char *names[] = {
        "Double Dungeon", "Demon Castle", "Pyramid Dungeon",
        "Red Gate Dungeon", "Hunters Guild Dungeon", "Busan A-Rank Dungeon",
        "Insects Dungeon", "Goblins Dungeon", "D-Rank Dungeon",
        "Gwanak Mountain Dungeon", "Hapjeong Subway Station Dungeon"
    };

    sem_wait(&sys_data->dungeon_sem);

    if (sys_data->num_dungeons >= MAX_DUNGEONS) {
        printf("[!] Maximum dungeons reached\n");
        sem_post(&sys_data->dungeon_sem);
        return;
    }

    Dungeon *d = &sys_data->dungeons[sys_data->num_dungeons];
    int index = rand() % 11;
    strncpy(d->name, names[index], MAX_NAME - 1);
    d->min_level = rand() % 5 + 1;
    d->atk = rand() % 51 + 100;
    d->hp = rand() % 51 + 50;
    d->def = rand() % 26 + 25;
    d->exp = rand() % 151 + 150;
    d->shm_key = ftok("/tmp", 'D' + sys_data->num_dungeons);

    printf("[+] Generated: %s (Lv %d+)\n", d->name, d->min_level);
    printf("    Key: %d | Stats: ATK:%d HP:%d DEF:%d EXP:%d\n", 
           d->shm_key, d->atk, d->hp, d->def, d->exp);
    sys_data->num_dungeons++;

    sem_post(&sys_data->dungeon_sem);
}

void ban_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to ban: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            sys_data->hunters[i].banned = 1;
            printf("[*] %s has been banned\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("[!] Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}

void unban_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to unban: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            sys_data->hunters[i].banned = 0;
            printf("[*] %s has been unbanned\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("[!] Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}

void reset_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to reset: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            Hunter *h = &sys_data->hunters[i];
            h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;
            h->banned = 0;
            printf("[*] %s has been reset\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("[!] Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}

void system_menu() {
    int choice;
    do {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. Hunter Info\n2. Dungeon Info\n3. Generate Dungeon\n");
        printf("4. Ban Hunter\n5. Unban Hunter\n6. Reset Hunter\n");
        printf("7. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: list_hunters(); break;
            case 2: list_dungeons(); break;
            case 3: generate_dungeon(); break;
            case 4: ban_hunter(); break;
            case 5: unban_hunter(); break;
            case 6: reset_hunter(); break;
            case 7: return;
            default: printf("Invalid choice\n");
        }
    } while (1);
}

int main() {
    srand(time(NULL));
    signal(SIGINT, SIG_IGN);
    init_shared_memory();
    system_menu();
    shmdt(sys_data);
    return 0;
}

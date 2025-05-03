#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50
#define EXP_LEVELUP 500
#define NOTIF_INTERVAL 3

typedef struct {
    char username[MAX_NAME];
    int level, exp, atk, hp, def;
    int banned;
    int notification_on;
    key_t shm_key;
    pthread_t notif_thread;
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
int shm_id;
volatile sig_atomic_t shutdown_flag = 0;

void press_enter() {
    printf("Press enter to continue...");
    while (getchar() != '\n');
    getchar();
}

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

void *notification_thread(void *arg) {
    int idx = *(int *)arg;
    free(arg);
    while (!shutdown_flag) {
        sleep(NOTIF_INTERVAL);
        if (!sys_data->hunters[idx].notification_on) continue;

        sem_wait(&sys_data->dungeon_sem);
        int found = 0;
        for (int i = 0; i < sys_data->num_dungeons; i++) {
            if (sys_data->dungeons[i].min_level <= sys_data->hunters[idx].level) {
                Dungeon *d = &sys_data->dungeons[i];
                printf("\n=== HUNTER SYSTEM ===\n%s for minimum level %d opened!\n",
                       d->name, d->min_level);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("\n[!] No suitable dungeons available.\n");
        }
        sem_post(&sys_data->dungeon_sem);
    }
    return NULL;
}

void toggle_notification(int idx) {
    Hunter *h = &sys_data->hunters[idx];
    h->notification_on ^= 1;
    printf("[*] Notifications %s\n", h->notification_on ? "ENABLED" : "DISABLED");

    if (h->notification_on) {
        int *arg = malloc(sizeof(int));
        *arg = idx;
        pthread_create(&h->notif_thread, NULL, notification_thread, arg);
    } else {
        pthread_cancel(h->notif_thread);
    }
}

int login_hunter(const char *name) {
    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            if (sys_data->hunters[i].banned) {
                printf("[!] You are banned!\n");
                sem_post(&sys_data->hunter_sem);
                return -1;
            }
            printf("[+] Welcome back, %s!\n", name);
            sem_post(&sys_data->hunter_sem);
            return i;
        }
    }
    printf("[!] Username not found.\n");
    sem_post(&sys_data->hunter_sem);
    return -1;
}

void show_dungeons(int level) {
    sem_wait(&sys_data->dungeon_sem);
    printf("\n=== AVAILABLE DUNGEONS ===\n");
    int cnt = 0;
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        Dungeon *d = &sys_data->dungeons[i];
        if (d->min_level <= level) {
            printf("%d. %s (Lv %d+) | ATK+%d HP+%d DEF+%d EXP+%d\n",
                   ++cnt, d->name, d->min_level, d->atk, d->hp, d->def, d->exp);
        }
    }
    if (cnt == 0) printf("No dungeons available.\n");
    sem_post(&sys_data->dungeon_sem);
}

void raid_dungeon(int idx) {
    show_dungeons(sys_data->hunters[idx].level);
    printf("Choose dungeon number: ");
    int choice;
    scanf("%d", &choice);

    sem_wait(&sys_data->dungeon_sem);
    int cnt = 0, target = -1;
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        if (sys_data->dungeons[i].min_level <= sys_data->hunters[idx].level) {
            cnt++;
            if (cnt == choice) {
                target = i;
                break;
            }
        }
    }

    if (target == -1) {
        printf("[!] Invalid dungeon choice.\n");
        sem_post(&sys_data->dungeon_sem);
        return;
    }

    Dungeon d = sys_data->dungeons[target];
    for (int i = target; i < sys_data->num_dungeons - 1; i++)
        sys_data->dungeons[i] = sys_data->dungeons[i + 1];
    sys_data->num_dungeons--;

    sem_post(&sys_data->dungeon_sem);

    Hunter *h = &sys_data->hunters[idx];
    h->atk += d.atk; h->hp += d.hp; h->def += d.def; h->exp += d.exp;
    if (h->exp >= EXP_LEVELUP) {
        h->level++;
        h->exp = 0;
        printf("[+] LEVEL UP! You are now level %d\n", h->level);
    }

    printf("[*] Raid success! Gained ATK+%d HP+%d DEF+%d EXP+%d\n",
           d.atk, d.hp, d.def, d.exp);
    press_enter();
}

void battle_menu(int idx) {
    sem_wait(&sys_data->hunter_sem);
    printf("\n=== PVP LIST ===\n");
    int options[MAX_HUNTERS], cnt = 0;

    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (i == idx || sys_data->hunters[i].banned) continue;
        Hunter *op = &sys_data->hunters[i];
        printf("%d. %s - Total Power: %d\n", cnt + 1, op->username,
               op->atk + op->hp + op->def);
        options[cnt++] = i;
    }
    sem_post(&sys_data->hunter_sem);

    if (cnt == 0) {
        printf("No available opponents.\n");
        return;
    }

    printf("Target: ");
    int choice;
    scanf("%d", &choice);
    if (choice < 1 || choice > cnt) {
        printf("Invalid choice.\n");
        return;
    }

    int opp = options[choice - 1];
    Hunter *me = &sys_data->hunters[idx];
    Hunter *enemy = &sys_data->hunters[opp];

    int my_power = me->atk + me->hp + me->def;
    int their_power = enemy->atk + enemy->hp + enemy->def;

    printf("You chose to battle %s\n", enemy->username);
    printf("Your Power: %d\nOpponent's Power: %d\n", my_power, their_power);

    sem_wait(&sys_data->hunter_sem);
    if (my_power >= their_power) {
        me->atk += enemy->atk; me->hp += enemy->hp; me->def += enemy->def;
        printf("Battle won! You acquired %s's stats\n", enemy->username);

        for (int i = opp; i < sys_data->num_hunters - 1; i++)
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

    } else {
        enemy->atk += me->atk; enemy->hp += me->hp; enemy->def += me->def;
        printf("You lost and were removed from the system.\n");

        for (int i = idx; i < sys_data->num_hunters - 1; i++)
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

        sem_post(&sys_data->hunter_sem);
        press_enter();
        exit(0);
    }
    sem_post(&sys_data->hunter_sem);
    press_enter();
}

void hunter_dashboard(int idx) {
    int choice;
    do {
        Hunter *h = &sys_data->hunters[idx];
        printf("\n=== %s's MENU ===\n", h->username);
        printf("1. List Dungeons\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: show_dungeons(h->level); break;
            case 2: raid_dungeon(idx); break;
            case 3: battle_menu(idx); break;
            case 4: toggle_notification(idx); break;
            case 5: return;
            default: printf("Invalid choice\n");
        }
    } while (1);
}

void register_hunter() {
    char name[MAX_NAME];
    printf("Username: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            printf("[!] Username already taken\n");
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }

    if (sys_data->num_hunters >= MAX_HUNTERS) {
        printf("[!] Max hunters reached.\n");
        sem_post(&sys_data->hunter_sem);
        return;
    }

    Hunter *h = &sys_data->hunters[sys_data->num_hunters++];
    strcpy(h->username, name);
    h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;
    h->banned = 0; h->notification_on = 0;
    h->shm_key = ftok("/tmp", name[0]);

    printf("Registration success!\n");
    sem_post(&sys_data->hunter_sem);
}

int main() {
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) {
        perror("[!] System not initialized. Run ./system first.");
        return 1;
    }

    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if ((void *)sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    int choice;
    char username[MAX_NAME];
    do {
        printf("\n=== HUNTER MENU ===\n1. Register\n2. Login\n3. Exit\nChoice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: register_hunter(); break;
            case 2:
                printf("Username: ");
                scanf("%s", username);
                {
                    int id = login_hunter(username);
                    if (id != -1) hunter_dashboard(id);
                }
                break;
            case 3:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid input\n");
        }
    } while (choice != 3);

    shmdt(sys_data);
    return 0;
}

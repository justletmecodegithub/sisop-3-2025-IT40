#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50
#define EXP_TO_LEVEL_UP 500

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
volatile sig_atomic_t shutdown_flag = 0;
pthread_t notif_thread;

void press_enter() {
    printf("\nPress enter to continue...");
    getchar();
    getchar();
}

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

void *notify_thread(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    while (!shutdown_flag) {
        if (sys_data->hunters[idx].notification_on) {
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
                printf("\nNo suitable dungeons available.\n");
            }
            sem_post(&sys_data->dungeon_sem);
        }
        sleep(3);
    }
    return NULL;
}

int login(const char *username) {
    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, username) == 0) {
            if (sys_data->hunters[i].banned) {
                printf("[!] You are banned.\n");
                sem_post(&sys_data->hunter_sem);
                return -1;
            }
            sem_post(&sys_data->hunter_sem);
            printf("[+] Welcome back, %s!\n", username);
            return i;
        }
    }
    sem_post(&sys_data->hunter_sem);
    printf("[!] Username not found.\n");
    return -1;
}

void show_dungeons(int level) {
    sem_wait(&sys_data->dungeon_sem);
    printf("\n=== AVAILABLE DUNGEONS ===\n");
    int count = 0;
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        Dungeon *d = &sys_data->dungeons[i];
        if (d->min_level <= level) {
            printf("%d. %s (Lv %d+) ATK+%d HP+%d DEF+%d EXP+%d\n", 
                   ++count, d->name, d->min_level, d->atk, d->hp, d->def, d->exp);
        }
    }
    if (count == 0) printf("No dungeons available.\n");
    sem_post(&sys_data->dungeon_sem);
}

void raid(int idx) {
    Hunter *h = &sys_data->hunters[idx];
    show_dungeons(h->level);
    printf("Choose dungeon number to raid: ");
    int input, count = 0, selected = -1;
    scanf("%d", &input);

    sem_wait(&sys_data->dungeon_sem);
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        if (sys_data->dungeons[i].min_level <= h->level) {
            count++;
            if (count == input) {
                selected = i;
                break;
            }
        }
    }
    if (selected == -1) {
        printf("[!] Invalid selection.\n");
        sem_post(&sys_data->dungeon_sem);
        return;
    }

    Dungeon d = sys_data->dungeons[selected];
    h->atk += d.atk;
    h->hp += d.hp;
    h->def += d.def;
    h->exp += d.exp;

    if (h->exp >= EXP_TO_LEVEL_UP) {
        h->exp = 0;
        h->level++;
        printf("[+] LEVEL UP! Now Level %d\n", h->level);
    }

    for (int i = selected; i < sys_data->num_dungeons - 1; i++) {
        sys_data->dungeons[i] = sys_data->dungeons[i + 1];
    }
    sys_data->num_dungeons--;

    printf("Raid success!\n");
    printf("Gained: ATK+%d HP+%d DEF+%d EXP+%d\n", d.atk, d.hp, d.def, d.exp);
    sem_post(&sys_data->dungeon_sem);
    press_enter();
}

void battle(int idx) {
    Hunter *me = &sys_data->hunters[idx];
    int list[MAX_HUNTERS], count = 0;
    printf("\n=== PVP LIST ===\n");
    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (i != idx && !sys_data->hunters[i].banned) {
            Hunter *h = &sys_data->hunters[i];
            printf("%s - Total Power: %d\n", h->username, h->atk + h->hp + h->def);
            list[count++] = i;
        }
    }
    sem_post(&sys_data->hunter_sem);
    if (count == 0) {
        printf("No opponents available.\n");
        press_enter();
        return;
    }

    char target[MAX_NAME];
    printf("Target: ");
    scanf("%s", target);

    int opp = -1;
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (i != idx && strcmp(sys_data->hunters[i].username, target) == 0 && !sys_data->hunters[i].banned) {
            opp = i;
            break;
        }
    }

    if (opp == -1) {
        printf("[!] Invalid target.\n");
        press_enter();
        return;
    }

    Hunter *enemy = &sys_data->hunters[opp];
    int p1 = me->atk + me->hp + me->def;
    int p2 = enemy->atk + enemy->hp + enemy->def;

    printf("\nYou chose to battle %s\n", enemy->username);
    printf("Your Power: %d\nOpponent's Power: %d\n", p1, p2);

    if (p1 >= p2) {
        me->atk += enemy->atk;
        me->hp += enemy->hp;
        me->def += enemy->def;
        printf("Deleting defender's shared memory (shmid: %d)\n", enemy->shm_key);

        for (int i = opp; i < sys_data->num_hunters - 1; i++)
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

        printf("Battle won! You acquired %s's stats\n", enemy->username);
    } else {
        enemy->atk += me->atk;
        enemy->hp += me->hp;
        enemy->def += me->def;

        for (int i = idx; i < sys_data->num_hunters - 1; i++)
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

        printf("You lost. You were removed from the system.\n");
        press_enter();
        exit(0);
    }
    press_enter();
}

void toggle_notify(int idx) {
    Hunter *h = &sys_data->hunters[idx];
    h->notification_on = !h->notification_on;
    printf("[*] Notifications %s\n", h->notification_on ? "enabled" : "disabled");
    if (h->notification_on) {
        int *arg = malloc(sizeof(int));
        *arg = idx;
        pthread_create(&notif_thread, NULL, notify_thread, arg);
    } else {
        pthread_cancel(notif_thread);
    }
    press_enter();
}

void dashboard(int idx) {
    int ch;
    do {
        Hunter *h = &sys_data->hunters[idx];
        printf("\n=== %s's MENU ===\n", h->username);
        printf("1. List Dungeon\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ");
        scanf("%d", &ch);
        switch (ch) {
            case 1: show_dungeons(h->level); press_enter(); break;
            case 2: raid(idx); break;
            case 3: battle(idx); break;
            case 4: toggle_notify(idx); break;
            case 5: return;
            default: printf("Invalid choice\n");
        }
    } while (1);
}

int main() {
    signal(SIGINT, SIG_IGN);
    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) {
        printf("[!] ERROR: System not initialized. Run ./system first.\n");
        return 1;
    }

    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if (sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    if (sys_data->system_active != 1) {
        printf("[!] System is inactive.\n");
        return 1;
    }

    int ch;
    char name[MAX_NAME];
    do {
        printf("\n=== HUNTER MENU ===\n1. Register\n2. Login\n3. Exit\nChoice: ");
        scanf("%d", &ch);
        switch (ch) {
            case 1: {
                printf("Username: ");
                scanf("%s", name);
                sem_wait(&sys_data->hunter_sem);
                int exists = 0;
                for (int i = 0; i < sys_data->num_hunters; i++) {
                    if (strcmp(sys_data->hunters[i].username, name) == 0) exists = 1;
                }
                if (exists) printf("[!] Username already taken\n");
                else if (sys_data->num_hunters >= MAX_HUNTERS) printf("[!] Hunter list full\n");
                else {
                    Hunter *h = &sys_data->hunters[sys_data->num_hunters++];
                    strcpy(h->username, name);
                    h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;
                    h->banned = 0; h->notification_on = 0;
                    h->shm_key = ftok("/tmp", name[0]);
                    printf("Registration success!\n");
                }
                sem_post(&sys_data->hunter_sem);
                press_enter();
                break;
            }
            case 2: {
                printf("Username: ");
                scanf("%s", name);
                int id = login(name);
                if (id != -1) dashboard(id);
                break;
            }
            case 3: break;
            default: printf("Invalid choice\n");
        }
    } while (ch != 3);

    shmdt(sys_data);
    return 0;
}

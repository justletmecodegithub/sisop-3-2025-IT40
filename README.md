# Soal Nomor 1
Dikerjakan oleh 5027241024

## Skrip image_server.c
## 1. Library yang Diperlukan
```c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
```
## 3. Konstanta dan Prototipe Fungsi
```c
#define LISTEN_PORT 9090
#define MAX_BUFFER 8192
#define STORAGE_DIR "./data_store/"


void handle_connection(int client_fd);
void process_decryption(int client_fd, const char *hex_input);
void process_download(int client_fd, const char *file_name);
int convert_hex_to_bin(const char *hex_str, unsigned char **bin_data);
```

## 4. Fungsi Utama: main

```c
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);  

    server_fd = socket(AF_INET, SOCK_STREAM, 0);  
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LISTEN_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is operational on port %d\n", LISTEN_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_connection(client_fd);
            close(client_fd);
            exit(0);
        } else if (pid > 0) {
            close(client_fd);
        } else {
            perror("Fork failed");
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
```

## 5. Fungsi handle_connection
Fungsi ini menangani komunikasi dengan klien setelah koneksi diterima.
```c
void handle_connection(int client_fd) {
    char buffer[MAX_BUFFER];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        char *command = strtok(buffer, "::");
        char *argument = strtok(NULL, "::");

        if (command && argument) {
            if (strcmp(command, "DECRYPT") == 0) {
                process_decryption(client_fd, argument);
            } else if (strcmp(command, "DOWNLOAD") == 0) {
                process_download(client_fd, argument);
            } else {
                const char *msg = "Invalid command\n";
                send(client_fd, msg, strlen(msg), 0);
            }
        } else {
            const char *msg = "Malformed request\n";
            send(client_fd, msg, strlen(msg), 0);
        }
    }
}
```
## 6. Fungsi process_decryption
Fungsi ini memproses permintaan dekripsi dan menyimpan hasilnya sebagai file.

```c
void process_decryption(int client_fd, const char *hex_input) {
    unsigned char *binary_data = NULL;
    int binary_length = convert_hex_to_bin(hex_input, &binary_data);

    if (binary_length <= 0) {
        const char *msg = "Decryption failed\n";
        send(client_fd, msg, strlen(msg), 0);
        return;
    }

    time_t current_time = time(NULL);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%ld_output.jpeg", STORAGE_DIR, current_time);

    int file_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
        perror("File creation failed");
        const char *msg = "File saving failed\n";
        send(client_fd, msg, strlen(msg), 0);
        free(binary_data);
        return;
    }

    write(file_fd, binary_data, binary_length);
    close(file_fd);
    free(binary_data);

    const char *msg = "Decryption and saving successful\n";
    send(client_fd, msg, strlen(msg), 0);
}
```
## 7. Fungsi process_download
Fungsi ini menangani permintaan pengunduhan file dari server.
```c
void process_download(int client_fd, const char *file_name) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", STORAGE_DIR, file_name);

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        const char *msg = "Requested file not found\n";
        send(client_fd, msg, strlen(msg), 0);
        return;
    }

    char buffer[MAX_BUFFER];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }

    close(file_fd);
}
```
## 8. Fungsi convert_hex_to_bin
Fungsi ini mengonversi string hex menjadi data biner.
```c
int convert_hex_to_bin(const char *hex_str, unsigned char **bin_data) {
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) return -1;

    size_t bin_len = hex_len / 2;
    *bin_data = malloc(bin_len);
    if (!*bin_data) return -1;

    for (size_t i = 0; i < bin_len; ++i) {
        if (sscanf(hex_str + 2 * i, "%2hhx", &((*bin_data)[i])) != 1) {
            free(*bin_data);
            return -1;
        }
    }

    return bin_len;
}
```

# Soal 4

Dikerjakan oleh I Gede Bagus Saka Sinatrya/5027241088

### Penjelasan system.c dan hunter.c

#### Struktur Awal System.c
```bash
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
    pid_t system_pid;
} SystemData;

SystemData *sys_data = NULL;
int shm_id = -1;

key_t get_system_key() {
    return ftok("/tmp", 'S');
}
```

Fungsi untuk membuat shared memory
```bash
void init_shared_memory() {
    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("[!] shmget failed");
        exit(EXIT_FAILURE);
    }

    sys_data = (SystemData*)shmat(shm_id, NULL, 0);
    if (sys_data == (void*)-1) {
        perror("[!] shmat failed");
        exit(EXIT_FAILURE);
    }

    struct shmid_ds buf;
    shmctl(shm_id, IPC_STAT, &buf);
    if (buf.shm_nattch == 1) {
        memset(sys_data, 0, sizeof(SystemData));
	 sys_data->system_pid = getpid();
        sys_data->system_active = 1;  
        sem_init(&sys_data->hunter_sem, 1, 1);
        sem_init(&sys_data->dungeon_sem, 1, 1);
        sys_data->system_active = 1;
        printf("[System] Initialized shared memory\n");
    }
}
```

Fungsi untuk menampilkan list hunter
```bash
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
```

Fungsi untuk menampilkan list dungeon
```bash
void list_dungeons() {
    sem_wait(&sys_data->dungeon_sem);
    printf("\n=== DUNGEON INFO ===\n");
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        Dungeon *d = &sys_data->dungeons[i];
        printf("%d. %s | Lv %d+ | ATK:%d HP:%d DEF:%d EXP:%d\n",
               i + 1, d->name, d->min_level, d->atk, d->hp, d->def, d->exp);
    }
    sem_post(&sys_data->dungeon_sem);
}
```

Fungsi untuk membuat dungeon
```bash
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
    printf("[+] Generated: %s (Lv %d+)\n", d->name, d->min_level);
    sys_data->num_dungeons++;

    sem_post(&sys_data->dungeon_sem);
}
```

Fungsi untuk ban hunter
```bash
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
    printf("Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk unban hunter
```bash
void unban_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to unban: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            sys_data->hunters[i].banned = 0;
            printf("%s has been unbanned\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk reset hunter
```bash
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
    printf("Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk menghapus shared memory
```bash
void cleanup(int sig) {
    printf("\nShutting down and cleaning up shared memory\n");
    sem_destroy(&sys_data->hunter_sem);
    sem_destroy(&sys_data->dungeon_sem);
    shmdt(sys_data);
    shmctl(shm_id, IPC_RMID, NULL);
    printf("Cleanup complete. Shared memory removed.\n");
    exit(0);
}
```

Fungsi untuk tampilan system
```bash
void system_menu() {
    int choice;
    do {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. Hunter Info\n");
	printf("2. Dungeon Info\n");
	printf("3. Generate Dungeon\n");
        printf("4. Ban Hunter\n");
	printf("5. Unban Hunter\n");
	printf("6. Reset Hunter\n");
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
            case 7: cleanup(SIGINT); break;
            default: printf("Invalid choice\n");
        }
    } while (1);
}
```

Untuk int main()
```bash

int main() {
    srand(time(NULL));
    signal(SIGINT, cleanup);
    init_shared_memory();
    system("clear");
    system_menu();
    return 0;
}
```

#### Struktur Awal Hunter.c
```bash
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
#include <errno.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50
#define EXP_TO_LEVEL_UP 500
#define HEARTBEAT_INTERVAL 2

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
    pid_t system_pid;
} SystemData;

SystemData *sys_data = NULL;
int shm_id = -1;
volatile sig_atomic_t shutdown_flag = 0;
pthread_t notif_thread, heartbeat_thread;

key_t get_system_key() {
    return ftok("/tmp", 'S');
}
```
Mengecek system.c apakah masih aktif atau tidak
```bash
void *heartbeat_checker(void *arg) {
    while (1) {
        sleep(HEARTBEAT_INTERVAL);

        if (sys_data == NULL || sys_data == (void *)-1) {
            fprintf(stderr, "\n[!] ERROR: Shared memory is turned off. Turn off the hunter!\n");
            exit(1); 
        }

        if (sys_data->system_active != 1) {
            fprintf(stderr, "\n[!] System.c is shutting down. Turn of the hunter...\n");
            exit(1);  
        }

        if (kill(sys_data->system_pid, 0) != 0 && errno == ESRCH) {
            fprintf(stderr, "\n[!] System.c is turned off. Shutdown Hunter...\n");
            exit(1);  
        }
    }
    return NULL;
}
```
Membersihkan resource
```bash
void cleanup() {
    if (sys_data != NULL && sys_data != (void *)-1) {
        shmdt(sys_data);
    }
    if (shm_id != -1) {
    }
    exit(0);
}
```
Menghandle sinyal
```bash
void handle_signal(int sig) {
    shutdown_flag = 1;
    cleanup();
}
```


Fungsi untuk login
```bash
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
```

Fungsi untuk melihat dungeon yang tersedia pada setiap akun hunter
```bash
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
```
Fungsi untuk raid
```bash
void raid(int idx) {
    if (shutdown_flag) return;

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
```
Fungsi untuk battle
```bash
void battle(int idx) {
    if (shutdown_flag) return;

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
```

Fungsi untuk notifikasi dan tampilannya
```bash
void toggle_notify(int idx) {
    if (shutdown_flag) return;

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
```

```bash
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
```
Fungsi untuk tampilan setiap akun hunter
```bash
void dashboard(int idx) {
    int ch;
    do{
        if(sys_data == NULL || sys_data->system_active != 1) {
            exit(1);
	}

        Hunter *h = &sys_data->hunters[idx];
        printf("\n=== %s's MENU ===\n", h->username);
        printf("1. List Dungeon\n");
	printf("2. Raid\n");
	printf("3. Battle\n");
	printf("4. Toggle Notification\n");
	printf("5. Exit\n");
	printf("Choice: ");
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
```
Untuk int main()
```bash
int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) {
        printf("[!] Run ./system first.\n");
        return 1;
    }

    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if (sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    if (sys_data->system_active != 1) {
        printf("[!] System is inactive. Please start the system first.\n");
        shmdt(sys_data);
        return 1;
    }

    pthread_create(&heartbeat_thread, NULL, heartbeat_checker, NULL);

    system("clear");
    int ch;
    char name[MAX_NAME];
    do {
        if(sys_data == NULL || sys_data->system_active != 1) {
            exit(1);
        }

        printf("\n=== HUNTER MENU ===\n");
	printf("1. Register\n");
	printf("2. Login\n");
	printf("3. Exit\nChoice: ");
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
    } while (ch != 3 && !shutdown_flag);

    cleanup();
    return 0;
}
```
### Beberapa masalah yang terjadi

![Image](https://github.com/user-attachments/assets/fc8c919e-b798-465b-a66f-48f9ab314840)

![Image](https://github.com/user-attachments/assets/623be93d-16d9-4e44-8a42-662c95f4e32b)

Hunter.c masih bisa berjalan tanpa menjalankan system.c terlebih dahulu.

Shared memory tidak terhapus begitu keluar dari system.
### Revisi

1. Hunter.c masih bisa berjalan tanpa menggunakan system.c, solusinya tambahkan fungsi dan beberapa syarat berikut.
```bash
void *heartbeat_checker(void *arg) {
    while (1) {
        sleep(HEARTBEAT_INTERVAL);

        if (sys_data == NULL || sys_data == (void *)-1) {
            fprintf(stderr, "\n[!] ERROR: Shared memory is turned off. Turn off the hunter!\n");
            exit(1); 
        }

        if (sys_data->system_active != 1) {
            fprintf(stderr, "\n[!] System.c is shutting down. Turn of the hunter...\n");
            exit(1);  
        }

        if (kill(sys_data->system_pid, 0) != 0 && errno == ESRCH) {
            fprintf(stderr, "\n[!] System.c is turned off. Shutdown Hunter...\n");
            exit(1);  
        }
    }
    return NULL;
}
```
Pada int main kita tambahkan pengecekan
```bash
 key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) {
        printf("[!] Run ./system first.\n");
        return 1;
    }

    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if (sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    if (sys_data->system_active != 1) {
        printf("[!] System is inactive. Please start the system first.\n");
        shmdt(sys_data);
        return 1;
    }

    pthread_create(&heartbeat_thread, NULL, heartbeat_checker, NULL);
```


2. Shared memory belum terhapus saat keluar dari system, solusinya menambahkan fungsi untuk menghapus shared memory
```bash
void cleanup(int sig) {
    printf("\nShutting down and cleaning up shared memory\n");
    sem_destroy(&sys_data->hunter_sem);
    sem_destroy(&sys_data->dungeon_sem);
    shmdt(sys_data);
    shmctl(shm_id, IPC_RMID, NULL);
    printf("Cleanup complete. Shared memory removed.\n");
    exit(0);
}
```







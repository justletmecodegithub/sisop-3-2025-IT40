#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];
    int inventory_size;
} Player;

typedef struct {
    int health;
    int max_health;
} Enemy;

Player players[MAX_CLIENTS];
Enemy current_enemies[MAX_CLIENTS];

extern void init_shop();
extern char* get_shop_menu();
extern void buy_weapon(int weapon_id, Player* player, char* result);
extern void get_inventory(Player* player, char* inventory);
extern void equip_weapon(int weapon_id, Player* player, char* result);

void send_battle_prompt(int client_socket, Enemy* enemy) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char prompt[BUFFER_SIZE];
    snprintf(prompt, BUFFER_SIZE, 
        "BATTLE STARTED ===\n"
        "Enemy appeared with:\n"
        "[%s] %d/%d HP\n"
        "Type 'attack' to attack or 'exit' to leave battle.\n"
        "> ",
        health_bar, enemy->health, enemy->max_health);
    
    send(client_socket, prompt, strlen(prompt), 0);
}

void send_battle_result(int client_socket, Enemy* enemy, int damage, int is_critical) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char result[BUFFER_SIZE];
    if (is_critical) {
        snprintf(result, BUFFER_SIZE,
            "== CRITICAL HIT! ==\n"
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    } else {
        snprintf(result, BUFFER_SIZE,
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    }
    
    send(client_socket, result, strlen(result), 0);
}

void handle_battle(int client_socket, int player_id) {
    char buffer[BUFFER_SIZE] = {0};
    
    current_enemies[player_id].max_health = 50 + (rand() % 151);
    current_enemies[player_id].health = current_enemies[player_id].max_health;
    
    send_battle_prompt(client_socket, &current_enemies[player_id]);
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0) {
            if(current_enemies[player_id].health <= 0) {
                send(client_socket, "No enemy to attack!\n> ", 21, 0);
                continue;
            }
            
            int damage = players[player_id].base_damage + (rand() % (players[player_id].base_damage + 1));
            int is_critical = (rand() % 10 == 0);
            if (is_critical) damage *= 2;
            
            current_enemies[player_id].health -= damage;
            
            if(current_enemies[player_id].health <= 0) {
                players[player_id].kills++;
                int reward = 50 + (rand() % 101);
                players[player_id].gold += reward;
                
                char victory_msg[BUFFER_SIZE];
                snprintf(victory_msg, BUFFER_SIZE, 
                    "You defeated the enemy!\n"
                    "=== REWARD ===\n"
                    "You earned %d gold!\n"
                    "=== NEW ENEMY ===\n"
                    "> ",
                    reward);
                send(client_socket, victory_msg, strlen(victory_msg), 0);
                
                current_enemies[player_id].max_health = 50 + (rand() % 151);
                current_enemies[player_id].health = current_enemies[player_id].max_health;
            } else {
                send_battle_result(client_socket, &current_enemies[player_id], damage, is_critical);
            }
        }
        else if(strcmp(buffer, "exit") == 0) {
            send(client_socket, "Exiting battle...\n", 18, 0);
            break;
        }
        else {
            send(client_socket, "Invalid command. Type 'attack' or 'exit'.\n> ", 42, 0);
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int player_id = client_socket % MAX_CLIENTS;
    
    players[player_id].gold = 500;
    strcpy(players[player_id].equipped_weapon, "Fists");
    players[player_id].base_damage = 5;
    players[player_id].kills = 0;
    players[player_id].inventory_size = 0;
    
    init_shop();
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "stats") == 0) {
            char stats[BUFFER_SIZE];
            snprintf(stats, BUFFER_SIZE, 
                "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d",
                players[player_id].gold, 
                players[player_id].equipped_weapon, 
                players[player_id].base_damage, 
                players[player_id].kills);
            send(client_socket, stats, strlen(stats), 0);
        }
        else if(strcmp(buffer, "shop") == 0) {
            send(client_socket, get_shop_menu(), strlen(get_shop_menu()), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char result[BUFFER_SIZE];
            buy_weapon(atoi(buffer), &players[player_id], result);
            send(client_socket, result, strlen(result), 0);
        }
        else if(strcmp(buffer, "inventory") == 0) {
            char inventory[BUFFER_SIZE];
            get_inventory(&players[player_id], inventory);
            send(client_socket, inventory, strlen(inventory), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char equip_result[BUFFER_SIZE];
            equip_weapon(atoi(buffer), &players[player_id], equip_result);
            send(client_socket, equip_result, strlen(equip_result), 0);
        }
        else if(strcmp(buffer, "battle") == 0) {
            handle_battle(client_socket, player_id);
        }
        else if(strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    srand(time(NULL));
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Dungeon server running on port %d\n", PORT);
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        handle_client(new_socket);
    }
    
    return 0;
}

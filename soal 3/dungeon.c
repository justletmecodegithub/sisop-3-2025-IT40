#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

char* get_shop_menu();
void buy_weapon(int weapon_id, void* player, char* result);
void get_inventory(void* player, char* inventory);
void equip_weapon(int weapon_id, void* player, char* result);

typedef struct {
    int gold;
    char equipped_weapon[50];
    int base_damage;
    int kills;
    int weapon_id;
    int inventory[10];
    int inventory_size;
} Player;

typedef struct {
    int health;
    int max_health;
} Enemy;

Player players[MAX_CLIENTS];
Enemy current_enemies[MAX_CLIENTS];

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int player_id = -1;
    
    player_id = client_socket % MAX_CLIENTS;
    players[player_id].gold = 500;
    strcpy(players[player_id].equipped_weapon, "Fists");
    players[player_id].base_damage = 5;
    players[player_id].kills = 0;
    players[player_id].weapon_id = 0;
    players[player_id].inventory_size = 0;
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        read(client_socket, buffer, BUFFER_SIZE);
        
        if(strcmp(buffer, "stats") == 0) {
            char stats[BUFFER_SIZE];
            snprintf(stats, BUFFER_SIZE, "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d", 
                    players[player_id].gold, 
                    players[player_id].equipped_weapon, 
                    players[player_id].base_damage, 
                    players[player_id].kills);
            send(client_socket, stats, strlen(stats), 0);
        }
        else if(strcmp(buffer, "shop") == 0) {
            send(client_socket, get_shop_menu(), strlen(get_shop_menu()), 0);
        }
        else if(strncmp(buffer, "buy ", 4) == 0) {
            int weapon_id = atoi(buffer + 4);
            char result[BUFFER_SIZE];
            buy_weapon(weapon_id, &players[player_id], result);
            send(client_socket, result, strlen(result), 0);
        }
        else if(strcmp(buffer, "inventory") == 0) {
            char inventory[BUFFER_SIZE];
            get_inventory(&players[player_id], inventory);
            send(client_socket, inventory, strlen(inventory), 0);
        }
        else if(strncmp(buffer, "equip ", 6) == 0) {
            int weapon_id = atoi(buffer + 6);
            char result[BUFFER_SIZE];
            equip_weapon(weapon_id, &players[player_id], result);
            send(client_socket, result, strlen(result), 0);
        }
        else if(strcmp(buffer, "battle") == 0) {
            // Initialize enemy
            current_enemies[player_id].max_health = 50 + (rand() % 151);
            current_enemies[player_id].health = current_enemies[player_id].max_health;
            
            char battle_start[BUFFER_SIZE];
            snprintf(battle_start, BUFFER_SIZE, "Enemy appeared with:\n[ ] %d/%d HP\nType 'attack' to attack or 'exit' to leave battle.", 
                    current_enemies[player_id].health, current_enemies[player_id].max_health);
            send(client_socket, battle_start, strlen(battle_start), 0);
        }
        else if(strcmp(buffer, "attack") == 0) {
            if(current_enemies[player_id].health <= 0) {
                send(client_socket, "No enemy to attack!", 19, 0);
                continue;
            }
            
            // Calculate damage
            int damage = players[player_id].base_damage + (rand() % (players[player_id].base_damage / 2 + 1));
            
            if(rand() % 10 == 0) {
                damage *= 2;
                char crit_msg[BUFFER_SIZE];
                snprintf(crit_msg, BUFFER_SIZE, "== CRITICAL HIT! ==\nYou dealt %d damage!", damage);
                send(client_socket, crit_msg, strlen(crit_msg), 0);
            } else {
                char attack_msg[BUFFER_SIZE];
                snprintf(attack_msg, BUFFER_SIZE, "You dealt %d damage!", damage);
                send(client_socket, attack_msg, strlen(attack_msg), 0);
            }
            
            current_enemies[player_id].health -= damage;
            
            if(current_enemies[player_id].health <= 0) {
                players[player_id].kills++;
                int reward = 50 + (rand() % 151);
                players[player_id].gold += reward;
                
                char victory_msg[BUFFER_SIZE];
                snprintf(victory_msg, BUFFER_SIZE, "You defeated the enemy!\n=== REWARD ===\nYou earned %d gold!", reward);
                send(client_socket, victory_msg, strlen(victory_msg), 0);
                
                current_enemies[player_id].health = 0;
            } else {
                char enemy_status[BUFFER_SIZE];
                snprintf(enemy_status, BUFFER_SIZE, "=== ENEMY STATUS ===\nEnemy health: [ ] %d/%d HP", 
                        current_enemies[player_id].health, current_enemies[player_id].max_health);
                send(client_socket, enemy_status, strlen(enemy_status), 0);
            }
        }
        else if(strcmp(buffer, "exit") == 0) {
            send(client_socket, "Exiting game...", 16, 0);
            break;
        }
        else {
            send(client_socket, "Invalid command", 15, 0);
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

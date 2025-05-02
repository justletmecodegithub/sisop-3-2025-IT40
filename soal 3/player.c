#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void print_menu() {
    printf("\n==== MAIN MENU ====\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapons)\n");
    printf("3. View Inventory & Equip Weapons\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("Choose an option: ");
}

void battle_mode(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    
   
    send(sock, "battle", 6, 0);
    read(sock, buffer, BUFFER_SIZE);
    printf("\n%s\n", buffer);
    
    while(1) {
        printf("\n> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0) {
            send(sock, "attack", 6, 0);
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            printf("\n%s\n", buffer);
            
           
            if(strstr(buffer, "You defeated") != NULL) {
                break;
            }
            
          
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            printf("\n%s\n", buffer);
        }
        else if(strcmp(buffer, "exit") == 0) {
            send(sock, "exit", 4, 0);
            break;
        }
        else {
            printf("Invalid command. Type 'attack' or 'exit'\n");
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to dungeon server\n");
    
    int choice;
    while(1) {
        print_menu();
        scanf("%d", &choice);
        getchar();
        
        switch(choice) {
            case 1:
                send(sock, "stats", 5, 0);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n=== PLAYER STATS ===\n%s\n", buffer);
                break;
                
            case 2: {
                send(sock, "shop", 4, 0);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s\n", buffer);
                
                printf("Enter weapon number to buy (0 to cancel): ");
                int weapon;
                scanf("%d", &weapon);
                getchar();
                
                if(weapon != 0) {
                    char buy_cmd[10];
                    snprintf(buy_cmd, sizeof(buy_cmd), "buy %d", weapon);
                    send(sock, buy_cmd, strlen(buy_cmd), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(sock, buffer, BUFFER_SIZE);
                    printf("\n%s\n", buffer);
                }
                break;
            }
                
            case 3: {
                send(sock, "inventory", 9, 0);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n=== YOUR INVENTORY ===\n%s\n", buffer);
                
                printf("Enter weapon number to equip (0 to cancel): ");
                int weapon;
                scanf("%d", &weapon);
                getchar();
                
                if(weapon != 0) {
                    char equip_cmd[10];
                    snprintf(equip_cmd, sizeof(equip_cmd), "equip %d", weapon);
                    send(sock, equip_cmd, strlen(equip_cmd), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(sock, buffer, BUFFER_SIZE);
                    printf("\n%s\n", buffer);
                }
                break;
            }
                
            case 4:
                battle_mode(sock);
                break;
                
            case 5:
                send(sock, "exit", 4, 0);
                close(sock);
                return 0;
                
            default:
                printf("Invalid option. Please try again.\n");
        }
    }
    
    return 0;
}

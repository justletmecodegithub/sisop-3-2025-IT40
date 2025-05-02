#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[100];
} Weapon;

Weapon weapons[] = {
    {1, "Terra Blade", 50, 10, ""},
    {2, "Flint & Steel", 150, 25, ""},
    {3, "Kitchen Knife", 200, 35, ""},
    {4, "Staff of Light", 120, 20, "10% Insta-Kill Chance"},
    {5, "Dragon Claws", 300, 50, "30% Crit Chance"}
};

int num_weapons = sizeof(weapons) / sizeof(weapons[0]);

char* get_shop_menu() {
    static char menu[1000];
    strcpy(menu, "=== WEAPON SHOP ===\n");
    
    for(int i = 0; i < num_weapons; i++) {
        char weapon_info[200];
        if(strlen(weapons[i].passive) > 0) {
            snprintf(weapon_info, sizeof(weapon_info), 
                    "[%d] %s - Price: %d gold, Damage: %d (Passive: %s)\n", 
                    weapons[i].id, weapons[i].name, weapons[i].price, 
                    weapons[i].damage, weapons[i].passive);
        } else {
            snprintf(weapon_info, sizeof(weapon_info), 
                    "[%d] %s - Price: %d gold, Damage: %d\n", 
                    weapons[i].id, weapons[i].name, weapons[i].price, weapons[i].damage);
        }
        strcat(menu, weapon_info);
    }
    
    return menu;
}

void buy_weapon(int weapon_id, void* player_ptr, char* result) {
    typedef struct {
        int gold;
        char equipped_weapon[50];
        int base_damage;
        int kills;
        int weapon_id;
        int inventory[10];
        int inventory_size;
    } Player;
    
    Player* player = (Player*)player_ptr;
    
    if(weapon_id < 1 || weapon_id > num_weapons) {
        strcpy(result, "Invalid weapon ID");
        return;
    }
    
    Weapon* weapon = &weapons[weapon_id - 1];
    
    if(player->gold < weapon->price) {
        snprintf(result, 100, "Not enough gold! You need %d more gold.", weapon->price - player->gold);
        return;
    }
    
    for(int i = 0; i < player->inventory_size; i++) {
        if(player->inventory[i] == weapon_id) {
            strcpy(result, "You already own this weapon!");
            return;
        }
    }
    
    if(player->inventory_size < 10) {
        player->inventory[player->inventory_size++] = weapon_id;
        player->gold -= weapon->price;
        snprintf(result, 100, "You bought the %s!", weapon->name);
    } else {
        strcpy(result, "Your inventory is full!");
    }
}

void get_inventory(void* player_ptr, char* inventory) {
    typedef struct {
        int gold;
        char equipped_weapon[50];
        int base_damage;
        int kills;
        int weapon_id;
        int inventory[10];
        int inventory_size;
    } Player;
    
    Player* player = (Player*)player_ptr;
    
    strcpy(inventory, "[0] Fists");
    
    for(int i = 0; i < player->inventory_size; i++) {
        int weapon_id = player->inventory[i];
        Weapon* weapon = &weapons[weapon_id - 1];
        
        char item[200];
        if(strlen(weapon->passive) > 0) {
            snprintf(item, sizeof(item), "\n[%d] %s (Passive: %s)", 
                    weapon_id, weapon->name, weapon->passive);
        } else {
            snprintf(item, sizeof(item), "\n[%d] %s", weapon_id, weapon->name);
        }
        
        strcat(inventory, item);
        
        if(player->weapon_id == weapon_id) {
            strcat(inventory, " (EQUIPPED)");
        }
    }
}

void equip_weapon(int weapon_id, void* player_ptr, char* result) {
    typedef struct {
        int gold;
        char equipped_weapon[50];
        int base_damage;
        int kills;
        int weapon_id;
        int inventory[10];
        int inventory_size;
    } Player;
    
    Player* player = (Player*)player_ptr;
    
    if(weapon_id == 0) {
        strcpy(player->equipped_weapon, "Fists");
        player->base_damage = 5;
        player->weapon_id = 0;
        strcpy(result, "Equipped Fists");
        return;
    }
    
    int found = 0;
    for(int i = 0; i < player->inventory_size; i++) {
        if(player->inventory[i] == weapon_id) {
            found = 1;
            break;
        }
    }
    
    if(!found) {
        strcpy(result, "You don't own this weapon!");
        return;
    }
    
    Weapon* weapon = &weapons[weapon_id - 1];
    strcpy(player->equipped_weapon, weapon->name);
    player->base_damage = weapon->damage;
    player->weapon_id = weapon_id;
    
    snprintf(result, 100, "Equipped %s", weapon->name);
}

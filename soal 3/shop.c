#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WEAPONS 5
#define MAX_INVENTORY 10

typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];
    int inventory_size;
} Player;

typedef struct {
    char name[50];
    int price;
    int damage;
    char passive[100];
} Weapon;

Weapon shop_items[MAX_WEAPONS];

void init_shop() {
    strcpy(shop_items[0].name, "Terra Blade");
    shop_items[0].price = 50;
    shop_items[0].damage = 10;
    strcpy(shop_items[0].passive, "None");
    
    strcpy(shop_items[1].name, "Flint & Steel");
    shop_items[1].price = 150;
    shop_items[1].damage = 25;
    strcpy(shop_items[1].passive, "None");
    
    strcpy(shop_items[2].name, "Kitchen Knife");
    shop_items[2].price = 200;
    shop_items[2].damage = 35;
    strcpy(shop_items[2].passive, "None");
    
    strcpy(shop_items[3].name, "Staff of Light");
    shop_items[3].price = 120;
    shop_items[3].damage = 20;
    strcpy(shop_items[3].passive, "10% Insta-Kill Chance");
    
    strcpy(shop_items[4].name, "Dragon Claws");
    shop_items[4].price = 300;
    shop_items[4].damage = 50;
    strcpy(shop_items[4].passive, "30% Crit Chance");
}

char* get_shop_menu() {
    static char menu[1024];
    snprintf(menu, sizeof(menu),
        "\n=== WEAPON SHOP ===\n"
        "[1] %s - Price: %d gold, Damage: %d\n"
        "[2] %s - Price: %d gold, Damage: %d\n"
        "[3] %s - Price: %d gold, Damage: %d\n"
        "[4] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "[5] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "Enter weapon number to buy (0 to cancel): ",
        shop_items[0].name, shop_items[0].price, shop_items[0].damage,
        shop_items[1].name, shop_items[1].price, shop_items[1].damage,
        shop_items[2].name, shop_items[2].price, shop_items[2].damage,
        shop_items[3].name, shop_items[3].price, shop_items[3].damage, shop_items[3].passive,
        shop_items[4].name, shop_items[4].price, shop_items[4].damage, shop_items[4].passive);
    return menu;
}

void buy_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > MAX_WEAPONS) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    
    if (player->gold < shop_items[weapon_id].price) {
        snprintf(result, 1024, "Not enough gold!");
        return;
    }
    
    if (player->inventory_size >= MAX_INVENTORY) {
        snprintf(result, 1024, "Inventory full!");
        return;
    }
    
    player->inventory[player->inventory_size] = weapon_id;
    player->inventory_size++;
    player->gold -= shop_items[weapon_id].price;
    
    snprintf(result, 1024, "Purchase successful!");
}

void get_inventory(Player* player, char* inventory) {
    char temp[1024] = "\n=== YOUR INVENTORY ===\n";
    
    for (int i = 0; i < player->inventory_size; i++) {
        int weapon_id = player->inventory[i];
        char item[256];
        if (strcmp(shop_items[weapon_id].passive, "None") == 0) {
            snprintf(item, sizeof(item), "[%d] %s\n", i+1, shop_items[weapon_id].name);
        } else {
            snprintf(item, sizeof(item), "[%d] %s (Passive: %s)\n", i+1, 
                    shop_items[weapon_id].name, shop_items[weapon_id].passive);
        }
        strcat(temp, item);
    }
    
    strcat(temp, "Enter weapon number to equip (0 to cancel): ");
    strcpy(inventory, temp);
}

void equip_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > player->inventory_size) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    int actual_weapon_id = player->inventory[weapon_id];
    
    strcpy(player->equipped_weapon, shop_items[actual_weapon_id].name);
    player->base_damage = shop_items[actual_weapon_id].damage;
    
    snprintf(result, 1024, "Equipped %s!", shop_items[actual_weapon_id].name);
}

#ifndef MAP_H
#define MAP_H

#include "config.h"
#include <time.h>

#define MAP_WIDTH 19
#define MAP_HEIGHT 10

#define MAP_COLOR_DEFAULT 0
#define MAP_COLOR_WALLS 1
#define MAP_COLOR_TRASH 2
#define MAP_COLOR_PLATES_1 3
#define MAP_COLOR_PLATES_2 4
#define MAP_COLOR_OVEN 5

#define ITEM_COLOR_BREAD 10
#define ITEM_COLOR_HAMBURGER 11
#define ITEM_COLOR_HAMBURGER_BURNED 12
#define ITEM_COLOR_HAMBURGER_READY 13
#define ITEM_COLOR_SALAD 14
#define ITEM_COLOR_JUICE 15
#define ITEM_COLOR_FRIES 16
#define ITEM_COLOR_FRIES_BURNED 17
#define ITEM_COLOR_FRIES_READY 18

#define NUMBER_COLOR_DEFAULT 20
#define NUMBER_COLOR_WARNING 21
#define NUMBER_COLOR_EMERGENCY 22

#define PLAYERS_COLOR_CUSTOMER 30
#define PLAYERS_COLOR_P1 31
#define PLAYERS_COLOR_P2 32
#define PLAYERS_COLOR_P3 33
#define PLAYERS_COLOR_P4 34

#define ITEM_COLOR_BURGER_BREAD 40
#define ITEM_COLOR_SALAD_BREAD 41
#define ITEM_COLOR_FULL_BURGER 42
#define ITEM_COLOR_SALAD_BURGER 43

#define MAX_APPLIANCES 20

// Enum of item types and their rendered characters
enum Item_type {
    NONE,
    BREAD,
    SALAD,
    JUICE,

    HAMBURGER,
    HAMBURGER_BURNED,
    HAMBURGER_READY,

    FRIES,
    FRIES_READY,
    FRIES_BURNED,

    BURGER_BREAD,
    SALAD_BREAD,
    SALAD_BURGER,
    FULL_BURGER,
};

enum Cook_status {
    EMPTY = 'E',   // Empty oven (player can start oven)
    COOKING = 'C', // Cooking food (cannot get item)
    READY = 'R',   // Food ready (player can get item)
    BURNED = 'B',  // Food burned (player can get burneed item)
};

enum Appliance_type {
    APP_OVEN = 1,
    APP_FRYER = 2,
};

typedef struct {
    int x, y; // Posição visual (onde muda o caracter)
    int timer_x, timer_y;
    int time_left;
    enum Appliance_type type;
    enum Cook_status state;
    time_t start_time;
    enum Item_type content;
} APPLIANCE;

typedef struct {
    int x, y; // Posição visual (onde muda o caracter)
    enum Item_type content;
} COUNTER;

#define MAX_COUNTERS 32

// CLIENT
extern char map_players_char[MAX_PLAYERS];

// Game map
extern char map[MAP_HEIGHT][MAP_WIDTH];

// Map with the attr of each char
extern int attr_map[MAP_HEIGHT][MAP_WIDTH];

// Map with the color codes of each char
extern int color_map[MAP_HEIGHT][MAP_WIDTH];

// SERVER
// Game map with blocked spaces
extern int game_map[MAP_HEIGHT][MAP_WIDTH];

// Game map with itens
extern enum Item_type item_map[MAP_HEIGHT][MAP_WIDTH];

// Game map with trash position
extern int trash_map[MAP_HEIGHT][MAP_WIDTH];

extern int appliance_interaction_map[MAP_HEIGHT][MAP_WIDTH];

int init_appliances(APPLIANCE appliances[MAX_APPLIANCES]);
int get_appliance_id_at(int x, int y);

extern int counter_interaction_map[MAP_HEIGHT][MAP_WIDTH];

int init_counters(COUNTER counters[MAX_COUNTERS]);
int get_counter_id_at(int x, int y);

enum Item_type try_combine(enum Item_type item1, enum Item_type item2);

#endif

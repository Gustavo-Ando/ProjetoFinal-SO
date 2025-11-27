#include "map.h"
#include "message.h"

#include <curses.h>
#include <stdlib.h>

/*
    --- Client ---
*/

// Array of chars for each player to be rendered as
char map_players_char[MAX_PLAYERS] = {'a', 'b', 'c', 'd'};

// Game map to render
char map[MAP_HEIGHT][MAP_WIDTH] = {
    "########n#n########      ",
    "##x|  \\ O O /(- @)#      ",
    "## |              #      ",
    "## |             []      ",
    "## |             []      ",
    "## |             []      ",
    "#                []      ",
    "#                 #      ",
    "#(U W)/\"0\"0\"\\  (=)#      ",
    "########n#n########      "};

// Map with the attr of each char
int attr_map[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
// Map with the color codes of each char
int color_map[MAP_HEIGHT][MAP_WIDTH] = {
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_TRASH, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_DEFAULT, ITEM_COLOR_HAMBURGER, MAP_COLOR_DEFAULT, ITEM_COLOR_SALAD, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_1, MAP_COLOR_PLATES_1, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_2, MAP_COLOR_PLATES_2, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_1, MAP_COLOR_PLATES_1, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_2, MAP_COLOR_PLATES_2, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, ITEM_COLOR_JUICE, MAP_COLOR_DEFAULT, ITEM_COLOR_FRIES, MAP_COLOR_DEFAULT, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, ITEM_COLOR_BREAD, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT},
};

/*
    --- Server ---
*/

// Game map with blocked spaces
int game_map[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

// Game map with itens a player can get in each location
enum Item_type item_map[MAP_HEIGHT][MAP_WIDTH] = {
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, HAMBURGER, NONE, SALAD, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, JUICE, NONE, FRIES, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, BREAD, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
};

// Game map with positions a player can use the trash
int trash_map[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

int appliance_interaction_map[MAP_HEIGHT][MAP_WIDTH];

int init_appliances(APPLIANCE appliances[MAX_APPLIANCES]) {
    int num_appliances = 0;

    // Initialize everything with -1
    for (int i = 0; i < MAP_HEIGHT; i++)
        for (int j = 0; j < MAP_WIDTH; j++)
            appliance_interaction_map[i][j] = -1;

    // Search the appliances in the map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = map[y][x];
            
            // If oven or fryer
            if (c == 'O' || c == '0') {
                // Initialize it
                appliances[num_appliances].x = x;
                appliances[num_appliances].y = y;
                appliances[num_appliances].state = EMPTY;
                appliances[num_appliances].start_time = 0;
                appliances[num_appliances].content = NONE;
                appliances[num_appliances].time_left = 0;

                // Search the timer
                // Above
                if (y > 0 && map[y - 1][x] == 'n') {
                    appliances[num_appliances].timer_x = x;
                    appliances[num_appliances].timer_y = y - 1;
                } else if (y < MAP_HEIGHT - 1 && map[y + 1][x] == 'n') { // Underneath
                    appliances[num_appliances].timer_x = x;
                    appliances[num_appliances].timer_y = y + 1;
                } else { // Or if doesn't find it, -1
                    appliances[num_appliances].timer_x = -1;
                    appliances[num_appliances].timer_y = -1;
                }

                // Define it's type
                if (c == 'O') appliances[num_appliances].type = APP_OVEN;
                else appliances[num_appliances].type = APP_FRYER;

                // Define interaction area
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};

                for (int k = 0; k < 4; k++) {
                    int nx = x + dx[k];
                    int ny = y + dy[k];

                    // If inside the map and not a wall
                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                        if (game_map[ny][nx] == 0) {
                            appliance_interaction_map[ny][nx] = num_appliances; // Set the appliance id
                        }
                    }
                }
                num_appliances++;
            } 
        }
    }
    return num_appliances;
}

int get_appliance_id_at(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return -1;
    return appliance_interaction_map[y][x];
}

int counter_interaction_map[MAP_HEIGHT][MAP_WIDTH];

int init_counters(COUNTER counters[MAX_COUNTERS]) {
    int num_counters = 0;

    // Initialize everything with -1
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            counter_interaction_map[y][x] = -1;

    // Visual map scan to find counters
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = map[y][x];

            if (c == '|')  {
                counters[num_counters].x = x - 1;
                counters[num_counters].y = y;
                counters[num_counters].content = NONE;

                // Defines where the player can interact (adjacent cells)
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};

                for (int k = 0; k < 4; k++) {
                    int nx = x + dx[k];
                    int ny = y + dy[k];

                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                        if (game_map[ny][nx] == 0) {
                            counter_interaction_map[ny][nx] = num_counters;
                        }
                    }
                }

                num_counters++;
            }
        }
    }
    return num_counters;
}

int get_counter_id_at(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return -1;

    return counter_interaction_map[y][x];
}

int customer_interaction_map[MAP_HEIGHT][MAP_WIDTH];

int init_customers(CUSTOMER customers[MAX_CUSTOMERS]) {
    int num_customers = 0;

    // Initialize everything with -1
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            customer_interaction_map[y][x] = -1;

    // Visual map scan to find customers
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = map[y][x];

            if (c == '[')  {
                customers[num_customers].x = x;
                customers[num_customers].y = y;
                customers[num_customers].order_size = 0;
                customers[num_customers].active = 0;
                customers[num_customers].delivered = 0;
                customers[num_customers].time_left = 0;
                for (int k = 0; k < MAX_ORDER; ++k) {
                    customers[num_customers].order[k] = NONE;
                }

                // Defines where the player can interact (adjacent cells)
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};

                for (int k = 0; k < 4; k++) {
                    int nx = x + dx[k];
                    int ny = y + dy[k];

                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                        if (game_map[ny][nx] == 0) {
                            customer_interaction_map[ny][nx] = num_customers;
                        }
                    }
                }

                num_customers++;
            }
        }
    }
    return num_customers;
}


int get_customer_id_at(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) 
        return -1;
    int id = customer_interaction_map[y][x];
    if (id < 0 || id >= MAX_CUSTOMERS) 
        return -1;
    return id;
}



enum Item_type try_combine(enum Item_type a, enum Item_type b)
{
    // bread + hamburger ready
    if ((a == BREAD && b == HAMBURGER_READY) ||
        (b == BREAD && a == HAMBURGER_READY))
        return BURGER_BREAD;

    // bread + salad
    if ((a == BREAD && b == SALAD) ||
        (b == BREAD && a == SALAD))
        return SALAD_BREAD;

    // salad + hamburger ready
    if ((a == SALAD && b == HAMBURGER_READY) ||
        (b == SALAD && a == HAMBURGER_READY))
        return SALAD_BURGER;

    // bread + hamburger ready + salad (ex: combine burger_bread + salad or  salad_bread + hamburger_ready or salad_burger + bread)
    if ((a == BURGER_BREAD && b == SALAD) || (a == SALAD && b == BURGER_BREAD) ||
        (a == SALAD_BREAD && b == HAMBURGER_READY) || (a == HAMBURGER_READY && b == SALAD_BREAD) ||
        (a == SALAD_BURGER && b == BREAD) || (a == BREAD && b == SALAD_BURGER))
        return FULL_BURGER;

    return NONE; // Cannot combine
}

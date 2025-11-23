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
    "########n#n########",
    "##x|  \\ O O /(- @)#",
    "## |              #",
    "## |             []",
    "## |             []",
    "## |             []",
    "#                []",
    "#                 #",
    "#(U W)/\"0\"0\"\\  (=)#",
    "########n#n########"};

// Map with the attr of each char
int attr_map[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, A_UNDERLINE, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, A_UNDERLINE, A_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
// Map with the color codes of each char
int color_map[MAP_HEIGHT][MAP_WIDTH] = {
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_TRASH, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_DEFAULT, ITEM_COLOR_HAMBURGER, MAP_COLOR_DEFAULT, ITEM_COLOR_SALAD, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_1, MAP_COLOR_PLATES_1},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_2, MAP_COLOR_PLATES_2},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_1, MAP_COLOR_PLATES_1},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_PLATES_2, MAP_COLOR_PLATES_2},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS},
    {MAP_COLOR_WALLS, MAP_COLOR_DEFAULT, ITEM_COLOR_JUICE, MAP_COLOR_DEFAULT, ITEM_COLOR_FRIES, MAP_COLOR_DEFAULT, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_OVEN, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, MAP_COLOR_DEFAULT, ITEM_COLOR_BREAD, MAP_COLOR_DEFAULT, MAP_COLOR_WALLS},
    {MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, NUMBER_COLOR_DEFAULT, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS, MAP_COLOR_WALLS},
};

/*
    --- Server ---
*/

// Game map with blocked spaces
int game_map[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

// Game map with itens a player can get in each location
enum Item_type item_map[MAP_HEIGHT][MAP_WIDTH] = {
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, HAMBURGER, NONE, SALAD, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, JUICE, NONE, FRIES, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, BREAD, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
    {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
};

// Game map with positions a player can use the trash
int trash_map[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

Appliance appliances[MAX_APPLIANCES];
int num_appliances = 0;
int appliance_interaction_map[MAP_HEIGHT][MAP_WIDTH];

void init_appliances()
{
    num_appliances = 0;

    // Limpa o mapa de interação (-1 em tudo)
    for (int i = 0; i < MAP_HEIGHT; i++)
        for (int j = 0; j < MAP_WIDTH; j++)
            appliance_interaction_map[i][j] = -1;

    // Varre o mapa visual para achar as máquinas
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            char c = map[y][x];

            if (c == 'O' || c == '0')
            {
                appliances[num_appliances].x = x;
                appliances[num_appliances].y = y;
                appliances[num_appliances].state = COOK_OFF;
                appliances[num_appliances].start_time = 0;
                appliances[num_appliances].content = NONE;

                if (c == 'O')
                    appliances[num_appliances].type = APP_OVEN;
                else
                    appliances[num_appliances].type = APP_FRYER;

                // Define onde o player interage (olha em volta)
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};

                for (int k = 0; k < 4; k++)
                {
                    int nx = x + dx[k];
                    int ny = y + dy[k];

                    // Se dentro do mapa e nao for parede (assumindo 0 = chão)
                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT)
                    {
                        if (game_map[ny][nx] == 0)
                        {
                            appliance_interaction_map[ny][nx] = num_appliances; //?
                        }
                    }
                }
                num_appliances++;
            }
        }
    }
}

int get_appliance_id_at(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return -1;
    return appliance_interaction_map[y][x];
}

Counter counters[MAX_COUNTERS];
int num_counters = 0;
int counter_interaction_map[MAP_HEIGHT][MAP_WIDTH];

void init_counters()
{
    num_counters = 0;

    // Limpa o mapa de interação
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            counter_interaction_map[y][x] = -1;

    // Varre o mapa visual para achar os counters
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            char c = map[y][x];

            if (c == '|') //
            {
                counters[num_counters].x = x;
                counters[num_counters].y = y;
                counters[num_counters].content = NONE;

                // Define onde o player pode interagir (células adjacentes)
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};

                for (int k = 0; k < 4; k++)
                {
                    int nx = x + dx[k];
                    int ny = y + dy[k];

                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT)
                    {
                        if (game_map[ny][nx] == 0)
                        {
                            counter_interaction_map[ny][nx] = num_counters;
                        }
                    }
                }

                num_counters++;
            }
        }
    }
}

int get_counter_id_at(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return -1;

    return counter_interaction_map[y][x];
}

#include "render.h"
#include "message.h"
#include "client.h"
#include "map.h"

#include <curses.h>
#include <pthread.h>

/*
    Function to initialize color pairs to render in terminal
    Params:
        -
    Return:
        -
*/
void color_config() {
    if (!has_colors()) return;

    // Start color and use default terminal colors
    start_color();
    use_default_colors();

    // Buildings
    init_pair(MAP_COLOR_DEFAULT, COLOR_WHITE, -1);          // Default
    init_pair(MAP_COLOR_WALLS, COLOR_WHITE, COLOR_WHITE);   // Walls
    init_pair(MAP_COLOR_TRASH, COLOR_RED, -1);              // Trash
    init_pair(MAP_COLOR_PLATES_1, COLOR_WHITE, COLOR_BLUE); // Plates 1
    init_pair(MAP_COLOR_PLATES_2, COLOR_WHITE, COLOR_CYAN); // Plates 2
    init_pair(MAP_COLOR_OVEN, COLOR_RED, -1);               // Oven

    // Itens
    init_pair(ITEM_COLOR_BREAD, COLOR_YELLOW, -1);           // Bread
    init_pair(ITEM_COLOR_HAMBURGER, COLOR_WHITE, -1);        // Hamburger
    init_pair(ITEM_COLOR_HAMBURGER_BURNED, COLOR_BLACK, -1); // Hamburger Burned
    init_pair(ITEM_COLOR_HAMBURGER_READY, COLOR_RED, -1);    // Hamburger Ready
    init_pair(ITEM_COLOR_SALAD, COLOR_GREEN, -1);            // Salad
    init_pair(ITEM_COLOR_JUICE, COLOR_CYAN, -1);             // Juice
    init_pair(ITEM_COLOR_FRIES, COLOR_WHITE, -1);            // French Fries
    init_pair(ITEM_COLOR_FRIES_BURNED, COLOR_BLACK, -1);     // French Fries Burned
    init_pair(ITEM_COLOR_FRIES_READY, COLOR_YELLOW, -1);     // French Fries Ready

    init_pair(ITEM_COLOR_BURGER_BREAD, COLOR_RED, -1);   // Burger with Bread
    init_pair(ITEM_COLOR_SALAD_BREAD, COLOR_GREEN, -1);  // Salad with Bread
    init_pair(ITEM_COLOR_FULL_BURGER, COLOR_RED, -1);    // Full Burger
    init_pair(ITEM_COLOR_SALAD_BURGER, COLOR_GREEN, -1); // Salad Burger

    // Numbers
    init_pair(NUMBER_COLOR_DEFAULT, COLOR_BLACK, COLOR_WHITE);  // Default
    init_pair(NUMBER_COLOR_WARNING, COLOR_YELLOW, COLOR_WHITE); // Warning
    init_pair(NUMBER_COLOR_EMERGENCY, COLOR_RED, COLOR_WHITE);  // Emergency

    // Players/Customers
    init_pair(PLAYERS_COLOR_CUSTOMER, COLOR_WHITE, -1); // Customers
    init_pair(PLAYERS_COLOR_P1, COLOR_RED, -1);         // P1
    init_pair(PLAYERS_COLOR_P2, COLOR_BLUE, -1);        // P2
    init_pair(PLAYERS_COLOR_P3, COLOR_GREEN, -1);       // P3
    init_pair(PLAYERS_COLOR_P4, COLOR_YELLOW, -1);      // P4
}

char get_item_char(enum Item_type item) {
    switch (item) {
        case BREAD: return '=';
        case SALAD: return '@';
        case JUICE: return 'U';
    
        case HAMBURGER: return '-';
        case HAMBURGER_READY: return '-';
        case HAMBURGER_BURNED: return '~';
    
        case FRIES: return 'W';
        case FRIES_READY: return 'W';
        case FRIES_BURNED: return 'W';
    
        case BURGER_BREAD: return '=';
        case SALAD_BREAD: return '=';
        case SALAD_BURGER: return '*';
        case FULL_BURGER: return '#';
    
        case NONE: return ' ';
    
        default: return (char)item;
    }
}

int get_item_color(enum Item_type item){
   switch (item){
       case NONE: return MAP_COLOR_DEFAULT;
       case BREAD: return ITEM_COLOR_BREAD;
       case HAMBURGER: return ITEM_COLOR_HAMBURGER;
       case HAMBURGER_READY: return ITEM_COLOR_HAMBURGER_READY;
       case HAMBURGER_BURNED: return ITEM_COLOR_HAMBURGER_BURNED;
       case SALAD: return ITEM_COLOR_SALAD;
       case JUICE: return ITEM_COLOR_JUICE;
       case FRIES: return ITEM_COLOR_FRIES;
       case FRIES_READY: return ITEM_COLOR_FRIES_READY;
       case FRIES_BURNED: return ITEM_COLOR_FRIES_BURNED;
       case BURGER_BREAD: return ITEM_COLOR_BURGER_BREAD;
       case SALAD_BREAD: return ITEM_COLOR_SALAD_BREAD;
       case SALAD_BURGER: return ITEM_COLOR_SALAD_BURGER;
       case FULL_BURGER: return ITEM_COLOR_FULL_BURGER;
       default: return MAP_COLOR_DEFAULT;
   }
}

/*
    Function to render map
    Responsible for rendering each character of the map, with collor and attributes
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
    Return:
        -
*/
void render_map(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y) {
    // For each character
    for (int line = 0; line < MAP_HEIGHT; line++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            int pair_color = color_map[line][col];
            int attr = attr_map[line][col];

            // Set color, attributes and render in position
            attron(COLOR_PAIR(pair_color) | attr);
            mvaddch(start_y + line, start_x + col, map[line][col]);
            attroff(COLOR_PAIR(pair_color) | attr);
        }
    }
}

/*
    Function to render all players and their itens (only if it's this client's player)
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
    Return:
        -
*/
void render_players(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y)
{
    // Access player's CR
    pthread_mutex_lock(&thread_arg->players_mutex);
    // For each player
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        // Check if player is connected
        if (!thread_arg->players[i].is_active) continue;

        // Render player
        attron(COLOR_PAIR(30 + 1 + i) | A_BOLD);
        mvaddch(start_y + thread_arg->players[i].y, start_x + thread_arg->players[i].x, map_players_char[i]);
        attroff(COLOR_PAIR(30 + 1 + i) | A_BOLD);

        // Check if player is me and has item
        if (!thread_arg->players[i].is_me || thread_arg->players[i].item == NONE) continue;

        int color_index = get_item_color(thread_arg->players[i].item);

        // Render item
        attron(COLOR_PAIR(color_index));
        mvaddch(start_y + thread_arg->players[i].last_y, start_x + thread_arg->players[i].last_x, get_item_char(thread_arg->players[i].item));
        attroff(COLOR_PAIR(color_index));
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

void render_appliances(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    pthread_mutex_lock(&thread_arg->appliances_mutex);
    for (int i = 0; i < thread_arg->num_appliances; i++) {
        // Render item or appliance (if empty)
        enum Item_type item = thread_arg->appliances[i].content;
        char char_to_render;
        int pair_color;
        if(thread_arg->appliances[i].state != EMPTY) {
            char_to_render = get_item_char(item);
            pair_color = get_item_color(item);
        } else {
            char_to_render = thread_arg->appliances[i].type == APP_OVEN ? 'O' : '0';
            pair_color = MAP_COLOR_OVEN;
        }

        attron(COLOR_PAIR(pair_color));
        mvaddch(start_y + thread_arg->appliances[i].y, start_x + thread_arg->appliances[i].x, char_to_render);
        attroff(COLOR_PAIR(pair_color));

        // Render timer
        if (thread_arg->appliances[i].state == COOKING || thread_arg->appliances[i].state == READY) {
            // Get clamped timer
            int t = thread_arg->appliances[i].time_left;
            if (t < 0) t = 0; else if (t > 9) t = 9;
            char_to_render = '0' + t;

            pair_color = thread_arg->appliances[i].state == READY ? NUMBER_COLOR_EMERGENCY : NUMBER_COLOR_WARNING;
            attron(COLOR_PAIR(pair_color) | A_BOLD);
            mvaddch(start_y + thread_arg->appliances[i].timer_y, start_x + thread_arg->appliances[i].timer_x, char_to_render);
            attroff(COLOR_PAIR(pair_color) | A_BOLD);
        }
    }
    pthread_mutex_unlock(&thread_arg->appliances_mutex);
}

void render_counters(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    pthread_mutex_lock(&thread_arg->counters_mutex);
    for (int i = 0; i < thread_arg->num_counters; i++) {
        // Se a bancada tiver item, renderiza o item
        if (thread_arg->counters[i].content != NONE) {
            char char_to_render = get_item_char(thread_arg->counters[i].content);
            int pair_color = get_item_color(thread_arg->counters[i].content);

            attron(COLOR_PAIR(pair_color) | A_UNDERLINE);
            mvaddch(start_y + thread_arg->counters[i].y, start_x + thread_arg->counters[i].x, char_to_render);
            attroff(COLOR_PAIR(pair_color) | A_UNDERLINE);
        }
    }
    pthread_mutex_unlock(&thread_arg->counters_mutex);
}
    
/*
    Function to render debug information
    Responsible for rendering each line of debug, and highliting current one
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
    Return:
        -
*/
void render_debug(THREAD_ARG_STRUCT *thread_arg)
{
    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    // For each line
    for (int i = 0; i < 10; i++)
    {
        // Set color (to highlight current line), and render line
        if ((thread_arg->current_debug_line + 10 - 1) % 10 == i) attron(COLOR_PAIR(5));
        mvprintw(i, 0, "%.20s", thread_arg->debug[i]);
        if ((thread_arg->current_debug_line + 10 - 1) % 10 == i) attroff(COLOR_PAIR(5));
    }
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}

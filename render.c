#include "render.h"
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
    init_pair(NUMBER_COLOR_CUSTOMER_DEFAULT, COLOR_BLACK, -1);  // Customer Default
    init_pair(NUMBER_COLOR_CUSTOMER_WARNING, COLOR_YELLOW, -1); // Customer Warning
    init_pair(NUMBER_COLOR_CUSTOMER_EMERGENCY, COLOR_RED, -1);  // Customer Emergency

    // Players/Customers
    init_pair(PLAYERS_COLOR_CUSTOMER, COLOR_WHITE, -1); // Customers
    init_pair(PLAYERS_COLOR_P1, COLOR_RED, -1);         // P1
    init_pair(PLAYERS_COLOR_P2, COLOR_BLUE, -1);        // P2
    init_pair(PLAYERS_COLOR_P3, COLOR_GREEN, -1);       // P3
    init_pair(PLAYERS_COLOR_P4, COLOR_YELLOW, -1);      // P4
}

/*
    Function to return a given item's respective char
    Params:
        enum Item_type item: item witch the char is wanted
    Return:
        char respective item's char
*/
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

/*
    Function to return a given item's respective color
    Params:
        enum Item_type item: item witch the color is wanted
    Return:
        int respective item's color code
*/
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
            // Get their color and attr codes from map
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
void render_players(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y) {
    // Access player's CR
    pthread_mutex_lock(&thread_arg->players_mutex);
    // For each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Check if player is connected
        if (!thread_arg->players[i].is_active) continue;

        // Render player (with bounds check)
        int px = start_x + thread_arg->players[i].x;
        int py = start_y + thread_arg->players[i].y;
        if (px >= start_x && px < start_x + MAP_WIDTH && py >= start_y && py < start_y + MAP_HEIGHT) {
            attron(COLOR_PAIR(30 + 1 + i));
            attron(A_BOLD);
            mvaddch(py, px, map_players_char[i]);
            attroff(A_BOLD);
            attroff(COLOR_PAIR(30 + 1 + i));
        }

        // Check if player is me and has item
        if (!thread_arg->players[i].is_me || thread_arg->players[i].item == NONE) continue;
        
        // If it is, get item color and ensure last coords are inside the map
        int color_index = get_item_color(thread_arg->players[i].item);
        int itx = start_x + thread_arg->players[i].last_x;
        int ity = start_y + thread_arg->players[i].last_y;

        // Render item
        if (itx >= start_x && itx < start_x + MAP_WIDTH && ity >= start_y && ity < start_y + MAP_HEIGHT) {
            char ch = (thread_arg->players[i].item != NONE) ? get_item_char(thread_arg->players[i].item) : ' ';
            attron(COLOR_PAIR(color_index));
            mvaddch(ity, itx, ch);
            attroff(COLOR_PAIR(color_index));
        }

    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to render all appliances and their itens
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
    Return:
        -
*/
void render_appliances(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    // Access appliances' CR
    pthread_mutex_lock(&thread_arg->appliances_mutex);
    
    for (int i = 0; i < thread_arg->num_appliances; i++) {
        // Render item or appliance (if empty)
        enum Item_type item = thread_arg->appliances[i].content;
        char char_to_render;
        int pair_color;
        // If the app is not empty, get item's info
        if(thread_arg->appliances[i].state != EMPTY) {
            char_to_render = get_item_char(item);
            pair_color = get_item_color(item);
        } else{ // If it is empty, see if it is oven or fryer
            char_to_render = thread_arg->appliances[i].type == APP_OVEN ? 'O' : '0';
            pair_color = MAP_COLOR_OVEN;
        }

        // Bounds check and render
        int ax = start_x + thread_arg->appliances[i].x;
        int ay = start_y + thread_arg->appliances[i].y;
        if (ax >= start_x && ax < start_x + MAP_WIDTH && ay >= start_y && ay < start_y + MAP_HEIGHT) {
            attron(COLOR_PAIR(pair_color));
            mvaddch(ay, ax, char_to_render);
            attroff(COLOR_PAIR(pair_color));
        }

        // Render timer
        int t = thread_arg->appliances[i].time_left;
        if (t < 0) t = 0; else if (t > 9) t = 9;
        char_to_render = '0' + t;
        // If there is no need for timer, render a space
        if(!(thread_arg->appliances[i].state == COOKING || thread_arg->appliances[i].state == READY)) char_to_render = ' ';
        // Define the color based on app's state
        pair_color = thread_arg->appliances[i].state == READY ? NUMBER_COLOR_EMERGENCY : NUMBER_COLOR_WARNING;
        
        // Timer coords may be -1 if not found; Check bounds and render
        if(thread_arg->appliances[i].timer_x == -1 || thread_arg->appliances[i].timer_y == - 1) continue;
        int tx = start_x + thread_arg->appliances[i].timer_x;
        int ty = start_y + thread_arg->appliances[i].timer_y;
        attron(COLOR_PAIR(pair_color) | A_BOLD);
        mvaddch(ty, tx, char_to_render);
        attroff(COLOR_PAIR(pair_color) | A_BOLD);
    }
    pthread_mutex_unlock(&thread_arg->appliances_mutex);
}

/*
    Function to render all counters and their itens
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
    Return:
        -
*/
void render_counters(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    // Access counters' CR
    pthread_mutex_lock(&thread_arg->counters_mutex);
    for (int i = 0; i < thread_arg->num_counters; i++) {
        // If counter has item
        if (thread_arg->counters[i].content != NONE) {
            // Gets its info
            char char_to_render = get_item_char(thread_arg->counters[i].content);
            int pair_color = get_item_color(thread_arg->counters[i].content);

            // Check bounds and render it
            int cx = start_x + thread_arg->counters[i].x;
            int cy = start_y + thread_arg->counters[i].y;
            if (cx >= start_x && cx < start_x + MAP_WIDTH && cy >= start_y && cy < start_y + MAP_HEIGHT) {
                attron(COLOR_PAIR(pair_color));
                attron(A_UNDERLINE);
                mvaddch(cy, cx, char_to_render);
                attroff(A_UNDERLINE);
                attroff(COLOR_PAIR(pair_color));
            }
        }
    }
    pthread_mutex_unlock(&thread_arg->counters_mutex);
}

/*
    Function to render all customers and their orders
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
		- int max_x: max x to render screen
    Return:
        -
*/
void render_customers(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y, int max_x) {
    // Access customers' CR
    pthread_mutex_lock(&thread_arg->customers_mutex);

    for(int i = 0; i < MAX_CUSTOMERS; i++) {
        CUSTOMER *c = &thread_arg->customers[i];

        if(!c->active) continue;  // Customer not active

        // Clamp order size
        if(c->order_size < 0) c->order_size = 0;
		else if(c->order_size > MAX_ORDER) c->order_size = MAX_ORDER;

        int real_x = start_x + c->x + 3;
        int real_y = start_y + c->y;

        // Customer rendering
		attron(COLOR_PAIR(PLAYERS_COLOR_CUSTOMER) | A_BOLD);
		mvaddch(real_y, real_x, 'o');
		mvaddch(real_y, real_x + 1, '/');
		attroff(COLOR_PAIR(PLAYERS_COLOR_CUSTOMER) | A_BOLD);

        // Render order
        int order_x = real_x + 3;  // Order starts one spaces after client
        for(int order_index = 0; order_index < c->order_size; order_index++) {
            if(order_x + order_index > max_x) break; // If outside screen stops rendering

			// Get item info
            if(c->order[order_index] == NONE) continue;
            char item_char = get_item_char(c->order[order_index]);
            int item_color = get_item_color(c->order[order_index]);

			// Render it
			attron(COLOR_PAIR(item_color));
			mvaddch(real_y, order_x + order_index, item_char);
			attroff(COLOR_PAIR(item_color));
        }

        // Render timer
		if(order_x + MAX_ORDER + 1 + 4 >= max_x) continue;
		int timer_color_pair = NUMBER_COLOR_CUSTOMER_DEFAULT;
		if(c->time_left <= 10)  timer_color_pair = NUMBER_COLOR_CUSTOMER_EMERGENCY;
		else if(c->time_left <= 20) timer_color_pair = NUMBER_COLOR_CUSTOMER_WARNING; 

		attron(COLOR_PAIR(timer_color_pair) | A_BOLD);
		mvprintw(real_y, order_x + MAX_ORDER + 1, "(%02d)", c->time_left);
		attroff(COLOR_PAIR(timer_color_pair) | A_BOLD);
    }

    pthread_mutex_unlock(&thread_arg->customers_mutex);
}

/*
    Function to render score
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
        - int start_x: initial x to render map
        - int start_y: initial y to render map
    Return:
        -
*/
void render_score(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y) {
    // Access score's CR
    pthread_mutex_lock(&thread_arg->score_mutex);
    int s = thread_arg->score;
    pthread_mutex_unlock(&thread_arg->score_mutex);
    // Define starting render position
    int pos_x = start_x + (MAP_WIDTH - 10) / 2; 

    // Render score
    attron(A_BOLD);
    mvprintw(start_y, pos_x, "SCORE: %03d", s);
    attroff(A_BOLD);
}
    
/*
    Function to render debug information
    Responsible for rendering each line of debug, and highliting current one
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
    Return:
        -
*/
void render_debug(THREAD_ARG_STRUCT *thread_arg) {
    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    // Verify if debug should be rendered
    if(!thread_arg->render_debug){
        pthread_mutex_unlock(&thread_arg->debug_mutex);
        return;
    }
    // For each line
    for (int i = 0; i < 10; i++) {
        // Set color (to highlight current line), and render line
        if ((thread_arg->current_debug_line + 10 - 1) % 10 == i) attron(COLOR_PAIR(5));
        mvprintw(i, 0, "%.20s", thread_arg->debug[i]);
        if ((thread_arg->current_debug_line + 10 - 1) % 10 == i) attroff(COLOR_PAIR(5));
    }
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}

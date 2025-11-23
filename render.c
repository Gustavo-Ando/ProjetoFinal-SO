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
void color_config(){
    if(!has_colors()) return;

    // Start color and use default terminal colors
    start_color();
    use_default_colors();

    // Buildings
    init_pair(MAP_COLOR_DEFAULT, COLOR_WHITE, -1); // Default
    init_pair(MAP_COLOR_WALLS, COLOR_WHITE, COLOR_WHITE); // Walls
    init_pair(MAP_COLOR_TRASH, COLOR_RED , -1); // Trash
    init_pair(MAP_COLOR_PLATES_1, COLOR_WHITE, COLOR_BLUE); // Plates 1
    init_pair(MAP_COLOR_PLATES_2, COLOR_WHITE, COLOR_CYAN); // Plates 2
    init_pair(MAP_COLOR_OVEN, COLOR_RED, -1); // Oven

    // Itens
    init_pair(ITEM_COLOR_BREAD, COLOR_YELLOW, -1); // Bread
    init_pair(ITEM_COLOR_HAMBURGER, COLOR_WHITE, -1); // Hamburger
    init_pair(ITEM_COLOR_HAMBURGER_BURNED, COLOR_BLACK, -1); // Hamburger Burned
    init_pair(ITEM_COLOR_HAMBURGER_READY, COLOR_RED, -1); // Hamburger Ready
    init_pair(ITEM_COLOR_SALAD, COLOR_GREEN, -1); // Salad
    init_pair(ITEM_COLOR_JUICE, COLOR_CYAN, -1); // Juice
    init_pair(ITEM_COLOR_FRIES, COLOR_WHITE, -1); // French Fries
    init_pair(ITEM_COLOR_FRIES_BURNED, COLOR_BLACK, -1); // French Fries Burned
    init_pair(ITEM_COLOR_FRIES_READY, COLOR_YELLOW, -1); // French Fries Ready

    // Numbers
    init_pair(NUMBER_COLOR_DEFAULT, COLOR_BLACK, COLOR_WHITE); // Default
    init_pair(NUMBER_COLOR_WARNING, COLOR_YELLOW, COLOR_WHITE); // Warning
    init_pair(NUMBER_COLOR_EMERGENCY, COLOR_RED, COLOR_WHITE); // Emergency

    // Players/Customers
    init_pair(PLAYERS_COLOR_CUSTOMER, COLOR_WHITE, -1); // Customers
    init_pair(PLAYERS_COLOR_P1, COLOR_RED, -1); // P1
    init_pair(PLAYERS_COLOR_P2, COLOR_BLUE, -1); // P2
    init_pair(PLAYERS_COLOR_P3, COLOR_GREEN, -1); // P3
    init_pair(PLAYERS_COLOR_P4, COLOR_YELLOW, -1); // P4
}

char get_item_char(enum Item_type item) {
    switch(item) {
        case BREAD:             return '=';
        case SALAD:             return '@';
        case JUICE:             return 'U';

        case HAMBURGER:         return '-';
        case HAMBURGER_READY:   return '-';
        case HAMBURGER_BURNED:  return '~';
        
        case FRIES:             return 'W';
        case FRIES_READY:       return 'W';
        case FRIES_BURNED:      return 'W'; 
        
        case NONE:              return ' ';
        
        default:                return (char)item;
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
void render_map(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    // For each character
    for(int line = 0; line < MAP_HEIGHT; line++){
        for(int col = 0; col < MAP_WIDTH; col++){
            char char_to_render = map[line][col];
            int pair_color = color_map[line][col];
            int attr = attr_map[line][col];

            for(int i=0; i < num_appliances; i++) {
                if(appliances[i].x == col && appliances[i].y == line) {
                    if(appliances[i].state != COOK_OFF) {
                        char_to_render = get_item_char(appliances[i].content);
                        
                        // Ajuste de cor baseado no item
                        switch(appliances[i].content) {
                            case HAMBURGER: pair_color = ITEM_COLOR_HAMBURGER; break;
                            case HAMBURGER_READY: pair_color = ITEM_COLOR_HAMBURGER_READY; break;
                            case HAMBURGER_BURNED: pair_color = ITEM_COLOR_HAMBURGER_BURNED; break;
                            case FRIES: pair_color = ITEM_COLOR_FRIES; break;
                            case FRIES_READY: pair_color = ITEM_COLOR_FRIES_READY; break; 
                            case FRIES_BURNED: pair_color = ITEM_COLOR_FRIES_BURNED; break;
                            default: break;
                        }
                    }
                    break;
                }

                if(appliances[i].timer_x == col && appliances[i].timer_y == line) {
                    if(appliances[i].state == COOK_COOKING || appliances[i].state == COOK_READY) {
                        // Converte o int time_left para char
                        // Assumindo que o tempo é < 10 para 1 dígito, ou usamos 9 se for maior visualmente
                        int t = appliances[i].time_left;
                        if (t < 0) t = 0;
                        if (t > 9) t = 9;
                        
                        char_to_render = '0' + t; 
                        
                        if(appliances[i].state == COOK_READY) {
                             pair_color = NUMBER_COLOR_EMERGENCY; // Vermelho se vai queimar
                        } else {
                             pair_color = NUMBER_COLOR_WARNING; // Amarelo cozinhando
                        }
                        attr = A_BOLD;
                    }
                    // Se estiver OFF ou BURNT, mantém o 'n' ou desenha espaço?
                    // O mapa original tem 'n'. Se quiser sumir com o 'n', descomente abaixo:
                    else { 
                        char_to_render = ' '; 
                        
                    } 
                }
            }

            // Set color, attributes and render in position
            attron(COLOR_PAIR(pair_color) | attr);
            mvaddch(start_y + line, start_x + col, char_to_render);
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
void render_players(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y){
    // Access player's CR
    pthread_mutex_lock(&thread_arg->players_mutex);
    // For each player
    for(int i = 0; i < MAX_PLAYERS; i++){
        // Check if player is connected
        if(!thread_arg->players[i].is_active) continue;

        // Render player
        attron(COLOR_PAIR(30 + 1 + i) | A_BOLD);
        mvaddch(start_y + thread_arg->players[i].y, start_x + thread_arg->players[i].x, map_players_char[i]);
        attroff(COLOR_PAIR(30 + 1 + i) | A_BOLD);

        // Check if player is me and has item
        if(!thread_arg->players[i].is_me || thread_arg->players[i].item == NONE) continue;
        
        int color_index;
        switch(thread_arg->players[i].item){
            case BREAD: color_index = 10; break;
            case HAMBURGER: color_index = 11; break;
            case HAMBURGER_BURNED: color_index = 12; break;
            case HAMBURGER_READY: color_index = 13; break;
            case SALAD: color_index = 14; break;
            case JUICE: color_index = 15; break;
            case FRIES: color_index = 16; break;
            case FRIES_BURNED: color_index = 17; break;
            case FRIES_READY: color_index = 18; break;
            case NONE: color_index = 0; break;
        }

        // Render item
        attron(COLOR_PAIR(color_index));
        mvaddch(start_y + thread_arg->players[i].last_y, start_x + thread_arg->players[i].last_x, get_item_char(thread_arg->players[i].item));
        attroff(COLOR_PAIR(color_index));
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to render debug information
    Responsible for rendering each line of debug, and highliting current one
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contatining shared information
    Return:
        - 
*/
void render_debug(THREAD_ARG_STRUCT *thread_arg){
    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    // For each line
    for(int i = 0; i < 10; i++){
        // Set color (to highlight current line), and render line
        if((thread_arg->current_debug_line + 10 - 1)%10== i) attron(COLOR_PAIR(5));
        mvprintw(i, 0, "%.20s", thread_arg->debug[i]);
        if((thread_arg->current_debug_line + 10 - 1)%10== i) attroff(COLOR_PAIR(5));
    }
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}

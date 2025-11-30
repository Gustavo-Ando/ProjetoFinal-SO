#include "utility.h"
#include "server.h"
#include "map.h"
#include "server_send_message.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

/*
    Funcion to do the application interaction
    Responsible for updating applications' and players' itens, combining itens and starting apps
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int app_updated_index: index of the updated app
*/
int app_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index){
    int app_updated_index = -1;
    int app_id = get_appliance_id_at(px, py);

    // Access both app's CR and clients's CR at the same time
    mutex_lock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);

    // Get player's item
    enum Item_type p_item = thread_arg->clients[index].item;
    
    // If appliance and player both have an item, try to combine
    if (p_item != NONE && thread_arg->appliances[app_id].content != NONE) {
        enum Item_type combined = try_combine(p_item, thread_arg->appliances[app_id].content);

        if (combined != NONE) {
            // Give the combined item to the player
            thread_arg->clients[index].item = combined;
            thread_arg->appliances[app_id].state = EMPTY;
            thread_arg->appliances[app_id].content = NONE;
            thread_arg->appliances[app_id].time_left = 0;
            app_updated_index = app_id;
        }
    } else if (thread_arg->appliances[app_id].state == EMPTY) { // Player puts the item on the appliance
        int success = 0;

        // Check if item and appliance match
        if (thread_arg->appliances[app_id].type == APP_OVEN && p_item == HAMBURGER) {
            thread_arg->appliances[app_id].content = HAMBURGER;
            success = 1;
        } else if (thread_arg->appliances[app_id].type == APP_FRYER && p_item == FRIES) {
            thread_arg->appliances[app_id].content = FRIES;
            success = 1;
        }
        // If success, update player's item and app state
        if (success) {
            thread_arg->clients[index].item = NONE;
            thread_arg->appliances[app_id].state = COOKING;
            clock_gettime(CLOCK_MONOTONIC, &thread_arg->appliances[app_id].start_time);
            thread_arg->appliances[app_id].last_update_time = 0;
            thread_arg->appliances[app_id].time_left = TIME_TO_COOK;
            app_updated_index = app_id;
        }
    } else if ((thread_arg->appliances[app_id].state != COOKING) && p_item == NONE) { // Player gets the item from app if it's done cooking
        thread_arg->clients[index].item = thread_arg->appliances[app_id].content;
        thread_arg->appliances[app_id].state = EMPTY;
        thread_arg->appliances[app_id].content = NONE;
        thread_arg->appliances[app_id].time_left = 0;
        app_updated_index = app_id;
    }
    
    mutex_unlock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);
    return app_updated_index;
}

/*
    Funcion to do the counter interaction
    Responsible for updating counter' and players' itens and combining itens
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int counter_updated_index:  index of the updated counter
*/
int counter_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index){
    // Access both counter's CR and clients's CR at the same time
    mutex_lock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);
    
    int counter_id = get_counter_id_at(px, py);
    int counter_updated_index = -1;

    // Get player's and counter's itens
    enum Item_type player_item = thread_arg->clients[index].item;
    enum Item_type counter_item = thread_arg->counters[counter_id].content;

    // If player and counter both have an item, try to combine
    if (player_item != NONE && counter_item != NONE) {
        enum Item_type combined = try_combine(player_item, counter_item);
        // If combination was successful, update both player and counter itens
        // Player gets the combined item, counter is now empty
        if (combined != NONE) {
            thread_arg->clients[index].item = combined;
            thread_arg->counters[counter_id].content = NONE;
            counter_updated_index = counter_id;
        } // If combination was not a success, nothing to do
    } else if (player_item != NONE && counter_item == NONE) { // Place the player item on empty counter
        thread_arg->counters[counter_id].content = player_item;
        thread_arg->clients[index].item = NONE;
        counter_updated_index = counter_id;
    } else if (player_item == NONE && counter_item != NONE) { // Get an item from the counter
        thread_arg->clients[index].item = counter_item;
        thread_arg->counters[counter_id].content = NONE;
        counter_updated_index = counter_id;
    }

    mutex_unlock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);
    return counter_updated_index;
}

/*
    Function to remove one instance of an item from a customer's order
    Params:
        - CUSTOMER *c: pointer to the customer
        - enum Item_type item: item to remove
    Return:
        - int: 0 if successful, -1 if not found or error
*/

static int customer_remove_order_item(CUSTOMER *c, enum Item_type item) {
    if (c == NULL) return -1;
    if (c->order_size <= 0 || c->active == 0 || item == NONE) return -1;
    // Find first match
    for (int i = 0; i < c->order_size; ++i) {
        if (c->order[i] == item) {
            // Shift left
            for (int j = i; j + 1 < c->order_size; ++j) {
                c->order[j] = c->order[j+1];
            }
            // Clear tail
            c->order[c->order_size - 1] = NONE;
            c->order_size--;
            return 0;
        }
    }
    return -1; // Not found
}


/*
    Function to do the customer interaction
    Responsible for updating customers' and players' itens, combining itens and giving score
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int customer_id: id of customer served (-1 if none)
*/
int customer_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index) {
    int score_gained = 0;
    int customer_id = get_customer_id_at(px, py);
    
    // Lock shared resources
    mutex_lock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);

    // Get player's item
    enum Item_type player_item = thread_arg->clients[index].item;
    CUSTOMER *c = &thread_arg->customers[customer_id];
    
    // Check if item can be delivered to customer, removing it's order
    if (customer_remove_order_item(c, player_item) == -1) {
        // If can't proceed, unlock mutexes and terminate
        mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);
        return -1;
    }

    // Remove item from player
    thread_arg->clients[index].item = NONE;

    // Score update
    score_gained++;
    
    // Check if the order is complete
    if (c->order_size == 0) {
        c->active = 0;
        c->delivered = 1;
        // Clear order array safely
        for (int j = 0; j < MAX_ORDER; j++) c->order[j] = NONE;
    }

    // Unlock mutexes
    mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);

    // Access score CR and update it
    pthread_mutex_lock(&thread_arg->score_mutex);
    thread_arg->score += score_gained;
    pthread_mutex_unlock(&thread_arg->score_mutex);
    
    return customer_id;
}

/*
    Function to treat input from a client
    Responsible for checking if a move or action is valid, processing it's effects and sending result to all clients
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
        - char input: character inputed from client
        - int index: index of client responsible for input
    Return:
        -
*/
void treat_client_input(THREAD_ARG_STRUCT *thread_arg, char input, int index) {

    int try_moved = 0, try_action = 0;
    int app_updated_index = -1, counter_updated_index = -1, customer_updated_index = -1;

    switch (input) {
        // Movement (W, A, S, D)
        case 'w':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y - 1][thread_arg->clients[index].x]))
                thread_arg->clients[index].y--;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 'a':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x - 2]))
                thread_arg->clients[index].x -= 2;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 's':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y + 1][thread_arg->clients[index].x]))
                thread_arg->clients[index].y++;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 'd':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x + 2]))
                thread_arg->clients[index].x += 2;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
    
        // Interaction
        case ' ':
            try_action = 1;

            // Access client's CR to get position, item, and item in map
            pthread_mutex_lock(&thread_arg->clients_mutex);
            int px = thread_arg->clients[index].x;
            int py = thread_arg->clients[index].y;
            enum Item_type client_item = thread_arg->clients[index].item;
            enum Item_type map_item = item_map[py][px];
            pthread_mutex_unlock(&thread_arg->clients_mutex);

            // Check action to perform
            if (map_item != NONE && client_item == NONE){ // Get itens from map
                // Access client's CR
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].item = map_item;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if (map_item != NONE && client_item != NONE) { // Try to combine itens
                enum Item_type combined = try_combine(client_item, map_item);
                // Access client's CR
                pthread_mutex_lock(&thread_arg->clients_mutex);
                if (combined != NONE) thread_arg->clients[index].item = combined;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if (trash_map[py][px]) { // Trash item
                // Access client's CR
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].item = NONE;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if(appliance_interaction_map[py][px] != -1){ // Appliance interaction
                app_updated_index = app_interaction(thread_arg, px, py, index);
            } else if(counter_interaction_map[py][px] != -1){ // Counter interaction
                counter_updated_index = counter_interaction(thread_arg, px, py, index);
            } else if(customer_interaction_map[py][px] != -1){ // Customer interaction
                customer_updated_index = customer_interaction(thread_arg, px, py, index);
            }
            break;
    
        default:
            break;
    }

    printf("Received input from client %d: %c\n", index, input);
    printf(" - %d, %d\n", thread_arg->clients[index].x, thread_arg->clients[index].y);

    if (try_moved) broadcast_player_position(thread_arg, index);
    if (try_action) broadcast_player_item(thread_arg, index);
    if (app_updated_index != -1) broadcast_appliance_status(thread_arg, app_updated_index);
    if (counter_updated_index != -1) broadcast_counter_update(thread_arg, counter_updated_index);
    if (customer_updated_index != -1) {
        broadcast_customer_update(thread_arg, customer_updated_index);
        broadcast_score(thread_arg);
    }
}

/*
    Function to create customer
	Responsible for finding an empty customer slot and initializing a random client
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
    Return:
        -
*/
void create_customer(THREAD_ARG_STRUCT *thread_arg){
	// Access customer CR
	pthread_mutex_lock(&thread_arg->customers_mutex);

	// Search for an inactive customer slot
	int created_index = -1;
	for (int i = 0; i < thread_arg->num_customers; i++) {
		CUSTOMER *c = &thread_arg->customers[i];
		// Ignore already active customers
		if(c->active) continue;

		// Activate customer
		c->active = 1;
		c->delivered = 0;

		// Random time left (between 20 and 40 seconds)
		c->time_left = 20 + (rand() % 21);

		// Random order size (between 1 and MAX_ORDER)
		c->order_size = 1 + (rand() % MAX_ORDER);

		// Generate random order
		for (int j = 0; j < c->order_size; j++) {
			int r = rand() % 5;
			switch (r) {
				case 0: c->order[j] = BURGER_BREAD; break;
				case 1: c->order[j] = SALAD; break;
				case 2: c->order[j] = JUICE; break;
				case 3: c->order[j] = FULL_BURGER; break;
				case 4: c->order[j] = FRIES_READY; break;
			}
		}   

		// Clear remaining order slots
		for (int j = c->order_size; j < MAX_ORDER; ++j) c->order[j] = NONE;
		
		// Exit loop
		created_index = i;
		break;
	}
	pthread_mutex_unlock(&thread_arg->customers_mutex);

	// Update clients
	if (created_index != -1) broadcast_customer_update(thread_arg, created_index);
}

/*
    Function to tick customer (called every second)
	Responsible for ticking customer times and removing customers ready to leave (updating and broadcasting the score)
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
    Return:
        -
*/
void tick_customers(THREAD_ARG_STRUCT *thread_arg){
	int customers_to_update[MAX_CUSTOMERS];
	int update_count = 0;
	int score_change = 0;

	// Access customer's CR
	pthread_mutex_lock(&thread_arg->customers_mutex);
	for (int i = 0; i < thread_arg->num_customers; i++) {
		CUSTOMER *c = &thread_arg->customers[i];
		// Skip inactive customers
		if (!c->active) continue;

		// Tick timer
		c->time_left--;

		// Leave
		if (c->time_left <= 0) {
			c->time_left = 0;
			c->active = 0;

			//Minus 1 score
			score_change--;
		}
		
		// Add on updated list
		customers_to_update[update_count++] = i;
	}
	pthread_mutex_unlock(&thread_arg->customers_mutex);

	// Broadcast customers updates
	for (int k = 0; k < update_count; k++) {
		broadcast_customer_update(thread_arg, customers_to_update[k]);
	}

	if (score_change != 0) {
		// Access score CR and pdate score
		pthread_mutex_lock(&thread_arg->score_mutex);
        if(thread_arg->score > 0)
		    thread_arg->score += score_change; 
		pthread_mutex_unlock(&thread_arg->score_mutex);

		// Broadcat new score
		broadcast_score(thread_arg);
	}
}

/*
    Function to tick a specific appliance
	Responsible for updating it's timer, state and content, should not lock appliance mutex (already should be locked)
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
		- int index: appliance index
		- int current_time_elapsed: time (in seconds) since start of appliance
    Return:
        - int changed: returns 1 if appliance changed timer or status
*/
static int tick_appliance(THREAD_ARG_STRUCT *thread_arg, int index, int current_time_elapsed){
	int changed = 0;
	thread_arg->appliances[index].last_update_time++;

	if (thread_arg->appliances[index].state == COOKING) {
		// COOKING to READY
		int new_time_left = TIME_TO_COOK - current_time_elapsed;
		if (new_time_left < 0) new_time_left = 0;

		thread_arg->appliances[index].time_left = new_time_left;
		changed = 1;

		if (current_time_elapsed >= TIME_TO_COOK) {
			thread_arg->appliances[index].state = READY;
			thread_arg->appliances[index].content = (thread_arg->appliances[index].type == APP_OVEN) ? HAMBURGER_READY : FRIES_READY;
			thread_arg->appliances[index].time_left = TIME_TO_BURN - current_time_elapsed;
		}
	} else if (thread_arg->appliances[index].state == READY) {
		// READY to BURNED
		int new_time_left = TIME_TO_BURN - current_time_elapsed;
		if (new_time_left < 0) new_time_left = 0;

		thread_arg->appliances[index].time_left = new_time_left;
		changed = 1;

		if (current_time_elapsed >= TIME_TO_BURN) {
			thread_arg->appliances[index].state = BURNED;
			thread_arg->appliances[index].content = (thread_arg->appliances[index].type == APP_OVEN) ? HAMBURGER_BURNED : FRIES_BURNED;
			thread_arg->appliances[index].time_left = 0;
		}
	}

	return changed;
}

/*
    Function to tick all appliances
	Responsible for verifiying if a second has passed since last update (for each appliance) and ticking it if necessary. Sends broadcast of updates to clients
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
    Return:
		-
*/
void tick_appliances(THREAD_ARG_STRUCT *thread_arg){
	int updated[MAX_APPLIANCES] = { -1 };
	int num_updated = 0;

    // Access appliances' CR
	pthread_mutex_lock(&thread_arg->appliances_mutex);
	for (int i = 0; i < thread_arg->num_appliances; i++) {
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		int diff_time = seconds_between(thread_arg->appliances[i].start_time, now);
		if(thread_arg->appliances[i].last_update_time < diff_time) {
			if(tick_appliance(thread_arg, i, diff_time)) updated[num_updated++] = i;
				
		}
	}
	pthread_mutex_unlock(&thread_arg->appliances_mutex);

	// Warns all clients of all changes
	for(int i = 0; i < num_updated; i++) broadcast_appliance_status(thread_arg, updated[i]);
}

#ifndef RENDER_H
#define RENDER_H

#include "client.h"

void color_config();

void render_map(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y);
void render_players(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y);
void render_debug(THREAD_ARG_STRUCT *thread_arg);
void render_appliances(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y);
void render_counters(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y);
void render_customers(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y, int max_x);
void render_score(THREAD_ARG_STRUCT *thread_arg, int start_x, int start_y);

#endif

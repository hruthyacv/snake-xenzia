#ifndef GAME_H
#define GAME_H
#include "defs.h"
void game_init(App *app);
void game_update(App *app);
void game_handle_key(App *app, SDL_Keycode key);
void game_spawn_food(App *app);
void game_spawn_powerup(App *app);
void game_spawn_obstacle(App *app);
void game_apply_powerup(App *app, PowerUpType t);
void game_end(App *app);
const char *mode_name(GameMode m);
#endif

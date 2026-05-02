#ifndef UI_H
#define UI_H
#include "defs.h"

/* Initialise fonts and return 1 on success */
int  ui_init_fonts(App *app);
void ui_free_fonts(App *app);

/* Per-frame render entry point — dispatches to sub-renderers */
void ui_render(App *app);

/* Individual screen renderers */
void ui_render_menu(App *app);
void ui_render_mode_select(App *app);
void ui_render_game(App *app);
void ui_render_paused(App *app);
void ui_render_gameover(App *app);
void ui_render_leaderboard(App *app);
void ui_render_name_entry(App *app);

/* Primitive helpers */
void ui_draw_text(App *app, TTF_Font *font, const char *text,
                  int x, int y, SDL_Color col, int centered);
void ui_draw_rect_filled(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c);
void ui_draw_rect_outline(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c, int thick);
void ui_draw_rounded_rect(SDL_Renderer *r, int x, int y, int w, int h, int rad, SDL_Color c);
void ui_draw_glow_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color col);
void ui_draw_progress_bar(SDL_Renderer *r, int x, int y, int w, int h,
                          float frac, SDL_Color bg, SDL_Color fg);

/* Name entry input */
void ui_name_entry_key(App *app, SDL_Keycode key, const char *text);
#endif

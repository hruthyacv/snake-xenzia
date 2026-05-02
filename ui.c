/*
 * ui.c — All SDL2 rendering for Snake Xenzia.
 *
 * Covers: grid, snake (with interpolation), food glow, power-ups,
 * particles, sidebar panel, all menus, pause, game-over, leaderboard.
 */
#include "ui.h"
#include "game.h"
#include "fileio.h"
#include <math.h>

/* ══════════════════════════════════════════════════════════════════════
 * Font management
 * ══════════════════════════════════════════════════════════════════════ */

int ui_init_fonts(App *app)
{
    /* Try to load a system font. Fall back through common paths. */
    const char *paths[] = {
        "assets/font.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };
    for (int i=0; paths[i]; i++) {
        app->font_lg = TTF_OpenFont(paths[i], 36);
        if (app->font_lg) {
            app->font_md = TTF_OpenFont(paths[i], 22);
            app->font_sm = TTF_OpenFont(paths[i], 16);
            return 1;
        }
    }
    SDL_Log("WARNING: Could not load any font. Text will not render.");
    return 0;
}

void ui_free_fonts(App *app)
{
    if (app->font_lg) TTF_CloseFont(app->font_lg);
    if (app->font_md) TTF_CloseFont(app->font_md);
    if (app->font_sm) TTF_CloseFont(app->font_sm);
}

/* ══════════════════════════════════════════════════════════════════════
 * Primitive helpers
 * ══════════════════════════════════════════════════════════════════════ */

void ui_draw_rect_filled(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c)
{
    SET_COLOR(r, c);
    SDL_Rect rect = {x,y,w,h};
    SDL_RenderFillRect(r, &rect);
}

void ui_draw_rect_outline(SDL_Renderer *r, int x, int y, int w, int h,
                           SDL_Color c, int thick)
{
    SET_COLOR(r, c);
    for (int t=0; t<thick; t++) {
        SDL_Rect rect = {x+t, y+t, w-2*t, h-2*t};
        SDL_RenderDrawRect(r, &rect);
    }
}

/* Approximate rounded rectangle using filled rects + corner circles */
void ui_draw_rounded_rect(SDL_Renderer *r, int x, int y, int w, int h,
                           int rad, SDL_Color c)
{
    SET_COLOR(r, c);
    /* Main cross */
    SDL_Rect rh = {x+rad, y, w-2*rad, h};
    SDL_Rect rv = {x, y+rad, w, h-2*rad};
    SDL_RenderFillRect(r, &rh);
    SDL_RenderFillRect(r, &rv);
    /* Corners (simple filled squares — good enough at this scale) */
    SDL_Rect c1={x,y,rad,rad}, c2={x+w-rad,y,rad,rad};
    SDL_Rect c3={x,y+h-rad,rad,rad}, c4={x+w-rad,y+h-rad,rad,rad};
    SDL_RenderFillRect(r,&c1); SDL_RenderFillRect(r,&c2);
    SDL_RenderFillRect(r,&c3); SDL_RenderFillRect(r,&c4);
}

/* Soft glow circle drawn with concentric transparent circles */
void ui_draw_glow_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color col)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int ring = radius; ring > 0; ring--) {
        Uint8 alpha = (Uint8)(col.a * (1.0f - (float)ring/radius));
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, alpha);
        /* Draw circle via Bresenham */
        int x0=ring, y0=0, err=0;
        while (x0 >= y0) {
            SDL_RenderDrawPoint(r,cx+x0,cy+y0); SDL_RenderDrawPoint(r,cx-x0,cy+y0);
            SDL_RenderDrawPoint(r,cx+x0,cy-y0); SDL_RenderDrawPoint(r,cx-x0,cy-y0);
            SDL_RenderDrawPoint(r,cx+y0,cy+x0); SDL_RenderDrawPoint(r,cx-y0,cy+x0);
            SDL_RenderDrawPoint(r,cx+y0,cy-x0); SDL_RenderDrawPoint(r,cx-y0,cy-x0);
            y0++;
            err += 1+2*y0;
            if (2*(err-x0)+1 > 0){ x0--; err += 1-2*x0; }
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void ui_draw_progress_bar(SDL_Renderer *r, int x, int y, int w, int h,
                           float frac, SDL_Color bg, SDL_Color fg)
{
    ui_draw_rect_filled(r,x,y,w,h,bg);
    if (frac > 0.0f) {
        int fw = (int)(w * CLAMP(frac,0,1));
        ui_draw_rect_filled(r,x,y,fw,h,fg);
    }
}

void ui_draw_text(App *app, TTF_Font *font, const char *text,
                  int x, int y, SDL_Color col, int centered)
{
    if (!font || !text || !text[0]) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(app->renderer, surf);
    if (tex) {
        SDL_Rect dst = {x, y, surf->w, surf->h};
        if (centered) dst.x -= surf->w/2;
        SDL_RenderCopy(app->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

/* ══════════════════════════════════════════════════════════════════════
 * Board & grid rendering
 * ══════════════════════════════════════════════════════════════════════ */

static void render_grid(App *app)
{
    SDL_Renderer *r = app->renderer;

    /* Board background */
    ui_draw_rect_filled(r, GRID_X_OFFSET, GRID_Y_OFFSET,
                        BOARD_W, BOARD_H, COL_BG);

    /* Grid lines */
    SET_COLOR(r, COL_GRID);
    for (int col=0; col<=GRID_COLS; col++) {
        int x = GRID_X_OFFSET + col*CELL_SIZE;
        SDL_RenderDrawLine(r, x, GRID_Y_OFFSET, x, GRID_Y_OFFSET+BOARD_H);
    }
    for (int row=0; row<=GRID_ROWS; row++) {
        int y = GRID_Y_OFFSET + row*CELL_SIZE;
        SDL_RenderDrawLine(r, GRID_X_OFFSET, y, GRID_X_OFFSET+BOARD_W, y);
    }

    /* Border */
    ui_draw_rect_outline(r, GRID_X_OFFSET, GRID_Y_OFFSET,
                         BOARD_W, BOARD_H, COL_BORDER, 2);
}

/* ── Food rendering with glow pulse ── */
static void render_food(App *app)
{
    SDL_Renderer *r = app->renderer;
    int cx = GRID_X_OFFSET + app->food.x * CELL_SIZE + CELL_SIZE/2;
    int cy = GRID_Y_OFFSET + app->food.y * CELL_SIZE + CELL_SIZE/2;
    int glow_r = (int)(CELL_SIZE * (0.6f + app->food_pulse * 0.25f));

    SDL_Color glow = COL_FOOD_GLOW;
    glow.a = (Uint8)(60 + app->food_pulse * 80);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    ui_draw_glow_circle(r, cx, cy, glow_r, glow);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Food square */
    int margin = 3;
    SDL_Rect fr = {GRID_X_OFFSET + app->food.x*CELL_SIZE + margin,
                   GRID_Y_OFFSET + app->food.y*CELL_SIZE + margin,
                   CELL_SIZE - 2*margin, CELL_SIZE - 2*margin};
    SET_COLOR(r, COL_FOOD);
    SDL_RenderFillRect(r, &fr);

    /* Bright centre dot */
    SDL_Rect dot = {cx-2, cy-2, 4, 4};
    SET_COLOR(r, COL_WHITE);
    SDL_RenderFillRect(r, &dot);
}

/* ── Power-up on board ── */
static SDL_Color pu_color(PowerUpType t) {
    switch(t){
        case PU_SPEED:  return COL_PU_SPEED;
        case PU_SLOW:   return COL_PU_SLOW;
        case PU_DOUBLE: return COL_PU_DOUBLE;
        case PU_SHRINK: return COL_PU_SHRINK;
        default:        return COL_WHITE;
    }
}

static const char* pu_label(PowerUpType t) {
    switch(t){
        case PU_SPEED:  return "SPD";
        case PU_SLOW:   return "SLW";
        case PU_DOUBLE: return "X2";
        case PU_SHRINK: return "SHR";
        default:        return "?";
    }
}

static void render_powerup(App *app)
{
    if (!app->board_pu.active) return;
    SDL_Renderer *r = app->renderer;
    SDL_Color col = pu_color(app->board_pu.type);

    int bx = GRID_X_OFFSET + app->board_pu.pos.x * CELL_SIZE;
    int by = GRID_Y_OFFSET + app->board_pu.pos.y * CELL_SIZE;
    float p = app->board_pu.pulse;
    int margin = (int)(2 + p*2);

    /* Pulsing outer glow */
    SDL_Color glow = col;
    glow.a = (Uint8)(40 + p*80);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect gr = {bx-3, by-3, CELL_SIZE+6, CELL_SIZE+6};
    SET_COLOR(r, glow);
    SDL_RenderFillRect(r, &gr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Diamond shape */
    SDL_Rect pr = {bx+margin, by+margin, CELL_SIZE-2*margin, CELL_SIZE-2*margin};
    SET_COLOR(r, col);
    SDL_RenderFillRect(r, &pr);

    /* Label text */
    if (app->font_sm) {
        ui_draw_text(app, app->font_sm, pu_label(app->board_pu.type),
                     bx + CELL_SIZE/2, by+2, COL_BLACK, 1);
    }
}

/* ── Obstacles ── */
static void render_obstacles(App *app)
{
    SDL_Renderer *r = app->renderer;
    for (int i=0;i<app->obs_count;i++){
        int bx = GRID_X_OFFSET + app->obstacles[i].x * CELL_SIZE;
        int by = GRID_Y_OFFSET + app->obstacles[i].y * CELL_SIZE;
        SDL_Rect rect = {bx+1, by+1, CELL_SIZE-2, CELL_SIZE-2};
        SET_COLOR(r, COL_OBSTACLE);
        SDL_RenderFillRect(r, &rect);
        /* Cross pattern */
        SDL_Color darker = {80,80,100,255};
        SET_COLOR(r, darker);
        SDL_RenderDrawLine(r, bx+2, by+2, bx+CELL_SIZE-3, by+CELL_SIZE-3);
        SDL_RenderDrawLine(r, bx+CELL_SIZE-3, by+2, bx+2, by+CELL_SIZE-3);
    }
}

/* ── Snake rendering with smooth interpolation ── */
static void render_snake_segment(SDL_Renderer *r, int px, int py,
                                  SDL_Color col, int margin)
{
    SDL_Rect rect = {px + margin, py + margin,
                     CELL_SIZE - 2*margin, CELL_SIZE - 2*margin};
    SET_COLOR(r, col);
    SDL_RenderFillRect(r, &rect);
}

static void render_snake(App *app, const Snake *s)
{
    if (!s->alive) return;
    SDL_Renderer *r = app->renderer;

    SDL_Color head_col = s->is_ai ? COL_AI_HEAD : COL_SNAKE_HEAD;
    SDL_Color body_col = s->is_ai ? COL_AI_BODY : COL_SNAKE_BODY;

    for (int i = s->length-1; i >= 0; i--) {
        int bx, by;

        if (i == 0) {
            /* Interpolate head between prev_head and body[0] */
            float t = CLAMP(s->anim_t, 0.0f, 1.0f);
            float fx = s->prev_head.x + (s->body[0].x - s->prev_head.x) * t;
            float fy = s->prev_head.y + (s->body[0].y - s->prev_head.y) * t;
            bx = GRID_X_OFFSET + (int)(fx * CELL_SIZE);
            by = GRID_Y_OFFSET + (int)(fy * CELL_SIZE);
        } else {
            bx = GRID_X_OFFSET + s->body[i].x * CELL_SIZE;
            by = GRID_Y_OFFSET + s->body[i].y * CELL_SIZE;
        }

        int margin = (i == 0) ? 1 : 2;

        /* Gradient body: get darker toward tail */
        if (i > 0) {
            float fade = 1.0f - (float)i / s->length * 0.4f;
            SDL_Color gc = {
                (Uint8)(body_col.r * fade),
                (Uint8)(body_col.g * fade),
                (Uint8)(body_col.b * fade), 255
            };
            render_snake_segment(r, bx, by, gc, margin);
        } else {
            render_snake_segment(r, bx, by, head_col, margin);

            /* Head glow */
            SDL_Color glow = s->is_ai ? COL_AI_HEAD : COL_SNAKE_GLOW;
            glow.a = 60;
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            ui_draw_glow_circle(r, bx+CELL_SIZE/2, by+CELL_SIZE/2, CELL_SIZE, glow);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

            /* Eyes */
            SDL_Color eye = {18,18,30,255};
            int ex = bx + CELL_SIZE - 5, ey = by + 4;
            SDL_Rect e1 = {ex-2, ey, 3, 3};
            SDL_Rect e2 = {ex-2, ey+5, 3, 3};
            SET_COLOR(r, eye);
            SDL_RenderFillRect(r, &e1);
            SDL_RenderFillRect(r, &e2);
        }
    }
}

/* ── Particles ── */
static void render_particles(App *app)
{
    SDL_Renderer *r = app->renderer;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i=0;i<app->particle_count;i++){
        Particle *p = &app->particles[i];
        Uint8 alpha = (Uint8)(p->life * 220);
        SDL_SetRenderDrawColor(r, p->color.r, p->color.g, p->color.b, alpha);
        int sz = (int)(p->size * p->life);
        SDL_Rect rect = {(int)p->x - sz/2, (int)p->y - sz/2, sz, sz};
        SDL_RenderFillRect(r, &rect);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ══════════════════════════════════════════════════════════════════════
 * Sidebar / HUD panel
 * ══════════════════════════════════════════════════════════════════════ */

static const char* pu_full_name(PowerUpType t) {
    switch(t){
        case PU_SPEED:  return "SPEED BOOST";
        case PU_SLOW:   return "SLOW MOTION";
        case PU_DOUBLE: return "DOUBLE SCORE";
        case PU_SHRINK: return "SHRINK";
        default:        return "";
    }
}

static void render_sidebar(App *app)
{
    SDL_Renderer *r = app->renderer;
    int sx = BOARD_W + GRID_X_OFFSET;
    int sy = 0;
    int sw = SIDEBAR_W;
    int sh = WINDOW_H;

    /* Sidebar background */
    ui_draw_rect_filled(r, sx, sy, sw, sh, COL_SIDEBAR);
    /* Left border line */
    SDL_Color border = {50,50,80,255};
    SET_COLOR(r, border);
    SDL_RenderDrawLine(r, sx, sy, sx, sy+sh);

    int tx = sx + 14;
    int ty = 14;

#define SB_LABEL(text, y_off, col) \
    ui_draw_text(app, app->font_sm, text, tx, ty+(y_off), col, 0)
#define SB_VALUE(text, y_off, col) \
    ui_draw_text(app, app->font_md, text, tx, ty+(y_off), col, 0)

    /* Title */
    ui_draw_text(app, app->font_md, "SNAKE XENZIA",
                 sx + sw/2, ty, COL_ACCENT, 1);
    ty += 32;

    /* Mode */
    SB_LABEL("MODE", 0, COL_TEXT_DIM);
    SB_VALUE(mode_name(app->mode), 18, COL_ACCENT);
    ty += 52;

    /* Divider */
    SET_COLOR(r, border);
    SDL_RenderDrawLine(r, sx+8, ty, sx+sw-8, ty);
    ty += 12;

    /* Score */
    SB_LABEL("SCORE", 0, COL_TEXT_DIM);
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", app->score);
    ui_draw_text(app, app->font_lg, buf, sx+sw/2, ty+16, COL_ACCENT, 1);
    ty += 60;

    /* High Score */
    SB_LABEL("BEST", 0, COL_TEXT_DIM);
    snprintf(buf, sizeof(buf), "%d", app->high_score);
    SB_VALUE(buf, 18, COL_TEXT_GOLD);
    ty += 50;

    /* Divider */
    SET_COLOR(r, border);
    SDL_RenderDrawLine(r, sx+8, ty, sx+sw-8, ty);
    ty += 12;

    /* Combo */
    if (app->combo > 1) {
        snprintf(buf, sizeof(buf), "COMBO x%d!", app->combo);
        SDL_Color pulse_col = {
            255, (Uint8)(100 + (int)(app->food_pulse*155)), 0, 255};
        ui_draw_text(app, app->font_md, buf, sx+sw/2, ty, pulse_col, 1);
    } else {
        SB_LABEL("COMBO", 0, COL_TEXT_DIM);
        SB_LABEL("--", 18, COL_TEXT);
    }
    ty += 44;

    /* Length */
    SB_LABEL("LENGTH", 0, COL_TEXT_DIM);
    snprintf(buf, sizeof(buf), "%d", app->player.length);
    SB_VALUE(buf, 18, COL_TEXT);
    ty += 48;

    /* Divider */
    SET_COLOR(r, border);
    SDL_RenderDrawLine(r, sx+8, ty, sx+sw-8, ty);
    ty += 12;

    /* Speed bar */
    SB_LABEL("SPEED", 0, COL_TEXT_DIM);
    float speed_frac = 1.0f - (float)(app->move_interval_ms - SPEED_MIN) /
                                      (SPEED_NORMAL - SPEED_MIN);
    SDL_Color speed_col = {
        (Uint8)(57 + speed_frac*198), (Uint8)(255 - speed_frac*200), 20, 255};
    ui_draw_progress_bar(r, tx, ty+20, sw-28, 12, speed_frac,
                         (SDL_Color){40,40,60,255}, speed_col);
    ty += 44;

    /* Time Attack countdown */
    if (app->mode == MODE_TIME_ATTACK) {
        SB_LABEL("TIME LEFT", 0, COL_TEXT_DIM);
        float tfrac = (float)app->time_remaining_s / TIME_ATTACK_SECS;
        SDL_Color tc = tfrac > 0.33f ? COL_TIME_BAR : (SDL_Color){255,80,80,255};
        snprintf(buf, sizeof(buf), "%ds", app->time_remaining_s);
        ui_draw_text(app, app->font_md, buf, tx+80, ty+16, tc, 0);
        ui_draw_progress_bar(r, tx, ty+20, sw-28, 10, tfrac,
                             (SDL_Color){40,40,60,255}, tc);
        ty += 46;
    }

    /* Active power-up */
    if (app->active_fx.type != PU_NONE) {
        SET_COLOR(r, border);
        SDL_RenderDrawLine(r, sx+8, ty, sx+sw-8, ty);
        ty += 10;

        SDL_Color pc = pu_color(app->active_fx.type);
        SB_LABEL("ACTIVE POWER", 0, COL_TEXT_DIM);
        ui_draw_text(app, app->font_sm, pu_full_name(app->active_fx.type),
                     tx, ty+18, pc, 0);
        Uint32 rem = (app->active_fx.end_time_ms > app->ticks) ?
                      app->active_fx.end_time_ms - app->ticks : 0;
        float pufrac = (float)rem / POWERUP_EFFECT_MS;
        ui_draw_progress_bar(r, tx, ty+36, sw-28, 8, pufrac,
                             (SDL_Color){40,40,60,255}, pc);
        ty += 54;
    }

    /* Divider */
    SET_COLOR(r, border);
    SDL_RenderDrawLine(r, sx+8, ty, sx+sw-8, ty);
    ty += 12;

    /* Controls hint */
    SB_LABEL("ARROWS / WASD : Move", 0, COL_TEXT_DIM);
    SB_LABEL("P / ESC  : Pause",    18, COL_TEXT_DIM);
    SB_LABEL("R        : Restart",  36, COL_TEXT_DIM);
    ty += 60;

    /* Player name */
    if (app->player_name[0]) {
        SB_LABEL("PLAYER", 0, COL_TEXT_DIM);
        SB_LABEL(app->player_name, 18, COL_TEXT);
    }

#undef SB_LABEL
#undef SB_VALUE
}

/* ══════════════════════════════════════════════════════════════════════
 * Semi-transparent overlay helper
 * ══════════════════════════════════════════════════════════════════════ */
static void draw_overlay(App *app)
{
    SDL_Renderer *r = app->renderer;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SET_COLOR(r, COL_OVERLAY);
    SDL_Rect full = {0,0,WINDOW_W,WINDOW_H};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ══════════════════════════════════════════════════════════════════════
 * GAME screen
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render_game(App *app)
{
    render_grid(app);
    render_obstacles(app);
    render_food(app);
    render_powerup(app);
    render_snake(app, &app->player);
    if (app->mode == MODE_AI && app->ai.alive)
        render_snake(app, &app->ai);
    render_particles(app);
    render_sidebar(app);
}

/* ══════════════════════════════════════════════════════════════════════
 * PAUSE overlay
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render_paused(App *app)
{
    ui_render_game(app);   /* render game underneath */
    draw_overlay(app);

    SDL_Renderer *r = app->renderer;
    int cx = BOARD_W/2, cy = WINDOW_H/2;

    ui_draw_rounded_rect(r, cx-160, cy-90, 320, 180, 12, COL_PANEL);
    ui_draw_rect_outline(r, cx-160, cy-90, 320, 180, COL_ACCENT, 2);
    ui_draw_text(app, app->font_lg, "PAUSED",  cx, cy-60, COL_ACCENT, 1);
    ui_draw_text(app, app->font_sm, "P / ESC  -  Resume", cx, cy-10, COL_TEXT, 1);
    ui_draw_text(app, app->font_sm, "R        -  Restart", cx, cy+16, COL_TEXT, 1);
    ui_draw_text(app, app->font_sm, "ESC      -  Main Menu", cx, cy+42, COL_TEXT_DIM, 1);
}

/* ══════════════════════════════════════════════════════════════════════
 * GAME OVER screen
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render_gameover(App *app)
{
    ui_render_game(app);
    draw_overlay(app);

    SDL_Renderer *r = app->renderer;
    int cx = BOARD_W/2, cy = WINDOW_H/2;

    ui_draw_rounded_rect(r, cx-190, cy-130, 380, 280, 14, COL_PANEL);
    ui_draw_rect_outline(r, cx-190, cy-130, 380, 280, (SDL_Color){255,80,80,255}, 2);

    ui_draw_text(app, app->font_lg, "GAME OVER", cx, cy-120, (SDL_Color){255,80,80,255}, 1);

    char buf[64];
    snprintf(buf,sizeof(buf),"Score:  %d", app->final_score);
    ui_draw_text(app, app->font_md, buf, cx, cy-60, COL_TEXT, 1);
    snprintf(buf,sizeof(buf),"Length: %d", app->final_length);
    ui_draw_text(app, app->font_md, buf, cx, cy-32, COL_TEXT, 1);
    snprintf(buf,sizeof(buf),"Time:   %.1fs", app->survive_time_s);
    ui_draw_text(app, app->font_md, buf, cx, cy-4, COL_TEXT, 1);

    if (app->final_score == app->high_score && app->final_score > 0)
        ui_draw_text(app, app->font_md, "* NEW HIGH SCORE! *", cx, cy+30, COL_TEXT_GOLD, 1);

    ui_draw_text(app, app->font_sm, "R  -  Play Again", cx, cy+80, COL_ACCENT, 1);
    ui_draw_text(app, app->font_sm, "ESC  -  Main Menu", cx, cy+105, COL_TEXT_DIM, 1);
}

/* ══════════════════════════════════════════════════════════════════════
 * MAIN MENU
 * ══════════════════════════════════════════════════════════════════════ */

static const char *MENU_ITEMS[] = {
    "Play Game", "Select Mode", "Leaderboard", "Exit"
};
#define MENU_COUNT 4

void ui_render_menu(App *app)
{
    SDL_Renderer *r = app->renderer;
    Uint32 t = app->ticks;

    /* Animated background gradient rows */
    for (int row=0; row<GRID_ROWS; row++) {
        for (int col=0; col<GRID_COLS; col++) {
            float wave = sinf((col + row + t*0.01f) * 0.4f) * 0.5f + 0.5f;
            Uint8 br = (Uint8)(18 + wave*10);
            SDL_SetRenderDrawColor(r, br, br, br+12, 255);
            SDL_Rect cell = {GRID_X_OFFSET+col*CELL_SIZE, GRID_Y_OFFSET+row*CELL_SIZE,
                             CELL_SIZE-1, CELL_SIZE-1};
            SDL_RenderFillRect(r, &cell);
        }
    }

    /* Title */
    int cx = WINDOW_W/2, ty = 100;
    float pulse = sinf(t*0.003f)*0.5f+0.5f;
    SDL_Color title_col = {
        (Uint8)(57+pulse*80), 255, (Uint8)(20+pulse*40), 255};
    ui_draw_text(app, app->font_lg, "SNAKE XENZIA", cx, ty, title_col, 1);

    SDL_Color sub = {120,120,150,255};
    ui_draw_text(app, app->font_sm, "Enhanced Edition", cx, ty+44, sub, 1);

    /* Mode indicator */
    char modebuf[64];
    snprintf(modebuf,sizeof(modebuf),"Mode: %s", mode_name(app->mode));
    ui_draw_text(app, app->font_sm, modebuf, cx, ty+70, COL_TEXT_DIM, 1);

    /* Menu buttons */
    int btn_w=260, btn_h=52, btn_gap=14;
    int bx = cx - btn_w/2;
    int start_y = 220;

    for (int i=0;i<MENU_COUNT;i++){
        int by = start_y + i*(btn_h+btn_gap);
        int sel = (app->menu_sel == i);
        SDL_Color bg  = sel ? COL_ACCENT : COL_PANEL;
        SDL_Color txt = sel ? COL_BLACK  : COL_TEXT;

        if (sel) {
            /* Glow behind selected button */
            SDL_Color glow = COL_ACCENT; glow.a=40;
            SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
            SDL_Rect gr2={bx-6,by-6,btn_w+12,btn_h+12};
            SET_COLOR(r,glow); SDL_RenderFillRect(r,&gr2);
            SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        }

        ui_draw_rounded_rect(r, bx, by, btn_w, btn_h, 8, bg);
        if (!sel) ui_draw_rect_outline(r,bx,by,btn_w,btn_h,COL_BORDER,1);
        ui_draw_text(app, app->font_md, MENU_ITEMS[i], cx, by+14, txt, 1);
    }

    /* Navigation hint */
    ui_draw_text(app, app->font_sm, "UP/DOWN: Navigate   ENTER: Select",
                 cx, WINDOW_H-36, COL_TEXT_DIM, 1);
}

/* ══════════════════════════════════════════════════════════════════════
 * MODE SELECT screen
 * ══════════════════════════════════════════════════════════════════════ */

static const char *MODE_NAMES[] = {
    "Classic", "Time Attack", "Survival", "AI Mode"
};
static const char *MODE_DESC[] = {
    "Standard gameplay. Eat food, grow, survive.",
    "90 seconds! Maximise your score.",
    "Obstacles appear over time. How long can you last?",
    "Watch the AI play — or challenge it."
};
#define MODE_COUNT 4

void ui_render_mode_select(App *app)
{
    SDL_Renderer *r = app->renderer;
    ui_draw_rect_filled(r,0,0,WINDOW_W,WINDOW_H,COL_BG);

    int cx = WINDOW_W/2;
    ui_draw_text(app, app->font_lg, "SELECT MODE", cx, 60, COL_ACCENT, 1);

    int btn_w=340, btn_h=70, btn_gap=16;
    int bx = cx-btn_w/2, start_y=140;

    for (int i=0;i<MODE_COUNT;i++){
        int by = start_y + i*(btn_h+btn_gap);
        int sel = (app->mode_sel == i);
        SDL_Color bg  = sel ? COL_ACCENT       : COL_PANEL;
        SDL_Color txt = sel ? COL_BLACK        : COL_TEXT;
        SDL_Color desc_col = sel ? (SDL_Color){18,18,30,200} : COL_TEXT_DIM;

        ui_draw_rounded_rect(r, bx, by, btn_w, btn_h, 10, bg);
        if (!sel) ui_draw_rect_outline(r,bx,by,btn_w,btn_h,COL_BORDER,1);

        ui_draw_text(app, app->font_md, MODE_NAMES[i], cx, by+8, txt, 1);
        ui_draw_text(app, app->font_sm, MODE_DESC[i],  cx, by+36, desc_col, 1);
    }

    ui_draw_text(app, app->font_sm, "UP/DOWN: Select   ENTER: Confirm   ESC: Back",
                 cx, WINDOW_H-36, COL_TEXT_DIM, 1);
}

/* ══════════════════════════════════════════════════════════════════════
 * LEADERBOARD screen
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render_leaderboard(App *app)
{
    SDL_Renderer *r = app->renderer;
    ui_draw_rect_filled(r,0,0,WINDOW_W,WINDOW_H,COL_BG);

    int cx = WINDOW_W/2;
    ui_draw_text(app, app->font_lg, "LEADERBOARD", cx, 40, COL_TEXT_GOLD, 1);

    /* Header row */
    int tx=160, ty=110, row_h=44;
    ui_draw_text(app, app->font_sm, "#",    tx,      ty, COL_TEXT_DIM, 0);
    ui_draw_text(app, app->font_sm, "NAME", tx+50,   ty, COL_TEXT_DIM, 0);
    ui_draw_text(app, app->font_sm, "SCORE",tx+260,  ty, COL_TEXT_DIM, 0);
    ui_draw_text(app, app->font_sm, "MODE", tx+380,  ty, COL_TEXT_DIM, 0);

    SDL_Color divCol={60,60,90,255};
    SET_COLOR(r,divCol);
    SDL_RenderDrawLine(r,140,ty+22, WINDOW_W-140, ty+22);

    if (app->lb.count == 0) {
        ui_draw_text(app, app->font_md, "No scores yet — play a game!",
                     cx, 200, COL_TEXT_DIM, 1);
    }

    SDL_Color medal[3] = {
        {255,215,0,255}, {192,192,192,255}, {205,127,50,255}
    };

    for (int i=0;i<app->lb.count;i++){
        int ry = ty + 30 + i*row_h;
        SDL_Color rc = (i<3) ? medal[i] : COL_TEXT;

        char rank[8]; snprintf(rank,sizeof(rank),"%d", i+1);
        char score[16]; snprintf(score,sizeof(score),"%d", app->lb.entries[i].score);

        /* Highlight top entry */
        if (i==0) {
            ui_draw_rect_filled(r, 138, ry-4, WINDOW_W-276, row_h-4,
                                (SDL_Color){40,38,10,255});
        }

        ui_draw_text(app, app->font_md, rank,                       tx,     ry, rc, 0);
        ui_draw_text(app, app->font_md, app->lb.entries[i].name,    tx+50,  ry, COL_TEXT, 0);
        ui_draw_text(app, app->font_md, score,                      tx+260, ry, COL_ACCENT, 0);
        ui_draw_text(app, app->font_sm, app->lb.entries[i].mode,    tx+380, ry+4, COL_TEXT_DIM, 0);
    }

    ui_draw_text(app, app->font_sm, "ESC / ENTER : Back",
                 cx, WINDOW_H-36, COL_TEXT_DIM, 1);
}

/* ══════════════════════════════════════════════════════════════════════
 * NAME ENTRY screen
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render_name_entry(App *app)
{
    SDL_Renderer *r = app->renderer;
    ui_draw_rect_filled(r,0,0,WINDOW_W,WINDOW_H,COL_BG);

    int cx = WINDOW_W/2, cy = WINDOW_H/2;
    ui_draw_text(app, app->font_lg, "ENTER YOUR NAME", cx, cy-120, COL_ACCENT, 1);
    ui_draw_text(app, app->font_sm, "This will appear on the leaderboard.",
                 cx, cy-70, COL_TEXT_DIM, 1);

    /* Input box */
    int bw=340, bh=56;
    int bx=cx-bw/2, by=cy-28;
    ui_draw_rounded_rect(r, bx, by, bw, bh, 8, COL_PANEL);
    ui_draw_rect_outline(r, bx, by, bw, bh, COL_ACCENT, 2);

    /* Text + cursor */
    char display[MAX_NAME_LEN+2];
    snprintf(display, sizeof(display), "%s", app->name_buf);
    /* Blinking cursor */
    if ((app->ticks/500)%2==0) strncat(display,"_",1);
    ui_draw_text(app, app->font_md, display, cx, by+14, COL_TEXT, 1);

    ui_draw_text(app, app->font_sm, "ENTER: Confirm   BACKSPACE: Delete",
                 cx, cy+60, COL_TEXT_DIM, 1);
    ui_draw_text(app, app->font_sm, "(Leave blank to skip)",
                 cx, cy+84, COL_TEXT_DIM, 1);
}

void ui_name_entry_key(App *app, SDL_Keycode key, const char *text)
{
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        strncpy(app->player_name, app->name_buf, MAX_NAME_LEN-1);
        game_init(app);
        return;
    }
    if (key == SDLK_BACKSPACE && app->name_len > 0) {
        app->name_buf[--app->name_len] = '\0';
        return;
    }
    if (key == SDLK_ESCAPE) {
        app->state = STATE_MENU;
        return;
    }
    /* Printable characters from SDL_TEXTINPUT event (handled in main) */
    (void)text;
}

/* ══════════════════════════════════════════════════════════════════════
 * Master render dispatcher
 * ══════════════════════════════════════════════════════════════════════ */

void ui_render(App *app)
{
    SDL_Renderer *r = app->renderer;

    /* Clear */
    SET_COLOR(r, COL_BG);
    SDL_RenderClear(r);

    switch(app->state) {
        case STATE_MENU:         ui_render_menu(app);         break;
        case STATE_MODE_SELECT:  ui_render_mode_select(app);  break;
        case STATE_PLAYING:      ui_render_game(app);         break;
        case STATE_PAUSED:       ui_render_paused(app);       break;
        case STATE_GAMEOVER:     ui_render_gameover(app);     break;
        case STATE_LEADERBOARD:  ui_render_leaderboard(app);  break;
        case STATE_NAME_ENTRY:   ui_render_name_entry(app);   break;
    }

    SDL_RenderPresent(r);
}

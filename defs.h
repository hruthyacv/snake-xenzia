#ifndef DEFS_H
#define DEFS_H

/*
 * defs.h — Master definitions for Snake Xenzia SDL2
 * All shared constants, enums, and structs live here.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ─── Window & Grid ─────────────────────────────────────────────────── */
#define WINDOW_W        900
#define WINDOW_H        700
#define SIDEBAR_W       220      /* right-hand UI panel */
#define GRID_COLS       30
#define GRID_ROWS       24
#define CELL_SIZE       ((WINDOW_W - SIDEBAR_W) / GRID_COLS)  /* 22px */
#define GRID_X_OFFSET   0
#define GRID_Y_OFFSET   ((WINDOW_H - GRID_ROWS * CELL_SIZE) / 2)

/* Derived board pixel dimensions */
#define BOARD_W         (GRID_COLS * CELL_SIZE)
#define BOARD_H         (GRID_ROWS * CELL_SIZE)

/* ─── Frame Rate ────────────────────────────────────────────────────── */
#define TARGET_FPS      60
#define FRAME_MS        (1000 / TARGET_FPS)

/* ─── Snake Speed (ms per move step) ───────────────────────────────── */
#define SPEED_NORMAL    140
#define SPEED_MIN        45     /* fastest */
#define SPEED_BOOST      60     /* power-up: speed boost */
#define SPEED_SLOW      260     /* power-up: slow motion */

/* ─── Scoring ───────────────────────────────────────────────────────── */
#define SCORE_FOOD          10
#define SCORE_COMBO_BONUS    5  /* per consecutive eat */
#define COMBO_RESET_TICKS   10  /* steps without eating resets combo */

/* ─── Power-Up ──────────────────────────────────────────────────────── */
#define POWERUP_SPAWN_CHANCE  8   /* 1-in-N per food eaten */
#define POWERUP_LIFETIME_MS   8000
#define POWERUP_EFFECT_MS     6000
#define SHRINK_AMOUNT         4

/* ─── Obstacle (Survival mode) ──────────────────────────────────────── */
#define MAX_OBSTACLES    40
#define OBS_SPAWN_FOOD    5   /* spawn an obstacle every N food eaten */

/* ─── Leaderboard ───────────────────────────────────────────────────── */
#define SCORES_FILE       "snake_scores.dat"
#define MAX_SCORES        10
#define MAX_NAME_LEN      20

/* ─── Time Attack ───────────────────────────────────────────────────── */
#define TIME_ATTACK_SECS  90

/* ─── Color Palette ─────────────────────────────────────────────────── */
/* Background */
#define COL_BG          (SDL_Color){18,  18,  30,  255}
#define COL_GRID        (SDL_Color){28,  28,  44,  255}
#define COL_BORDER      (SDL_Color){60,  60,  100, 255}
/* Snake */
#define COL_SNAKE_HEAD  (SDL_Color){57,  255, 20,  255}   /* neon green */
#define COL_SNAKE_BODY  (SDL_Color){39,  174, 96,  255}
#define COL_SNAKE_GLOW  (SDL_Color){57,  255, 20,  80 }
#define COL_AI_HEAD     (SDL_Color){255, 80,  80,  255}
#define COL_AI_BODY     (SDL_Color){180, 50,  50,  255}
/* Food */
#define COL_FOOD        (SDL_Color){255, 80,  80,  255}
#define COL_FOOD_GLOW   (SDL_Color){255, 80,  80,  80 }
/* Power-ups */
#define COL_PU_SPEED    (SDL_Color){255, 200, 0,   255}
#define COL_PU_SLOW     (SDL_Color){100, 180, 255, 255}
#define COL_PU_DOUBLE   (SDL_Color){200, 100, 255, 255}
#define COL_PU_SHRINK   (SDL_Color){255, 130, 0,   255}
/* Obstacle */
#define COL_OBSTACLE    (SDL_Color){120, 120, 150, 255}
/* UI */
#define COL_SIDEBAR     (SDL_Color){22,  22,  38,  255}
#define COL_PANEL       (SDL_Color){30,  30,  50,  255}
#define COL_ACCENT      (SDL_Color){57,  255, 20,  255}
#define COL_TEXT        (SDL_Color){220, 220, 240, 255}
#define COL_TEXT_DIM    (SDL_Color){120, 120, 150, 255}
#define COL_TEXT_GOLD   (SDL_Color){255, 215, 0,   255}
#define COL_HEALTH_BAR  (SDL_Color){57,  255, 20,  255}
#define COL_TIME_BAR    (SDL_Color){100, 180, 255, 255}
#define COL_WHITE       (SDL_Color){255, 255, 255, 255}
#define COL_BLACK       (SDL_Color){0,   0,   0,   255}
#define COL_OVERLAY     (SDL_Color){0,   0,   0,   180}

/* ─── Helper Macros ─────────────────────────────────────────────────── */
#define SET_COLOR(r, col) SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a)
#define CLAMP(v, lo, hi)  ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

/* ─── Direction ─────────────────────────────────────────────────────── */
typedef enum { DIR_NONE=0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

/* ─── Game State Machine ─────────────────────────────────────────────── */
typedef enum {
    STATE_MENU = 0,
    STATE_MODE_SELECT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_LEADERBOARD,
    STATE_NAME_ENTRY
} GameState;

/* ─── Game Mode ─────────────────────────────────────────────────────── */
typedef enum {
    MODE_CLASSIC    = 0,
    MODE_TIME_ATTACK,
    MODE_SURVIVAL,
    MODE_AI
} GameMode;

/* ─── Power-Up Types ─────────────────────────────────────────────────── */
typedef enum {
    PU_NONE    = 0,
    PU_SPEED   = 1,
    PU_SLOW    = 2,
    PU_DOUBLE  = 3,
    PU_SHRINK  = 4
} PowerUpType;

/* ─── 2-D Integer Point ─────────────────────────────────────────────── */
typedef struct { int x, y; } Point;

/* ─── Snake Segment ─────────────────────────────────────────────────── */
typedef struct {
    Point     body[GRID_COLS * GRID_ROWS];
    int       length;
    Direction dir;
    Direction next_dir;      /* buffered input */
    int       alive;
    int       is_ai;
    /* Visual interpolation (fraction 0.0–1.0 between last/next cell) */
    float     anim_t;
    Point     prev_head;     /* head position last step (for lerp) */
} Snake;

/* ─── Power-Up on Board ─────────────────────────────────────────────── */
typedef struct {
    PowerUpType type;
    Point       pos;
    int         active;           /* 1 = visible on board */
    Uint32      spawn_time_ms;    /* SDL_GetTicks when spawned */
    float       pulse;            /* animation phase */
} BoardPowerUp;

/* ─── Active Effect on Snake ─────────────────────────────────────────── */
typedef struct {
    PowerUpType type;
    Uint32      end_time_ms;
} ActiveEffect;

/* ─── Score Entry ────────────────────────────────────────────────────── */
typedef struct {
    char name[MAX_NAME_LEN];
    int  score;
    char mode[16];
} ScoreEntry;

/* ─── Leaderboard ────────────────────────────────────────────────────── */
typedef struct {
    ScoreEntry entries[MAX_SCORES];
    int        count;
} Leaderboard;

/* ─── Particle (visual effect) ──────────────────────────────────────── */
typedef struct {
    float x, y;
    float vx, vy;
    float life;      /* 1.0 = fresh, 0.0 = dead */
    float size;
    SDL_Color color;
} Particle;

#define MAX_PARTICLES 256

/* ─── Main App Context ──────────────────────────────────────────────── */
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font_lg;    /* 36pt */
    TTF_Font     *font_md;    /* 22pt */
    TTF_Font     *font_sm;    /* 16pt */

    /* Audio */
    Mix_Music *music;
    Mix_Chunk *sfx_eat;
    Mix_Chunk *sfx_die;
    Mix_Chunk *sfx_powerup;

    /* State */
    GameState    state;
    GameMode     mode;
    int          running;

    /* Game data */
    Snake        player;
    Snake        ai;
    Point        food;
    Point        obstacles[MAX_OBSTACLES];
    int          obs_count;
    BoardPowerUp board_pu;
    ActiveEffect active_fx;

    /* Scoring */
    int          score;
    int          high_score;
    int          combo;
    int          combo_timer;
    int          food_eaten;    /* total this session */

    /* Timing */
    Uint32       last_move_ms;
    int          move_interval_ms;
    Uint32       game_start_ms;
    int          time_remaining_s;
    Uint32       last_sec_ms;

    /* Particles */
    Particle     particles[MAX_PARTICLES];
    int          particle_count;

    /* Leaderboard / name entry */
    Leaderboard  lb;
    char         player_name[MAX_NAME_LEN];
    char         name_buf[MAX_NAME_LEN];
    int          name_len;

    /* Game over stats */
    int          final_score;
    int          final_length;
    float        survive_time_s;

    /* UI animation */
    Uint32       ticks;           /* SDL_GetTicks snapshot per frame */
    float        food_pulse;
    int          menu_sel;        /* highlighted menu item */
    int          mode_sel;

    /* Survival */
    int          survival_food_count;
} App;

#endif /* DEFS_H */

/*
 * game.c — Core game logic for Snake Xenzia SDL2.
 *
 * Handles: game initialisation, per-frame update, input routing,
 * collision detection, scoring, power-up effects, and game-end flow.
 */
#include "game.h"
#include "snake.h"
#include "ai.h"
#include "fileio.h"

const char *mode_name(GameMode m) {
    switch(m){
        case MODE_CLASSIC:     return "Classic";
        case MODE_TIME_ATTACK: return "Time Attack";
        case MODE_SURVIVAL:    return "Survival";
        case MODE_AI:          return "AI Mode";
        default:               return "Unknown";
    }
}

/* ── Helpers ─────────────────────────────────────────────────────────── */

static int pt_eq(Point a, Point b){ return a.x==b.x && a.y==b.y; }

static int cell_free(App *app, Point p)
{
    if (p.x<0||p.x>=GRID_COLS||p.y<0||p.y>=GRID_ROWS) return 0;
    if (snake_occupies(&app->player, p)) return 0;
    if (app->mode==MODE_AI && snake_occupies(&app->ai, p)) return 0;
    if (pt_eq(p, app->food)) return 0;
    for (int i=0;i<app->obs_count;i++) if(pt_eq(p,app->obstacles[i])) return 0;
    if (app->board_pu.active && pt_eq(p, app->board_pu.pos)) return 0;
    return 1;
}

static Point random_free(App *app)
{
    Point p;
    int tries=0;
    do {
        p.x = rand() % GRID_COLS;
        p.y = rand() % GRID_ROWS;
        tries++;
    } while (!cell_free(app, p) && tries < 2000);
    return p;
}

/* ── Particle emitter ─────────────────────────────────────────────────── */
static void emit_particles(App *app, float px, float py, SDL_Color col, int n)
{
    for (int i=0;i<n;i++){
        if (app->particle_count >= MAX_PARTICLES) break;
        Particle *p = &app->particles[app->particle_count++];
        p->x = px; p->y = py;
        float angle = ((float)(rand()%360)) * 3.14159f / 180.0f;
        float speed = 1.0f + (rand()%300)/100.0f;
        p->vx = cosf(angle)*speed;
        p->vy = sinf(angle)*speed;
        p->life = 1.0f;
        p->size = 3.0f + (rand()%4);
        p->color = col;
    }
}

/* ── Spawning ─────────────────────────────────────────────────────────── */

void game_spawn_food(App *app)
{
    app->food = random_free(app);
}

void game_spawn_powerup(App *app)
{
    if (app->board_pu.active) return;
    if ((rand() % POWERUP_SPAWN_CHANCE) != 0) return;
    Point p = random_free(app);
    PowerUpType types[4] = {PU_SPEED, PU_SLOW, PU_DOUBLE, PU_SHRINK};
    app->board_pu.type = types[rand()%4];
    app->board_pu.pos  = p;
    app->board_pu.active = 1;
    app->board_pu.spawn_time_ms = app->ticks;
    app->board_pu.pulse = 0.0f;
}

void game_spawn_obstacle(App *app)
{
    if (app->obs_count >= MAX_OBSTACLES) return;
    Point p = random_free(app);
    app->obstacles[app->obs_count++] = p;
}

/* ── Power-up application ─────────────────────────────────────────────── */

void game_apply_powerup(App *app, PowerUpType t)
{
    app->active_fx.type = t;
    app->active_fx.end_time_ms = app->ticks + POWERUP_EFFECT_MS;

    switch(t) {
        case PU_SPEED:
            app->move_interval_ms = SPEED_BOOST;
            break;
        case PU_SLOW:
            app->move_interval_ms = SPEED_SLOW;
            break;
        case PU_DOUBLE:
            /* handled at score-time */
            break;
        case PU_SHRINK:
            snake_shrink(&app->player, SHRINK_AMOUNT);
            break;
        default: break;
    }

    if (app->sfx_powerup) Mix_PlayChannel(-1, app->sfx_powerup, 0);
    float px = (app->board_pu.pos.x + 0.5f) * CELL_SIZE;
    float py = GRID_Y_OFFSET + (app->board_pu.pos.y + 0.5f) * CELL_SIZE;
    SDL_Color col = {200,100,255,255};
    emit_particles(app, px, py, col, 20);
}

/* ── Game initialisation ─────────────────────────────────────────────── */

void game_init(App *app)
{
    srand((unsigned)time(NULL) ^ SDL_GetTicks());

    /* Reset game data */
    app->score             = 0;
    app->combo             = 0;
    app->combo_timer       = 0;
    app->food_eaten        = 0;
    app->obs_count         = 0;
    app->particle_count    = 0;
    app->survival_food_count = 0;
    app->food_pulse        = 0.0f;
    app->board_pu.active   = 0;
    app->active_fx.type    = PU_NONE;
    app->active_fx.end_time_ms = 0;

    /* Speed */
    app->move_interval_ms  = SPEED_NORMAL;
    app->last_move_ms      = SDL_GetTicks();
    app->game_start_ms     = SDL_GetTicks();
    app->last_sec_ms       = SDL_GetTicks();
    app->time_remaining_s  = TIME_ATTACK_SECS;

    /* Player snake — starts at left-centre */
    snake_init(&app->player, GRID_COLS/5, GRID_ROWS/2, DIR_RIGHT, 4, 0);

    /* AI snake — starts at right-centre (AI mode only) */
    if (app->mode == MODE_AI) {
        snake_init(&app->ai, GRID_COLS*4/5, GRID_ROWS/2, DIR_LEFT, 4, 1);
    } else {
        app->ai.alive = 0;
    }

    /* Spawn some obstacles for Survival mode */
    if (app->mode == MODE_SURVIVAL) {
        for (int i=0; i<4; i++) game_spawn_obstacle(app);
    }

    game_spawn_food(app);
    app->state = STATE_PLAYING;
}

/* ── Speed scaling ────────────────────────────────────────────────────── */

static void update_speed(App *app)
{
    /* Only adjust if no speed power-up is active */
    if (app->active_fx.type == PU_SPEED || app->active_fx.type == PU_SLOW) return;

    int steps = app->score / 50;   /* get faster every 50 pts */
    int interval = SPEED_NORMAL - steps * 8;
    if (interval < SPEED_MIN) interval = SPEED_MIN;

    /* Survival: extra ramp */
    if (app->mode == MODE_SURVIVAL) {
        interval -= app->obs_count * 3;
        if (interval < SPEED_MIN) interval = SPEED_MIN;
    }
    app->move_interval_ms = interval;
}

/* ── Input ────────────────────────────────────────────────────────────── */

void game_handle_key(App *app, SDL_Keycode key)
{
    if (app->state == STATE_PLAYING) {
        switch(key) {
            case SDLK_UP:    case SDLK_w: snake_set_dir(&app->player, DIR_UP);    break;
            case SDLK_DOWN:  case SDLK_s: snake_set_dir(&app->player, DIR_DOWN);  break;
            case SDLK_LEFT:  case SDLK_a: snake_set_dir(&app->player, DIR_LEFT);  break;
            case SDLK_RIGHT: case SDLK_d: snake_set_dir(&app->player, DIR_RIGHT); break;
            case SDLK_ESCAPE: case SDLK_p:
                app->state = STATE_PAUSED;
                break;
            default: break;
        }
    } else if (app->state == STATE_PAUSED) {
        if (key==SDLK_ESCAPE||key==SDLK_p) app->state = STATE_PLAYING;
        if (key==SDLK_r) { game_init(app); }
    } else if (app->state == STATE_GAMEOVER) {
        if (key==SDLK_r) game_init(app);
        if (key==SDLK_ESCAPE) app->state = STATE_MENU;
    }
}

/* ── Main update tick ─────────────────────────────────────────────────── */

void game_end(App *app)
{
    app->final_score  = app->score;
    app->final_length = app->player.length;
    app->survive_time_s = (float)(app->ticks - app->game_start_ms) / 1000.0f;

    /* Update high score */
    if (app->score > app->high_score) app->high_score = app->score;

    /* Insert into leaderboard if name is set */
    if (strlen(app->player_name) > 0) {
        ScoreEntry e;
        strncpy(e.name, app->player_name, MAX_NAME_LEN-1);
        e.name[MAX_NAME_LEN-1]='\0';
        e.score = app->score;
        strncpy(e.mode, mode_name(app->mode), 15);
        fileio_insert(&app->lb, &e);
        fileio_save(&app->lb);
    }

    if (app->sfx_die) Mix_PlayChannel(-1, app->sfx_die, 0);
    app->state = STATE_GAMEOVER;
}

void game_update(App *app)
{
    app->ticks = SDL_GetTicks();

    /* ── Animate particles ── */
    for (int i=0; i<app->particle_count; ) {
        Particle *p = &app->particles[i];
        p->x    += p->vx;
        p->y    += p->vy;
        p->vy   += 0.08f;        /* gravity */
        p->life -= 0.025f;
        if (p->life <= 0.0f) {
            app->particles[i] = app->particles[--app->particle_count];
        } else { i++; }
    }

    /* ── Power-up lifetime on board ── */
    if (app->board_pu.active) {
        if (app->ticks - app->board_pu.spawn_time_ms > POWERUP_LIFETIME_MS)
            app->board_pu.active = 0;
        app->board_pu.pulse = (float)sin(app->ticks * 0.006) * 0.5f + 0.5f;
    }

    /* ── Active effect expiry ── */
    if (app->active_fx.type != PU_NONE && app->ticks >= app->active_fx.end_time_ms) {
        app->active_fx.type = PU_NONE;
        update_speed(app);
    }

    /* ── Time Attack countdown ── */
    if (app->mode == MODE_TIME_ATTACK) {
        if (app->ticks - app->last_sec_ms >= 1000) {
            app->last_sec_ms = app->ticks;
            app->time_remaining_s--;
        }
        if (app->time_remaining_s <= 0) {
            game_end(app); return;
        }
    }

    /* ── Food animation ── */
    app->food_pulse = (float)sin(app->ticks * 0.005) * 0.5f + 0.5f;

    /* ── Smooth animation interpolation ── */
    float dt = 1.0f / TARGET_FPS;
    float step_time = (float)app->move_interval_ms / 1000.0f;
    app->player.anim_t += dt / step_time;
    if (app->player.anim_t > 1.0f) app->player.anim_t = 1.0f;
    if (app->ai.alive) {
        app->ai.anim_t += dt / step_time;
        if (app->ai.anim_t > 1.0f) app->ai.anim_t = 1.0f;
    }

    /* ── Move step ── */
    if ((int)(app->ticks - app->last_move_ms) < app->move_interval_ms) return;
    app->last_move_ms = app->ticks;

    /* ── Combo timer ── */
    app->combo_timer++;
    if (app->combo_timer > COMBO_RESET_TICKS) {
        app->combo = 0; app->combo_timer = 0;
    }

    /* ── AI thinks ── */
    if (app->mode == MODE_AI && app->ai.alive) {
        ai_think(&app->ai, &app->player, app->food,
                 app->obstacles, app->obs_count);
    }

    /* ── Compute next head position ── */
    Point next = snake_next_head(&app->player);

    /* ── Wall collision ── */
    if (next.x<0||next.x>=GRID_COLS||next.y<0||next.y>=GRID_ROWS) {
        game_end(app); return;
    }

    /* ── Self collision ── */
    if (snake_body_hit(&app->player, next)) {
        game_end(app); return;
    }

    /* ── Obstacle collision ── */
    for (int i=0;i<app->obs_count;i++) {
        if (pt_eq(next, app->obstacles[i])) {
            game_end(app); return;
        }
    }

    /* ── Food check ── */
    int ate = pt_eq(next, app->food);

    /* Move player */
    snake_step(&app->player, ate);

    if (ate) {
        app->food_eaten++;
        app->combo++;
        app->combo_timer = 0;

        int pts = SCORE_FOOD + (app->combo-1) * SCORE_COMBO_BONUS;
        if (app->active_fx.type == PU_DOUBLE) pts *= 2;
        app->score += pts;
        if (app->score > app->high_score) app->high_score = app->score;

        if (app->sfx_eat) Mix_PlayChannel(-1, app->sfx_eat, 0);

        /* Particles at food position */
        float px = (app->food.x + 0.5f) * CELL_SIZE;
        float py = GRID_Y_OFFSET + (app->food.y + 0.5f) * CELL_SIZE;
        emit_particles(app, px, py, COL_FOOD, 16);

        game_spawn_food(app);
        game_spawn_powerup(app);
        update_speed(app);

        /* Survival: add obstacle every N food */
        if (app->mode == MODE_SURVIVAL) {
            app->survival_food_count++;
            if (app->survival_food_count % OBS_SPAWN_FOOD == 0)
                game_spawn_obstacle(app);
        }
    }

    /* ── Power-up pickup ── */
    if (app->board_pu.active && pt_eq(app->player.body[0], app->board_pu.pos)) {
        game_apply_powerup(app, app->board_pu.type);
        app->board_pu.active = 0;
    }

    /* ── AI snake update ── */
    if (app->mode == MODE_AI && app->ai.alive) {
        Point ai_next = snake_next_head(&app->ai);
        int ai_ok = 1;
        if (ai_next.x<0||ai_next.x>=GRID_COLS||ai_next.y<0||ai_next.y>=GRID_ROWS) ai_ok=0;
        if (ai_ok && snake_body_hit(&app->ai, ai_next)) ai_ok=0;
        if (ai_ok && snake_occupies(&app->player, ai_next)) ai_ok=0;
        for (int i=0;i<app->obs_count&&ai_ok;i++)
            if (pt_eq(ai_next,app->obstacles[i])) ai_ok=0;

        if (!ai_ok) { app->ai.alive = 0; }
        else {
            int ai_ate = pt_eq(ai_next, app->food);
            snake_step(&app->ai, ai_ate);
            if (ai_ate) {
                game_spawn_food(app);
                update_speed(app);
            }
        }
    }
}

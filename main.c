/*
 * main.c — Snake Xenzia SDL2 Entry Point
 *
 * Responsibilities:
 *   • SDL2 / TTF / Mixer initialisation and teardown
 *   • Main event loop (60 FPS capped with delta-time)
 *   • Keyboard + text-input event routing to game / ui handlers
 *   • Audio loading (graceful fallback if files missing)
 *   • Menu navigation (arrow keys + enter)
 */

#include "defs.h"
#undef main
/* On Windows SDL redefines main->SDL_main; undo that so our main() is used */
#ifdef _WIN32
#undef main
#endif
#include "game.h"
#include "ui.h"
#include "fileio.h"

/* ══════════════════════════════════════════════════════════════════════
 * Audio helpers — loads .wav files; silently skips if absent
 * ══════════════════════════════════════════════════════════════════════ */

static void audio_init(App *app)
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(8);

    app->music      = Mix_LoadMUS("assets/sounds/music.wav");
    app->sfx_eat    = Mix_LoadWAV("assets/sounds/eat.wav");
    app->sfx_die    = Mix_LoadWAV("assets/sounds/die.wav");
    app->sfx_powerup= Mix_LoadWAV("assets/sounds/powerup.wav");

    if (app->music) {
        Mix_VolumeMusic(40);
        Mix_PlayMusic(app->music, -1);   /* loop forever */
    }
    if (app->sfx_eat)     Mix_VolumeChunk(app->sfx_eat,     80);
    if (app->sfx_die)     Mix_VolumeChunk(app->sfx_die,     90);
    if (app->sfx_powerup) Mix_VolumeChunk(app->sfx_powerup, 70);
}

static void audio_free(App *app)
{
    if (app->music)       Mix_FreeMusic(app->music);
    if (app->sfx_eat)     Mix_FreeChunk(app->sfx_eat);
    if (app->sfx_die)     Mix_FreeChunk(app->sfx_die);
    if (app->sfx_powerup) Mix_FreeChunk(app->sfx_powerup);
    Mix_CloseAudio();
}

/* ══════════════════════════════════════════════════════════════════════
 * SDL2 init / teardown
 * ══════════════════════════════════════════════════════════════════════ */

static int app_init(App *app)
{
    memset(app, 0, sizeof(*app));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 0;
    }
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        return 0;
    }

    app->window = SDL_CreateWindow(
        "Snake Xenzia — Enhanced Edition",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!app->window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 0;
    }

    app->renderer = SDL_CreateRenderer(
        app->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!app->renderer) {
        /* Fallback: software renderer */
        app->renderer = SDL_CreateRenderer(app->window, -1,
                                           SDL_RENDERER_SOFTWARE);
        if (!app->renderer) {
            SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
            return 0;
        }
    }

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    ui_init_fonts(app);
    audio_init(app);
    fileio_load(&app->lb);

    /* Load persisted high score */
    for (int i = 0; i < app->lb.count; i++)
        if (app->lb.entries[i].score > app->high_score)
            app->high_score = app->lb.entries[i].score;

    /* Default state */
    app->state    = STATE_MENU;
    app->mode     = MODE_CLASSIC;
    app->mode_sel = 0;
    app->menu_sel = 0;
    app->running  = 1;
    app->player_name[0] = '\0';
    app->name_buf[0]    = '\0';
    app->name_len       = 0;
    app->ticks    = SDL_GetTicks();

    return 1;
}

static void app_free(App *app)
{
    ui_free_fonts(app);
    audio_free(app);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window)   SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

/* ══════════════════════════════════════════════════════════════════════
 * Menu navigation
 * ══════════════════════════════════════════════════════════════════════ */

/* Main menu: 0=Play, 1=Mode, 2=Leaderboard, 3=Exit */
static void menu_key(App *app, SDL_Keycode key)
{
    int items = 4;
    switch (key) {
        case SDLK_UP:   app->menu_sel = (app->menu_sel - 1 + items) % items; break;
        case SDLK_DOWN: app->menu_sel = (app->menu_sel + 1) % items;          break;
        case SDLK_RETURN:
        // fallthrough to ENTER
            case SDLK_KP_ENTER:
            switch (app->menu_sel) {
                case 0:   /* Play */
                    app->state    = STATE_NAME_ENTRY;
                    app->name_buf[0] = '\0';
                    app->name_len    = 0;
                    break;
                case 1:   /* Select Mode */
                    app->mode_sel = (int)app->mode;
                    app->state    = STATE_MODE_SELECT;
                    break;
                case 2:   /* Leaderboard */
                    app->state = STATE_LEADERBOARD;
                    break;
                case 3:   /* Exit */
                    app->running = 0;
                    break;
            }
            break;
        case SDLK_ESCAPE:
            app->running = 0;
            break;
        default: break;
    }
}

static void mode_select_key(App *app, SDL_Keycode key)
{
    int modes = 4;
    switch (key) {
        case SDLK_UP:
            app->mode_sel = (app->mode_sel - 1 + modes) % modes;
            break;
        case SDLK_DOWN:
            app->mode_sel = (app->mode_sel + 1) % modes;
            break;
        case SDLK_RETURN:
        // fallthrough to ENTER
            case SDLK_KP_ENTER:
            app->mode  = (GameMode)app->mode_sel;
            app->state = STATE_MENU;
            break;
        case SDLK_ESCAPE:
            app->state = STATE_MENU;
            break;
        default: break;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * Text input for name entry
 * ══════════════════════════════════════════════════════════════════════ */

static void name_entry_text(App *app, const char *text)
{
    /* Append printable chars up to MAX_NAME_LEN-1 */
    for (int i = 0; text[i] && app->name_len < MAX_NAME_LEN-1; i++) {
        char c = text[i];
        if (c >= 32 && c < 127) {
            app->name_buf[app->name_len++] = c;
            app->name_buf[app->name_len]   = '\0';
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * Main event loop
 * ══════════════════════════════════════════════════════════════════════ */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    App app;
    if (!app_init(&app)) {
        fprintf(stderr, "Fatal: app_init failed.\n");
        return 1;
    }

    /* Enable text input for name entry */
    SDL_StartTextInput();

    Uint32 frame_start, frame_elapsed;

    while (app.running) {
        frame_start = SDL_GetTicks();
        app.ticks   = frame_start;

        /* ── Event handling ── */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {

                case SDL_QUIT:
                    app.running = 0;
                    break;

                case SDL_KEYDOWN: {
                    SDL_Keycode key = ev.key.keysym.sym;

                    switch (app.state) {
                        case STATE_MENU:
                            menu_key(&app, key);
                            break;

                        case STATE_MODE_SELECT:
                            mode_select_key(&app, key);
                            break;

                        case STATE_NAME_ENTRY:
                            if (key == SDLK_BACKSPACE && app.name_len > 0) {
                                app.name_buf[--app.name_len] = '\0';
                            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                                strncpy(app.player_name, app.name_buf,
                                        MAX_NAME_LEN-1);
                                app.player_name[MAX_NAME_LEN-1] = '\0';
                                game_init(&app);
                            } else if (key == SDLK_ESCAPE) {
                                app.state = STATE_MENU;
                            }
                            break;

                        case STATE_PLAYING:
                        case STATE_PAUSED:
                            game_handle_key(&app, key);
                            /* Restart from pause goes to name entry */
                            if (key == SDLK_ESCAPE && app.state == STATE_PAUSED)
                                app.state = STATE_MENU;
                            break;

                        case STATE_GAMEOVER:
                            if (key == SDLK_r) {
                                game_init(&app);
                            } else if (key == SDLK_ESCAPE) {
                                app.state = STATE_MENU;
                            }
                            break;

                        case STATE_LEADERBOARD:
                            if (key == SDLK_ESCAPE || key == SDLK_RETURN ||
                                key == SDLK_KP_ENTER)
                                app.state = STATE_MENU;
                            break;

                        default: break;
                    }
                    break;
                }

                case SDL_TEXTINPUT:
                    if (app.state == STATE_NAME_ENTRY)
                        name_entry_text(&app, ev.text.text);
                    break;

                default: break;
            }
        }

        /* ── Game update (only while playing) ── */
        if (app.state == STATE_PLAYING)
            game_update(&app);

        /* ── Render ── */
        ui_render(&app);

        /* ── Frame cap: target 60 FPS ── */
        frame_elapsed = SDL_GetTicks() - frame_start;
        if (frame_elapsed < FRAME_MS)
            SDL_Delay(FRAME_MS - frame_elapsed);
    }

    SDL_StopTextInput();
    app_free(&app);
    return 0;
}
# 🐍 Snake Xenzia — Enhanced SDL2 Edition

A polished, modern Snake game built in **C + SDL2** for Windows.  
Features smooth 60 FPS rendering, 4 game modes, power-ups, BFS AI, particle effects, synthesised audio, and a persistent leaderboard.

---

## 📁 Project Structure

```
snake_xenzia_sdl/
├── main.c            Entry point, SDL2 init, event loop, menu navigation
├── game.c / game.h   Game logic: update, collision, scoring, power-ups
├── snake.c / snake.h Snake entity: movement, body, interpolation
├── ai.c   / ai.h     AI pathfinding: BFS + greedy fallback
├── ui.c   / ui.h     All SDL2 rendering: board, menus, HUD, particles
├── fileio.c/ fileio.h Leaderboard binary persistence
├── defs.h            Shared constants, enums, structs, colour palette
├── gen_sounds.c      Standalone tool — generates WAV audio assets
├── Makefile          Cross-platform build (Windows MSYS2 / Linux / macOS)
└── assets/
    └── sounds/
        ├── eat.wav       Food eaten SFX
        ├── die.wav       Collision / death SFX
        ├── powerup.wav   Power-up collected SFX
        └── music.wav     Background music (looped)
```

---

## ⚙️ Windows Setup (MSYS2 — Recommended)

### Step 1 — Install MSYS2

Download and install from **https://www.msys2.org**  
Open the **"MSYS2 MinGW 64-bit"** terminal (not MSYS2 MSYS).

### Step 2 — Install GCC + SDL2

```bash
# Update package database first
pacman -Syu

# Install compiler + SDL2 libraries
pacman -S mingw-w64-x86_64-gcc \
           mingw-w64-x86_64-SDL2 \
           mingw-w64-x86_64-SDL2_ttf \
           mingw-w64-x86_64-SDL2_mixer \
           make
```

### Step 3 — Generate Sound Assets (one time only)

```bash
cd /c/Users/YourName/snake_xenzia_sdl

gcc -o gen_sounds gen_sounds.c -lm
./gen_sounds
# Writes assets/sounds/eat.wav, die.wav, powerup.wav, music.wav
```

### Step 4 — Build the Game

```bash
make
```

Expected output:
```
  ✓  Build complete:  snake_xenzia.exe
  Run with:  ./snake_xenzia   (or  make run)
```

### Step 5 — Run

```bash
./snake_xenzia.exe
# or
make run
```

> **Note:** The game window opens automatically. A console window may also appear — to suppress it in a release build, `-mwindows` is already set in the Makefile.

---

## 🔨 Build Commands

| Command       | Action                          |
|---------------|----------------------------------|
| `make`        | Build release binary             |
| `make run`    | Build and launch immediately     |
| `make debug`  | Build with `-g` (no optimisation, console visible) |
| `make clean`  | Remove `.o`, binary, score file  |

### Manual compile (no make):
```bash
gcc -std=c99 -O2 -o snake_xenzia.exe \
    main.c game.c snake.c ai.c ui.c fileio.c \
    $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_mixer) \
    -lm -mwindows
```

---

## 🎮 Controls

| Key             | Action              |
|-----------------|---------------------|
| Arrow Keys / WASD | Move snake        |
| P / ESC         | Pause / Resume      |
| R               | Restart game        |
| ESC (in menu)   | Back / Quit         |
| Enter           | Confirm selection   |
| Up / Down       | Navigate menus      |

---

## 🎯 Game Modes

| Mode         | Description                                          |
|--------------|------------------------------------------------------|
| **Classic**     | Standard Snake. Eat, grow, avoid yourself.        |
| **Time Attack** | 90 seconds. Maximise score before time runs out.  |
| **Survival**    | Obstacles appear every 5 food eaten. Speed ramps. |
| **AI Mode**     | Watch BFS pathfinding AI play in real-time.       |

---

## ⚡ Power-Ups

| Glyph | Name           | Effect                          | Duration |
|-------|----------------|---------------------------------|----------|
| SPD   | Speed Boost    | Move interval cut to 60ms       | 6 sec    |
| SLW   | Slow Motion    | Move interval raised to 260ms   | 6 sec    |
| X2    | Double Score   | Food points doubled             | 6 sec    |
| SHR   | Shrink         | Removes 4 body segments         | Instant  |

Power-ups spawn randomly (1-in-8 chance per food eaten) and disappear after 8 seconds if uncollected.

---

## 🧩 Architecture Notes

### Smooth Snake Movement
The snake's head is **linearly interpolated** between its previous cell and its new cell every render frame using `anim_t` (0→1 per step). This gives fluid motion at 60 FPS even at slower move intervals.

### BFS AI (ai.c)
- Runs full BFS from AI head toward food each step
- Avoids: walls, own body, player body, obstacles
- Falls back to greedy Manhattan heuristic when food is unreachable
- Falls back to "any safe move" if completely surrounded

### Particle System
- Up to 256 simultaneous particles
- Emitted on food eat (red burst) and power-up collect (purple burst)
- Per-particle gravity, alpha fade, size shrink over lifetime

### Leaderboard
- Binary file `snake_scores.dat` with magic header `"SNK2"`
- Top 10 entries, sorted descending by score
- Tagged with player name and game mode

### Audio
- All WAV files generated by `gen_sounds.c` using pure PCM math (no external tools)
- Music: 16-second synthesised chiptune loop (bass + pad + kick + hi-hat)
- SDL_mixer handles looping and multichannel SFX

---

## 🐛 Common Errors & Fixes

### `SDL2/SDL.h: No such file or directory`
```bash
pacman -S mingw-w64-x86_64-SDL2
```
Ensure you are in the **MinGW 64-bit** terminal, not the MSYS2 terminal.

### `SDL2_ttf not found`
```bash
pacman -S mingw-w64-x86_64-SDL2_ttf
```

### `SDL2_mixer not found`
```bash
pacman -S mingw-w64-x86_64-SDL2_mixer
```

### Game launches but no text renders
The game looks for fonts in this order:
1. `assets/font.ttf` (place your own TTF here)
2. `C:\Windows\Fonts\consola.ttf`
3. `C:\Windows\Fonts\cour.ttf`
4. `C:\Windows\Fonts\arial.ttf`

If none are found, gameplay still works but text is invisible. Copy any `.ttf` to `assets/font.ttf`.

### Game opens then immediately closes
Run from **MinGW terminal** (not double-clicking the `.exe`):
```bash
./snake_xenzia.exe
```
Or build debug to see console output:
```bash
make debug
./snake_xenzia.exe
```

### Linker error: `undefined reference to SDL_main`
Make sure you are using `gcc`, not `g++`, and that `-mwindows` is in `LDFLAGS`.

### `pkg-config: command not found`
```bash
pacman -S mingw-w64-x86_64-pkg-config
```

### No audio / crash on audio init
The game gracefully handles missing audio — if `Mix_OpenAudio` fails it logs a warning and continues. Check that `assets/sounds/` contains the 4 `.wav` files (run `./gen_sounds` first).

---

## 🖥️ Example Session

```
┌─────────────────────────────────────────────────────┬────────────────────┐
│  . . . . . . . . . . . . . . . . . . . . . . . .   │  SNAKE XENZIA      │
│  . . . . . . . . . . . . . . . . . . . . . . . .   │                    │
│  . . . . . . ●─●─●─O . . . . . . . . . . . . .    │  MODE   Classic    │
│  . . . . . . . . . . . . . . . . . . . . . . . .   │  SCORE  140        │
│  . . . . . . . . . ✦ . . . . . . . . . . . . .    │  BEST   280        │
│  . . . . . . . . . . . @ . . . . . . . . . . .     │  COMBO  x3!        │
│  . . . . . . . . . . . . . . . . . . . . . . . .   │  LEN    18         │
│  . . . . # . . . . . . . . . . . . . . . . . . .   │  SPEED  62%        │
└─────────────────────────────────────────────────────┴────────────────────┘
   O = player head   ● = body   @ = food   ✦ = power-up   # = obstacle
```

---

## 📜 License

MIT — free to use, modify, share. Attribution appreciated.

# 🐍 Snake Xenzia — Enhanced Edition

Snake Xenzia is a modern Snake game built in **C** with smooth gameplay, multiple game modes, AI-powered pathfinding, power-ups, and interactive visuals. The game combines classic arcade mechanics with advanced features like BFS-based AI movement, combo scoring, obstacles, portals, and leaderboard tracking.

🌐 **Play on Website:**
https://hruthyacv.github.io/snake-xenzia/

---

## 🎮 Game Modes

* **Classic** — Traditional snake gameplay with portal teleportation
* **Time Attack** — Score maximum points before time runs out
* **Survival** — Increasing obstacles make the game progressively harder
* **AI Battle** — Watch an AI snake use BFS pathfinding to collect food

---

## ⚡ Features

* BFS Pathfinding AI
* Power-Up System
* Combo-Based Scoring
* Dynamic Obstacles
* Portal Teleportation
* Local Leaderboard System
* SDL2 Graphics & Sound Effects
* Smooth Real-Time Gameplay
* Mobile Touch Controls

---

## 🧩 Project Structure

```text
snake-xenzia/
├── main.c        — Game loop and menu system
├── game.c/h      — Core gameplay logic
├── snake.c/h     — Snake movement and controls
├── ai.c/h        — BFS AI pathfinding
├── ui.c/h        — Graphics and rendering
├── fileio.c/h    — Leaderboard storage
├── defs.h        — Shared constants and structures
├── index.html    — Browser version
└── Makefile      — Build configuration
```

---

## 🛠️ Technologies Used

* C Programming
* SDL2 Library
* File Handling
* BFS Algorithm
* Modular Programming
* HTML5 Canvas for Web Version

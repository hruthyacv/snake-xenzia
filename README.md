# 🐍 Snake Xenzia — Enhanced Edition

> A feature-rich Snake game built in **C** as a college project for the course **Structured Programming in C**.
> **BMS College of Engineering** | Team: **Hruthya CV & Harshitha RG**

---

## 🎮 About the Game

Snake Xenzia is a modern take on the classic Snake game — built entirely in C with smooth gameplay, intelligent AI, and multiple game modes. The project demonstrates core C programming concepts including modular design, file I/O, data structures, and algorithm implementation.

---

## 🕹️ Game Modes

| Mode | Description |
|------|-------------|
| **Classic** | Standard snake gameplay with portal teleportation |
| **Time Attack** | Score as much as possible in 90 seconds |
| **Survival** | Obstacles increase over time — how long can you last? |
| **AI Battle** | Watch a BFS pathfinding AI snake hunt food in real time |

---

## ⚡ Features

- **4 Game Modes** — Classic, Time Attack, Survival, AI Battle
- **BFS Pathfinding AI** — AI opponent uses Breadth-First Search algorithm
- **Power-Up System** — Speed Boost, Slow Motion, Double Score, Shrink
- **Combo Scoring** — Consecutive food collection multiplies your points
- **Particle Effects** — Visual burst effects on food and power-up collection
- **Leaderboard** — Top 10 scores saved locally with player name and mode
- **Portal System** — Teleport across the board in Classic and Survival modes
- **Obstacle System** — Dynamic obstacles in Survival mode
- **60 FPS Smooth Rendering** — Built with SDL2 for flicker-free graphics
- **Background Music & Sound Effects** — Synthesised WAV audio via SDL2 Mixer
- **Touch Controls** — Swipe gestures for mobile browser play

---

## 🧩 Project Structure

```
snake-xenzia/
├── main.c        — Entry point, event loop, menu navigation
├── game.c/h      — Core game logic, collision, scoring, power-ups
├── snake.c/h     — Snake entity, movement, body management
├── ai.c/h        — BFS pathfinding AI with greedy fallback
├── ui.c/h        — SDL2 rendering, animations, all screens
├── fileio.c/h    — Leaderboard file persistence
├── defs.h        — Shared constants, structs, colour palette
├── gen_sounds.c  — Procedural WAV audio generator
├── index.html    — Browser-playable version (HTML5 + Canvas)
└── Makefile      — Build system
```

---

## 🌐 Play Online

👉 **[Play Snake Xenzia in your browser](https://hruthyacv.github.io/snake-xenzia/)**

Works on mobile, tablet, and desktop. No installation needed.

---

## 📚 Academic Context

This project was developed as part of the **Structured Programming in C** course at **BMS College of Engineering**. It applies the following concepts from the curriculum:

- Modular programming with multiple `.c` / `.h` files
- Structs, enums, and pointers
- File I/O for leaderboard persistence
- Dynamic arrays and queue-based BFS algorithm
- Real-time input handling and game loop design

---

*Hruthya CV & Harshitha RG — BMS College of Engineering*
# Santa's Factory

*A Christmas-themed factory management and conveyor puzzle game built with C++ and SFML 3.*

## About the Game

Santa's Factory is a five-level strategy game where players manage Santa's workshop to prepare and deliver presents before time runs out.

Players construct conveyor routes, process toys through boxing and wrapping stations, sort presents according to Santa's orders, and earn money to improve factory efficiency.

As the levels progress, players must navigate increasingly complex layouts, respond to timed rush orders, and deal with mischievous gremlins that sabotage machinery.

## Features

- Five levels with unique conveyor puzzles
- Conveyor placement, rotation, and removal
- Toy launcher, boxing, wrapping, and sorting stations
- Five hireable elves that automate different factory tasks
- Factory upgrades for conveyor speed, sorting, and repairs
- Gremlins that sabotage equipment
- Manual and automated machine repairs
- Red and blue present orders with automatic storage
- Timed rush orders with bonus rewards
- Animated characters, machines, and conveyor belts
- Original jazzy Christmas background music and sound effects
- Fullscreen mode with F11

## Controls

| Key | Action |
|---|---|
| Enter | Start the game |
| G | Start factory production |
| E | Release the next toy |
| Space | Box or wrap a toy |
| A | Approve a present |
| S | Store a present |
| Left Click | Place or rotate conveyors |
| Right Click | Remove conveyors |
| P | Purchase three additional conveyor pieces |
| B | Upgrade conveyor speed |
| Z | Upgrade Repair Elf speed |
| T | Upgrade Sorting Elf speed |
| H | Open employee shop |
| 1–5 | Hire an employee while the shop is open |
| X | Repair a broken machine manually |
| M | Toggle background music |
| F11 | Toggle fullscreen |
| Esc | Pause or resume |
| R | Restart the level |
| N | Proceed to the next completed level |

The pause menu also displays the controls.

## How to Play

1. Inspect the factory layout and use the available conveyor pieces to complete the route.
2. Press **G** to begin production.
3. Process toys through the boxing and wrapping stations.
4. Sort presents according to Santa's current red or blue order.
5. Earn money by delivering presents and completing rush orders.
6. Hire elves and purchase upgrades to automate production.
7. Repair machines sabotaged by gremlins.
8. Complete all five levels to save Christmas!

## Technologies

- C++
- SFML 3
- Visual Studio Code
- MSYS2 UCRT64 / g++

## Building and Running

This project was developed on Windows using Visual Studio Code, MSYS2 UCRT64, and SFML 3.

### Requirements

- A C++ compiler supporting the project's language features
- MSYS2 UCRT64 with g++
- SFML 3 graphics, window, system, and audio libraries
- Visual Studio Code (optional)

### Build Instructions

1. Clone or download this repository.
2. Install MSYS2 UCRT64 and the compatible SFML 3 libraries.
3. Open the project folder in Visual Studio Code.
4. Check `.vscode/tasks.json` and adjust the compiler path if MSYS2 is installed somewhere other than `C:\msys64`.
5. Run the configured build task, **Build Santa's Factory**.
6. Launch `SantasFactory.exe` from the project directory so the game can find the audio files in `assets`.

The project uses these SFML libraries:

- `sfml-graphics`
- `sfml-window`
- `sfml-system`
- `sfml-audio`

## Project Structure

```text
SantasFactory/
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── assets/
│   ├── delivery.wav
│   ├── gremlin_sabotage.wav
│   ├── reject.wav
│   └── santas_factory_jazz_tight_groove.wav
├── main.cpp
└── README.md
```

## Project Information

Created as a C++ programming course project.

The project explores object movement, collision handling, event-driven input, game state management, resource management, automation, and graphical user interfaces using SFML.

**Goal:** Complete Santa's five delivery challenges and save Christmas! 

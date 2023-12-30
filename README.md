# CMancala

## Overview
CMancala is a computer implementation of the classic board game Mancala. This version supports both standard Mancala rules and an **Avalanche** mode, providing an engaging challenge with a sophisticated AI opponent.

## Rules
The game adheres to basic Mancala rules. In addition, an **Avalanche** mode is available, offering a different style of gameplay.

## Performance
The AI's capability varies based on the game state:
- In a 5-second search window, the AI can anticipate 20+ moves ahead, although this number greatly depends on the number of stones on the board.
- For games with 4 stones per cell, "perfect play" is achievable with approximately 60 seconds of search time.
- The AI performs more efficiently in games with fewer stones per cell.
- As the game progresses, the AI increasingly solves the position.

## Algorithm
CMancala employs a Negamax algorithm with Alpha-Beta pruning, tailored to accommodate double moves. Key features include:
- **Double Move Handling**: The algorithm adjusts search parameters based on whether the player's turn continues or switches to the opponent.
- **Aspiration Windows with Iterative Deepening**: This technique allows for time-limited searches while enhancing performance.

## Restrictions
- The game is designed to support a maximum of 224 stones on the board at any given time.

## Interface
- CMancala features a terminal-based user interface. Users interested in a graphical interface may consider developing a simple web-based UI.

## Building and Running
To build and run CMancala:

1. Ensure you have CMake installed.
2. Ensure you have a compatible C compiler.
3. Run the following commands in the project directory:
    ```bash
    cd build
    cmake ..
    make
    ```
4. Execute the program:
    ```bash
    ./Mancala
    ```
### License

CMancala is released under the MIT License. See LICENSE file for more details.

© 2023 Alexander Kurtz. All rights reserved.
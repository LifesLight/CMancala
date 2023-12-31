# CMancala

## Overview
CMancala is a computer implementation of the classic board game Mancala. This version supports both standard Mancala rules and an **Avalanche** mode, providing an engaging challenge with a sophisticated AI opponent.

## Rules
The game adheres to basic Mancala rules. In addition, an **Avalanche** mode is available, offering a different style of gameplay.

## Command-Line Arguments
CMancala supports several command-line arguments to configure gameplay:
- `--stones <number>`: Sets the number of stones per cell with a standard distribution.
- `--rstones <number>`: Sets the number of stones in total with a random distribution.
- `--time <seconds>`: Sets an approximate thinking time for the AI.
- `--depth <number>`: Sets the depth for the Negamax algorithm; if specified, overrides the time-based search.
- `--ai-start`: The AI will make the first move.
- `--human-start`: The human player will make the first move (default).

## Understanding Terminal Outputs

The terminal output provides a visual representation of the game state along with key information about the current turn and the game's progress:<br>
- **Board Layout**: The game board is displayed in a grid format.<br> Each cell contains a number representing the stones in that particular cell.<br> The layout is mirrored for each player.<br>
- **Turn Indicator**: `Turn: P1` or `Turn: P2` indicates which player's turn it is.<br> P1 stands for Player 1 (Human) and P2 for Player 2 (AI).<br>
- **Evaluation Score**: Displayed as `Evaluation: <number>`.<br> This score estimates the current advantage in terms of stones.<br> A positive score indicates an advantage for Player 1 (Human), and a negative score indicates an advantage for Player 2 (AI).<br> The magnitude of the score reflects the extent of the lead or deficit.<br> For example, `Evaluation: -8` suggests Player 1 is currently predicted to lose by 8 stones if both players continue with optimal play.<br>
- **Depth Reached**: When the AI makes a move, you might see `Depth reached: <number>`.<br> This shows how many moves ahead the AI calculated during its turn.<br>
- **AI Move**: After the AI completes its turn, `AI move: <cell number>` shows the cell index chosen by the AI for its move.<br>
- **Enter Move**: During the human player's turn, `Enter move:` prompts you to input your move.<br>


## Performance
The AI operates on a single thread, and its performance varies based on the game's state:<br>
- It typically anticipates 20-30 moves within a 5-second thinking window, influenced by the number of stones on the board.<br>
- With 4 stones per cell, the AI reaches a very high level of play in about 60 seconds.<br>
- Games with fewer stones per cell enhance the AI's performance.<br>
- As the game progresses, the AI's ability to predict moves improves.<br>

In most configurations, when the AI starts the game it becomes almost unbeatable due to the nature of Mancala.<br>
(in the classic 4 stone per cell configuration the starting player has a evaluation advantage of 9 at depth 38)<br>
These benchmarks are approximate and might slightly differ across various CPUs.<br>
Observations were made on an M2 Pro, but similar performance is expected on modern processors due to the exponential nature of the problem.<br>

## Algorithm
CMancala employs a Negamax algorithm with Alpha-Beta pruning, tailored to accommodate double moves. Key features include:
- **Double Move Handling**: The algorithm adjusts search parameters based on whether the player's turn continues or switches to the opponent.
- **Aspiration Windows with Iterative Deepening**: This technique allows for time-limited searches while enhancing performance.

## Restrictions
- The game is designed to support a maximum of 224 stones on the board at any given time.
- The game mode needs to be specified before compilation in `board.h`.

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
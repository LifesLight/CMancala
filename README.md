## CMancala

## Overview
CMancala is a powerful Mancala solver and analyzer.<br>
While it's not the project’s primary focus, it is very much possible to play against (effectively unbeatable) AI agents.

## Rules
The game adheres to basic Mancala rules. In addition, an **Avalanche** mode is available.

## Interface
CMancala starts in **config** mode, where you can set up the game hyperparameters.<br>
When starting the game, you will enter one of two modes, between which you can switch at all times:
- **Config:** Adjust application / game hyperparameters.
- **Menu:** Perform analytical operations on the current game.
- **Playing:** Play the game.

More information is provided via “help," which provides all legal commands in every mode.

## Performance
The AI operates on a single thread.<br> Its performance varies based on the game's state:<br>
- It typically anticipates 20-30 moves within a 5-second thinking window, influenced by the number of stones on the board.<br>
- With 4 stones per cell, the AI reaches a very high level of play in about 60 seconds.<br>
- Games with fewer stones per cell enhance the AI's performance.<br>
- As the game progresses, the AI's ability to predict moves improves.<br>

In most configurations, when the AI starts the game, it becomes almost unbeatable due to the nature of Mancala.<br>
(In the classic 4 stone per cell configuration, the starting player has an evaluation advantage of 8 at depth 40.)<br>
These benchmarks are approximate and might slightly differ across various CPUs.<br>
Observations were made on an M2 Pro, but similar performance is expected on modern processors due to the exponential nature of the problem.<br>

## Algorithm
All CMancala solvers employ a Negamax algorithm with Alpha-Beta pruning, tailored to accommodate double moves. Common features include:
- **Double Move Handling**: The algorithm adjusts search parameters based on whether the player's turn continues or switches to the opponent.
- **Aspiration Windows with Iterative Deepening**: This technique allows for time-limited searches while enhancing performance.
- **Clip [BETA]**: Clip changes the behavior of solvers to only compute if any move is winning or losing. This feature can be used for playing solvers ONLY if the AI is playing from a winning position; in losing positions, it will treat every move as equally bad if it can't find a winning move, which will result in the agent most likely not returning to a winning position since it will always make the first IDX move.

### Solvers
- **GLOBAL:**<br>Recommended for quickly analyzing a position or playing against a AI.<br>The reference solver. Is only satisfied once the complete game tree is exhaustively searched for the best possible move at the current node.
- **LOCAL:**<br>Recommended for longer sessions, approximately half the speed of **GLOBAL** before the cache is warmed up.<br>Similar to GLOBAL but with knowledge of every node’s subtree solve status. This allows for optimizations like solved node caching.<br>
The tradeoff between resistance to hash collisions and memory usage can be specified before compilation; by default, it's set to guarantee no hash collisions since the memory usage is usually very low.<br>
While I am very confident in the solver’s final output, I can't ensure its correctness; for any critical applications, **GLOBAL** is still recommended.

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

## Discoveries
- On search depth 50 in the standard 4 stones per pit losing position (Encoded: 008080a0a0a0a04000a0a000808)<br> the best (known) move is **5** with a **-8** evaluation.
- On a uniform 3 stone per pit game, the starting player wins by 2 points with perfect play.<br>Best start move is IDX: 5. (Depth reached was 108, which solved the position).

### License
CMancala is released under the MIT License. See LICENSE file for more details.
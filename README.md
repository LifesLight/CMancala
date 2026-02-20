## CMancala

### Overview

CMancala is a powerful Mancala solver and analyzer.
You can play against (effectively unbeatable) AI agents.

### Rules

The game follows basic Mancala rules.
An **Avalanche** mode is available.

### Interface (config/menu/playing)

CMancala starts in **config** mode to set game hyperparameters.
Three modes — switchable anytime:

* **Config:** Adjust application and game hyperparameters.
* **Menu:** Perform analytical operations on the current game.
* **Playing:** Play the game.

See `help` for all legal commands.

### Performance

The AI runs on a single thread.
Performance varies with game state and number of stones.

* Typical search: 20–30 moves in a 1s thinking window.
* 4 stones/pit → very high level in ~5s.
* Fewer stones → faster searches.
* Predictions improve as the game progresses.

If the AI starts, it is usually almost unbeatable.
(In 4-stone classic, starter has an evaluation advantage of 8.)

Cache size notes (**LOCAL**):

* Cache size is an exponent: size = `2^N`.
* Memory usage: `2^N * ~4–8 bytes per entry` (depends on tag size and depth mode).
* Default: **24** → ~100 MiB RAM.
* Use **29–32** for long solves (e.g., uniform 4-stone).
* Cache size requirements have steadily decreased since v3.0. Run your own tests if concerned.

* **Compressed mode:**
  * The code auto-selects compressed vs non-compressed. You usually don't need to think about it.
  * Required: `indexBits + tagBits >= keyBits`.
    * For compressed keys that means `indexBits + tagBits >= 48`.
    * For non-compressed keys that means `indexBits + tagBits >= 60`.
  * Why that matters:
    * Compressed mode uses **48-bit keys (B48)** instead of **60-bit keys (B60)**.
    * Compression allows using 16-bit tags (when possible) instead of 32-bit tags, which can reduce RAM usage at given cache size.
    * Non-compressed (B60) allows storing boards with larger per-cell values (up to <31 vs <16).
  * Only force-compress if the math requires it or you explicitly want smaller tags to save RAM.

<p align="center">
  <img src="measurements/v3.0/PickCacheSize.jpg" width="600">
</p>

*Measured on CMancala **v3.0***. More measurements in `measurements/`.

### Algorithm

All solvers use Negamax + Alpha-Beta. They handle double moves.

* **Double move handling:** Search adjusts when the same player continues or turn switches.
* **Aspiration windows + iterative deepening:** Enables time-limited searches and better performance. (Skipped for LOCAL with infinite depth and time)
* **Move ordering:** Prioritizes promising moves early. Improves pruning.
* **Clip:** Changes solver behavior to only search for wins/losses. Useful if you only want to see if a move is winning or losing, not "how winning" or "how losing". In losing positions it will always pick the first IDX move if no win is found so it most likely won't return to a winning position.

#### Solvers

* **GLOBAL:** Reference solver.
* **LOCAL (default):** Way Faster in most cases. Uses a transposition table. At equal search depth it should be as strong or stronger than GLOBAL.

### Limitations

* Maximum supported stones on the board at any time: 224.

### User interface

* Terminal UI.
* Experimental WebUI.

### Building and Running

1. Install CMake.
2. Have a compatible C compiler.
3. In project dir:

```bash
cd build
cmake ..
make
````

4. Run:

```bash
./Mancala
```

PGO supported. Example build script included.

### Discoveries

* Uniform 5-stone-per-pit solved with **LOCAL**. Starter wins by **10** points. Best first move: **IDX: 2**.
* Uniform 4-stone-per-pit solved with **LOCAL**. Starter wins by **8** points. Best first move: **IDX: 3**.
* Uniform 3-stone-per-pit: starter wins by **2** points. Best first move: **IDX: 5**.

### License

CMancala is released under the MIT License. See `LICENSE` for details.
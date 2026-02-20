#include "user/interface.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_HISTORY 512

static Context globalContext;
static Board globalBoard;
static Board globalLastBoard;

/* History Tracking */
static Board boardHistory[MAX_HISTORY];
static int   boardHistoryMoves[MAX_HISTORY];
static int   boardHistoryCaptures[MAX_HISTORY];
static int   historyCount = 0;

static int   currentCaptureMask = 0;

static bool isGameInitialized = false;
static bool aiThinking = false;
static double totalNodesExplored = 0.0;

/* ── Capture Detection Logic ───────────────────────────────────────── */

static int calc_capture_mask(const Board* prev, const Board* curr) {
    int mask = 0;
    bool sweep_p1 = false;
    bool sweep_p2 = false;

    // Check if a terminal sweep just happened.
    // If a player's side is empty at the end, the OTHER player's stones are swept.
    if (isBoardTerminal(curr)) {
        if (isBoardPlayerOneEmpty(curr)) sweep_p2 = true; // P1 is empty, P2's pits were swept
        if (isBoardPlayerTwoEmpty(curr)) sweep_p1 = true; // P2 is empty, P1's pits were swept
    }

    // A capture occurs when an opponent's pit decreases in value.
    if (prev->color == 1) { // It was Player 1's turn (Opponent is P2, pits 7-12)
        for (int i = 7; i <= 12; i++) {
            if (curr->cells[i] < prev->cells[i]) {
                // If P2's pits were swept to end the game, ignore it.
                if (!sweep_p2) {
                    mask |= (1 << i);
                }
            }
        }
    } else { // It was Player 2's turn (Opponent is P1, pits 0-5)
        for (int i = 0; i <= 5; i++) {
            if (curr->cells[i] < prev->cells[i]) {
                // If P1's pits were swept to end the game, ignore it.
                if (!sweep_p1) {
                    mask |= (1 << i);
                }
            }
        }
    }

    return mask;
}


/* ── Board state accessors ─────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE int  get_stone_count(int i) { return isGameInitialized ? globalContext.board->cells[i] : 0; }
EMSCRIPTEN_KEEPALIVE int  get_current_player()   { return isGameInitialized ? globalContext.board->color : 0; }
EMSCRIPTEN_KEEPALIVE int  get_last_move()        { return isGameInitialized ? globalContext.metadata.lastMove : -1; }
EMSCRIPTEN_KEEPALIVE int  get_ai_thinking()      { return aiThinking; }
EMSCRIPTEN_KEEPALIVE int  get_game_over()        { return isGameInitialized && isBoardTerminal(globalContext.board); }
EMSCRIPTEN_KEEPALIVE int  get_current_captures() { return currentCaptureMask; }

/* ── History accessors ─────────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE int get_history_count() { return historyCount; }

EMSCRIPTEN_KEEPALIVE int get_history_stone_count(int turn, int cell) {
    if (turn >= 0 && turn < historyCount) return boardHistory[turn].cells[cell];
    return 0;
}

EMSCRIPTEN_KEEPALIVE int get_history_move(int turn) {
    if (turn >= 0 && turn < historyCount) return boardHistoryMoves[turn];
    return -1;
}

EMSCRIPTEN_KEEPALIVE int get_history_captures(int turn) {
    if (turn >= 0 && turn < historyCount) return boardHistoryCaptures[turn];
    return 0;
}

static void push_history(Board* b, int move, int capMask) {
    if (historyCount < MAX_HISTORY) {
        boardHistory[historyCount] = *b;
        boardHistoryMoves[historyCount] = move;
        boardHistoryCaptures[historyCount] = capMask;
        historyCount++;
    }
}

/* ── Solver stat accessors ─────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE int    get_stat_solved() { return globalContext.metadata.lastSolved; }
EMSCRIPTEN_KEEPALIVE int    get_stat_depth()  { return globalContext.metadata.lastDepth; }
EMSCRIPTEN_KEEPALIVE double get_stat_nodes()  { return (double)globalContext.metadata.lastNodes; }
EMSCRIPTEN_KEEPALIVE double get_stat_time()   { return globalContext.metadata.lastTime; }
EMSCRIPTEN_KEEPALIVE double get_stat_total_nodes() { return totalNodesExplored; }

EMSCRIPTEN_KEEPALIVE int get_stat_eval() {
    if (globalContext.metadata.lastEvaluation == INT32_MAX) return -9999;
    return globalContext.metadata.lastEvaluation;
}

/* ── Game initialisation ───────────────────────────────────────────── */

static void init_game_with_config(int stones, int distribution, int moveFunc,
                                  double timeLimit, int startColor) {
    SolverConfig solverConfig = {
        .solver          = LOCAL_SOLVER,
        .depth           = 0,
        .timeLimit       = timeLimit,
        .clip            = false,
        .compressCache   = AUTO,
        .progressBar     = false
    };

    GameSettings gameSettings = {
        .stones        = stones,
        .distribution  = distribution ? RANDOM_DIST : UNIFORM_DIST,
        .seed          = (int)time(NULL),
        .startColor    = startColor,
        .player1       = HUMAN_AGENT,
        .player2       = AI_AGENT,
        .moveFunction  = moveFunc ? AVALANCHE_MOVE : CLASSIC_MOVE
    };

    Config config = {
        .autoplay      = false,
        .gameSettings  = gameSettings,
        .solverConfig  = solverConfig
    };

    initializeBoardFromConfig(&globalBoard, &config);
    setMoveFunction(config.gameSettings.moveFunction);

    globalContext.board     = &globalBoard;
    globalContext.lastBoard = &globalLastBoard;
    globalContext.config    = config;
    globalContext.metadata  = (Metadata){
        .lastMove       = -1,
        .lastEvaluation = INT32_MAX,
        .lastDepth      = 0,
        .lastSolved     = false,
        .lastTime       = 0,
        .lastNodes      = 0
    };

    isGameInitialized = true;
    aiThinking = false;
    totalNodesExplored = 0.0;
    currentCaptureMask = 0;
    
    historyCount = 0;
    push_history(globalContext.board, -1, 0); // Push initial state
}

/* ── AI turn logic ─────────────────────────────────────────────────── */

#ifdef __EMSCRIPTEN__
static void ai_think_callback(void *arg) {
    (void)arg;

    aspirationRoot(&globalContext, &globalContext.config.solverConfig);

    totalNodesExplored += globalContext.metadata.lastNodes;

    int move = globalContext.metadata.lastMove;
    if (move != -1) {
        copyBoard(globalContext.board, globalContext.lastBoard);
        makeMoveManual(globalContext.board, move);
        
        currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board);
        push_history(globalContext.board, move, currentCaptureMask);
    }

    EM_ASM({ updateView(); });

    if (!isBoardTerminal(globalContext.board) && globalContext.board->color == -1) {
        emscripten_async_call(ai_think_callback, NULL, 500);
    } else {
        aiThinking = false;
        EM_ASM({ updateView(); });
    }
}

static void kick_off_ai(void) {
    aiThinking = true;
    EM_ASM({ updateView(); });
    emscripten_async_call(ai_think_callback, NULL, 500);
}
#endif

/* ── Human move entry point ────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
void do_web_move(int index) {
    if (!isGameInitialized || aiThinking) return;
    if (globalContext.board->color != 1) return;
    if (globalContext.board->cells[index] == 0) return;

    copyBoard(globalContext.board, globalContext.lastBoard);
    makeMoveManual(globalContext.board, index);
    globalContext.metadata.lastMove = index;
    
    currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board);
    push_history(globalContext.board, index, currentCaptureMask);

    EM_ASM({ updateView(); });

    if (!isBoardTerminal(globalContext.board) && globalContext.board->color == -1) {
#ifdef __EMSCRIPTEN__
        kick_off_ai();
#endif
    }
}

/* ── Settings restart ──────────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
void restart_game(int stones, int distribution, int moveFunc,
                  double timeLimit, int startColor) {
    init_game_with_config(stones, distribution, moveFunc, timeLimit, startColor);
#ifdef __EMSCRIPTEN__
    EM_ASM({ updateView(); });
    if (globalContext.board->color == -1) {
        kick_off_ai();
    }
#endif
}

/* ── GUI (Embedded JavaScript Application) ─────────────────────────── */

#ifdef __EMSCRIPTEN__
EM_JS(void, launch_gui, (), {
    document.body.innerHTML = "";

    const css = `
        * { box-sizing: border-box; }
        body { background: #222; color: #fff; font-family: monospace; margin: 0; overflow: hidden; }

        .sidebar { position: fixed; top: 0; width: 280px; height: 100%;
          background: #2d2d2d; padding: 20px; padding-top: 50px; border: none;
          transition: transform .3s; z-index: 10; overflow-y: auto; }
        .sidebar-left  { left: 0; border-right: 2px solid #505050; transform: translateX(-100%); }
        .sidebar-left.open  { transform: translateX(0); }
        .sidebar-right { right: 0; border-left: 2px solid #505050; transform: translateX(100%); }
        .sidebar-right.open { transform: translateX(0); }

        .side-toggle { position: fixed; top: 10px; padding: 10px 15px;
          background: #3c3c3c; cursor: pointer; z-index: 11;
          border: 1px solid #646464; color: #fff; font-family: monospace; border-radius: 20px; }
        .toggle-left  { left: 10px; }
        .toggle-right { right: 10px; }

        .stat-box   { margin-bottom: 20px; border-bottom: 1px solid #464646; padding-bottom: 10px; }
        .stat-label { color: #969696; font-size: 12px; display: block; }
        .stat-val   { font-size: 18px; color: #0f0; }

        .cfg-row   { margin-bottom: 14px; }
        .cfg-label { color: #969696; font-size: 12px; display: block; margin-bottom: 4px; }
        .cfg-row select, .cfg-row input { width: 100%; padding: 6px; background: #444; color: #fff; border: 1px solid #646464; font-family: monospace; font-size: 14px; border-radius: 4px; }
        
        .cfg-btn { width: 100%; padding: 10px; margin-top: 10px; background: #0a0; color: #000; border: none; cursor: pointer; font-family: monospace; font-size: 16px; font-weight: bold; border-radius: 10px; }
        .cfg-btn:hover { background: #0c0; }
        
        .hist-toggle-text { background: transparent; border: none; color: #888; 
            font-size: 14px; cursor: pointer; margin-top: 15px; font-family: monospace; }
        .hist-toggle-text:hover { color: #fff; text-decoration: underline; }

        .main-content { display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; }
        
        .board-wrapper { display: flex; background: #333; border-radius: 10px; }
        .board-wrapper.main { padding: 20px; gap: 20px; margin-bottom: 10px; }
        .board-wrapper.mini { padding: 10px; gap: 10px; border-radius: 6px; background: #2a2a2a; }

        .pit { border-radius: 50%; display: flex; align-items: center; justify-content: center; background: #444; transition: .2s; }
        .store { border-radius: 20px; display: flex; align-items: center; justify-content: center; background: #555; }
        .row { display: flex; }

        .main .pit { width: 60px; height: 60px; font-size: 24px; border: 2px solid #555; }
        .main .store { width: 80px; font-size: 32px; border: 2px solid #666; }
        .main .row { gap: 10px; margin: 10px 0; }
        
        .mini .pit { width: 38px; height: 38px; font-size: 14px; border: 1px solid #444; }
        .mini .store { width: 50px; font-size: 18px; border: 1px solid #555; border-radius: 10px; }
        .mini .row { gap: 5px; margin: 5px 0; }

        .player-pit { cursor: pointer; border-color: #0f0 !important; }
        .ai-pit     { border-color: #f00 !important; }
        
        .highlight-move { background: #999 !important; color: #000; }
        /* A softer, muted red that is clearly visible but easier on the eyes */
        .highlight-capture { background-color: #8b3a3a !important; color: #fff; }

        .disabled { opacity: .5; pointer-events: none; }

        .history-container { display: none; flex-direction: column; gap: 20px; padding: 15px; background: #1a1a1a; border-radius: 10px; max-height: 35vh; overflow-y: auto; margin-top: 10px; border: 1px solid #444; }
        .hist-item { display: flex; justify-content: center; border-bottom: 1px solid #333; padding-bottom: 15px; }
        .hist-item:last-child { border-bottom: none; }

        .footer { position: fixed; bottom: 10px; width: 100%; text-align: center; pointer-events: none; }
        .footer a { color: #888; text-decoration: none; font-size: 14px; pointer-events: auto; transition: color 0.2s; }
        .footer a:hover { color: #fff; }
    `;
    const style = document.createElement("style");
    style.textContent = css;
    document.head.appendChild(style);

    window.createBoardDOM = function(container, prefix, isMini) {
        container.innerHTML = "";
        const wrap = document.createElement("div");
        wrap.className = "board-wrapper " + (isMini ? "mini" : "main");
        wrap.id = prefix + "board";

        const p2Store = document.createElement("div"); p2Store.className = "store"; p2Store.id = prefix + "13";
        wrap.appendChild(p2Store);

        const rows = document.createElement("div");
        const topRow = document.createElement("div"); topRow.className = "row";
        for (let i = 12; i >= 7; i--) {
            const p = document.createElement("div"); p.className = "pit ai-pit"; p.id = prefix + i;
            topRow.appendChild(p);
        }
        
        const botRow = document.createElement("div"); botRow.className = "row";
        for (let i = 0; i <= 5; i++) {
            const p = document.createElement("div"); p.className = "pit player-pit"; p.id = prefix + i;
            if (!isMini) {
                p.onclick = () => {
                    if (Module._get_ai_thinking() || Module._get_current_player() !== 1) return;
                    Module._do_web_move(i);
                };
            }
            botRow.appendChild(p);
        }
        
        rows.appendChild(topRow);
        rows.appendChild(botRow);
        wrap.appendChild(rows);

        const p1Store = document.createElement("div"); p1Store.className = "store"; p1Store.id = prefix + "6";
        wrap.appendChild(p1Store);

        container.appendChild(wrap);
    };

    const sideL = document.createElement("div");
    sideL.className = "sidebar sidebar-left";
    sideL.innerHTML = `
        <h3>SOLVER STATS</h3>
        <div class='stat-box'><span class='stat-label'>STATUS</span><span id='s-status' class='stat-val'>-</span></div>
        <div class='stat-box'><span class='stat-label'>EVALUATION</span><span id='s-eval' class='stat-val'>-</span></div>
        <div class='stat-box'><span class='stat-label'>LAST NODES</span><span id='s-nodes' class='stat-val'>-</span></div>
        <div class='stat-box'><span class='stat-label'>TOTAL NODES</span><span id='s-total-nodes' class='stat-val'>-</span></div>
        <div class='stat-box'><span class='stat-label'>THROUGHPUT</span><span id='s-tput' class='stat-val'>-</span></div>
    `;
    document.body.appendChild(sideL);

    const togL = document.createElement("button"); togL.className = "side-toggle toggle-left"; togL.innerText = "STATS";
    togL.onclick = () => sideL.classList.toggle("open");
    document.body.appendChild(togL);

    const sideR = document.createElement("div");
    sideR.className = "sidebar sidebar-right";
    sideR.innerHTML = `
        <h3>GAME SETTINGS</h3>
        <div class='cfg-row'><span class='cfg-label'>STONES (1-16)</span><input id='cfg-stones' type='number' min='1' max='16' value='4'></div>
        <div class='cfg-row'><span class='cfg-label'>DISTRIBUTION</span><select id='cfg-dist'><option value='0'>Uniform</option><option value='1'>Random</option></select></div>
        <div class='cfg-row'><span class='cfg-label'>MODE</span><select id='cfg-mode'><option value='0'>Classic</option><option value='1'>Avalanche</option></select></div>
        <div class='cfg-row'><span class='cfg-label'>AI TIME LIMIT (s)</span><input id='cfg-time' type='number' min='0' step='0.5' value='1'></div>
        <div class='cfg-row'><span class='cfg-label'>STARTING PLAYER</span><select id='cfg-start'><option value='1'>Player (You)</option><option value='-1'>AI</option></select></div>
        <button id='cfg-go' class='cfg-btn'>START GAME</button>
    `;
    document.body.appendChild(sideR);

    const togR = document.createElement("button"); togR.className = "side-toggle toggle-right"; togR.innerText = "SETTINGS";
    togR.onclick = () => sideR.classList.toggle("open");
    document.body.appendChild(togR);

    document.getElementById("cfg-go").onclick = () => {
        if (Module._get_ai_thinking()) return;
        let stones = parseInt(document.getElementById("cfg-stones").value) || 4;
        stones = Math.max(1, Math.min(16, stones));
        const dist  = parseInt(document.getElementById("cfg-dist").value);
        const mode  = parseInt(document.getElementById("cfg-mode").value);
        let tl      = parseFloat(document.getElementById("cfg-time").value);
        if (isNaN(tl) || tl < 0) tl = 1;
        const start = parseInt(document.getElementById("cfg-start").value);

        Module._restart_game(stones, dist, mode, tl, start);
        sideR.classList.remove("open");
    };

    const main = document.createElement("div"); main.className = "main-content";
    document.body.appendChild(main);

    const boardContainer = document.createElement("div");
    main.appendChild(boardContainer);
    window.createBoardDOM(boardContainer, "main-", false);

    const statusText = document.createElement("div");
    statusText.id = "status-text";
    statusText.style.marginTop = "20px";
    statusText.style.fontSize = "18px";
    main.appendChild(statusText);

    const histBtn = document.createElement("button");
    histBtn.className = "hist-toggle-text";
    histBtn.innerText = "▼ history ▼";
    histBtn.onclick = () => {
        const hc = document.getElementById("history-container");
        if (hc.style.display === "none" || !hc.style.display) {
            hc.style.display = "flex";
            histBtn.innerText = "▲ history ▲";
            window.syncHistory();
        } else {
            hc.style.display = "none";
            histBtn.innerText = "▼ history ▼";
        }
    };
    main.appendChild(histBtn);

    const histContainer = document.createElement("div");
    histContainer.id = "history-container";
    histContainer.className = "history-container";
    main.appendChild(histContainer);

    const footer = document.createElement("div"); footer.className = "footer";
    footer.innerHTML = "<a href='https://github.com/LifesLight/CMancala' target='_blank'>CMancala v6.0</a>";
    document.body.appendChild(footer);

    window.syncHistory = function() {
        const hc = document.getElementById("history-container");
        if (hc.style.display === "none") return;

        const historyCount = Module._get_history_count();
        hc.innerHTML = ""; 

        // Loop includes the initial state (0), hides current (count-1)
        for (let t = historyCount - 2; t >= 0; t--) {
            const item = document.createElement("div");
            item.className = "hist-item";

            const bContainer = document.createElement("div");
            window.createBoardDOM(bContainer, "h" + t + "-", true);
            item.appendChild(bContainer);
            hc.appendChild(item);
            
            // Use index 't' to correctly match the move and capture mask 
            // that generated the board state at index 't'.
            const playedMove = Module._get_history_move(t);
            const capturesMask = Module._get_history_captures(t);

            for (let i = 0; i < 14; i++) {
                const el = document.getElementById("h" + t + "-" + i);
                if (el) {
                    el.innerText = Module._get_history_stone_count(t, i);
                    el.classList.toggle("highlight-move", i === playedMove);
                    el.classList.toggle("highlight-capture", !!(capturesMask & (1 << i)));
                }
            }
        }
        
        if (historyCount <= 1) {
            hc.innerHTML = "<div style='text-align:center; color:#666;'>No past moves to show</div>";
        }
    };

    window.updateView = function() {
        const thinking = Module._get_ai_thinking();
        const gameOver = Module._get_game_over();
        const last     = Module._get_last_move();
        const caps     = Module._get_current_captures();
        
        const mainBoard = document.getElementById("main-board");
        mainBoard.classList.toggle("disabled", !!(thinking || gameOver));

        for (let i = 0; i < 14; i++) {
            const el = document.getElementById("main-" + i);
            if (!el) continue;
            el.innerText = Module._get_stone_count(i);
            
            el.classList.toggle("highlight-move", i === last);
            el.classList.toggle("highlight-capture", !!(caps & (1 << i)));
        }

        const solved  = Module._get_stat_solved();
        const depth   = Module._get_stat_depth();
        const evalVal = Module._get_stat_eval();
        const nodes   = Module._get_stat_nodes();
        const time    = Module._get_stat_time();

        document.getElementById("s-status").innerText = solved ? "SOLVED" : "DEPTH: " + depth;
        document.getElementById("s-eval").innerText   = (evalVal === -9999) ? "N/A" : (evalVal > 0 ? "+" + evalVal : "" + evalVal);
        document.getElementById("s-nodes").innerText  = (nodes / 1e6).toFixed(3) + " M";
        document.getElementById("s-total-nodes").innerText = (Module._get_stat_total_nodes() / 1e6).toFixed(3) + " M";

        const tput = time > 0 ? ((nodes / time) / 1e6).toFixed(2) : "0.00";
        document.getElementById("s-tput").innerText = tput + " M n/s";

        const st = document.getElementById("status-text");
        if (gameOver)      st.innerText = ">> GAME OVER";
        else if (thinking) st.innerText = ">> AI THINKING...";
        else               st.innerText = ">> YOUR TURN";

        window.syncHistory();
    };

    setTimeout(updateView, 100);
});
#endif

void startInterface() {
    init_game_with_config(4, 0, 0, 1.0, 1);
#ifdef __EMSCRIPTEN__
    launch_gui();
#endif
}

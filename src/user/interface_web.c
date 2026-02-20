#include "user/interface.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static Context globalContext;
static Board globalBoard;
static Board globalLastBoard;
static bool isGameInitialized = false;
static bool aiThinking = false;
static double totalNodesExplored = 0.0; /* NEW: Accumulator for total nodes */

/* ── Board state accessors ─────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE int  get_stone_count(int i) { return isGameInitialized ? globalContext.board->cells[i] : 0; }
EMSCRIPTEN_KEEPALIVE int  get_current_player()   { return isGameInitialized ? globalContext.board->color : 0; }
EMSCRIPTEN_KEEPALIVE int  get_last_move()         { return isGameInitialized ? globalContext.metadata.lastMove : -1; }
EMSCRIPTEN_KEEPALIVE int  get_ai_thinking()       { return aiThinking; }
EMSCRIPTEN_KEEPALIVE int  get_game_over()         { return isGameInitialized && isBoardTerminal(globalContext.board); }

/* ── Solver stat accessors ─────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE int    get_stat_solved() { return globalContext.metadata.lastSolved; }
EMSCRIPTEN_KEEPALIVE int    get_stat_depth()  { return globalContext.metadata.lastDepth; }
EMSCRIPTEN_KEEPALIVE double get_stat_nodes()  { return (double)globalContext.metadata.lastNodes; }
EMSCRIPTEN_KEEPALIVE double get_stat_time()   { return globalContext.metadata.lastTime; }
EMSCRIPTEN_KEEPALIVE double get_stat_total_nodes() { return totalNodesExplored; } /* NEW: Accessor for total */

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
    totalNodesExplored = 0.0; /* NEW: Reset total on new game */
}

/* ── AI turn logic ─────────────────────────────────────────────────── */

#ifdef __EMSCRIPTEN__
static void ai_think_callback(void *arg) {
    (void)arg;

    aspirationRoot(&globalContext, &globalContext.config.solverConfig);

    /* NEW: Accumulate nodes after AI finishes thinking */
    totalNodesExplored += globalContext.metadata.lastNodes;

    int move = globalContext.metadata.lastMove;
    if (move != -1) {
        copyBoard(globalContext.board, globalContext.lastBoard);
        makeMoveManual(globalContext.board, move);
    }

    EM_ASM({ updateView(); });

    /* Handle consecutive AI turns (captures / avalanche extra turns) */
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

    /* If AI starts, kick it off immediately */
    if (globalContext.board->color == -1) {
        kick_off_ai();
    }
#endif
}

/* ── GUI ───────────────────────────────────────────────────────────── */

#ifdef __EMSCRIPTEN__
EM_JS(void, launch_gui, (), {
    document.body.innerHTML = "";

    /* ── Base styles ─────────────────────────────────────────────── */
    var css = [
        "* { box-sizing: border-box; }",
        "body { background: #222; color: #fff; font-family: monospace;",
        "       margin: 0; overflow: hidden; }",

        /* Shared sidebar chrome - WIDER & with top padding for content */
        ".sidebar { position: fixed; top: 0; width: 280px; height: 100%;", /* CHANGED: 260px -> 280px */
        "  background: #2d2d2d; padding: 20px; padding-top: 50px; border: none;", /* CHANGED: Added padding-top */
        "  transition: transform .3s; z-index: 10; overflow-y: auto; }",

        /* Left sidebar – stats */
        ".sidebar-left  { left: 0; border-right: 2px solid #505050;",
        "  transform: translateX(-100%); }",
        ".sidebar-left.open  { transform: translateX(0); }",

        /* Right sidebar – settings */
        ".sidebar-right { right: 0; border-left: 2px solid #505050;",
        "  transform: translateX(100%); }",
        ".sidebar-right.open { transform: translateX(0); }",

        /* Toggle buttons */
        ".side-toggle { position: fixed; top: 10px; padding: 10px 15px;",
        "  background: #3c3c3c; cursor: pointer; z-index: 11;",
        "  border: 1px solid #646464; color: #fff; font-family: monospace;",
        "  border-radius: 20px; }",
        ".toggle-left  { left: 10px; }",
        ".toggle-right { right: 10px; }",

        /* Stat boxes */
        ".stat-box   { margin-bottom: 20px; border-bottom: 1px solid #464646;",
        "  padding-bottom: 10px; }",
        ".stat-label { color: #969696; font-size: 12px; display: block; }",
        ".stat-val   { font-size: 18px; color: #0f0; }",

        /* Settings controls */
        ".cfg-row   { margin-bottom: 14px; }",
        ".cfg-label { color: #969696; font-size: 12px; display: block;",
        "  margin-bottom: 4px; }",
        ".cfg-row select, .cfg-row input { width: 100%; padding: 6px;",
        "  background: #444; color: #fff; border: 1px solid #646464;",
        "  font-family: monospace; font-size: 14px; border-radius: 4px; }",
        ".cfg-btn { width: 100%; padding: 10px; margin-top: 10px;",
        "  background: #0a0; color: #000; border: none; cursor: pointer;",
        "  font-family: monospace; font-size: 16px; font-weight: bold;",
        "  border-radius: 10px; }",
        ".cfg-btn:hover { background: #0c0; }",

        /* Board */
        ".main-content { display: flex; flex-direction: column;",
        "  align-items: center; justify-content: center; height: 100vh; }",
        ".board-container { display: flex; background: #333;",
        "  padding: 20px; border-radius: 10px; gap: 20px; }",
        ".row { display: flex; gap: 10px; margin: 10px 0; }",
        ".pit { width: 60px; height: 60px; background: #444;",
        "  border-radius: 50%; display: flex; align-items: center;",
        "  justify-content: center; font-size: 24px;",
        "  border: 2px solid #555; transition: .2s; }",
        ".store { width: 80px; background: #555; border-radius: 20px;",
        "  display: flex; align-items: center; justify-content: center;",
        "  font-size: 32px; border: 2px solid #666; }",
        ".player-pit { cursor: pointer; border-color: #0f0; }",
        ".ai-pit     { border-color: #f00; }",
        ".highlight-move { background: #999 !important; color: #000; }",
        ".disabled { opacity: .5; pointer-events: none; }",

        /* Footer */
        ".footer { position: fixed; bottom: 10px; width: 100%; text-align: center; }",
        ".footer a { color: #888; text-decoration: none; font-size: 14px; transition: color 0.2s; }",
        ".footer a:hover { color: #fff; }"
    ].join("\n");

    var style = document.createElement("style");
    style.textContent = css;
    document.head.appendChild(style);

    /* ── Left sidebar – solver stats ─────────────────────────────── */
    var sideL = document.createElement("div");
    sideL.id = "sidebar-left";
    sideL.className = "sidebar sidebar-left";
    sideL.innerHTML =
        "<h3>SOLVER STATS</h3>" +
        "<div class='stat-box'><span class='stat-label'>STATUS</span>"   +
        "  <span id='s-status' class='stat-val'>-</span></div>" +
        "<div class='stat-box'><span class='stat-label'>EVALUATION</span>" +
        "  <span id='s-eval' class='stat-val'>-</span></div>" +
        "<div class='stat-box'><span class='stat-label'>LAST NODES EXPLORED</span>" + /* CHANGED: Label */
        "  <span id='s-nodes' class='stat-val'>-</span></div>" +
        "<div class='stat-box'><span class='stat-label'>TOTAL NODES EXPLORED</span>" + /* NEW: Total stat box */
        "  <span id='s-total-nodes' class='stat-val'>-</span></div>" +
        "<div class='stat-box'><span class='stat-label'>THROUGHPUT</span>" +
        "  <span id='s-tput' class='stat-val'>-</span></div>";
    document.body.appendChild(sideL);

    var togL = document.createElement("button");
    togL.className = "side-toggle toggle-left";
    togL.innerText = "STATS";
    togL.onclick = function() { sideL.classList.toggle("open"); };
    document.body.appendChild(togL);

    /* ── Right sidebar – game settings ───────────────────────────── */
    var sideR = document.createElement("div");
    sideR.id = "sidebar-right";
    sideR.className = "sidebar sidebar-right";
    sideR.innerHTML =
        "<h3>GAME SETTINGS</h3>" +

        "<div class='cfg-row'><span class='cfg-label'>STONES (1–16)</span>" +
        "  <input id='cfg-stones' type='number' min='1' max='16' value='4'></div>" +

        "<div class='cfg-row'><span class='cfg-label'>DISTRIBUTION</span>" +
        "  <select id='cfg-dist'>" +
        "    <option value='0'>Uniform</option>" +
        "    <option value='1'>Random</option>" +
        "  </select></div>" +

        "<div class='cfg-row'><span class='cfg-label'>MODE</span>" +
        "  <select id='cfg-mode'>" +
        "    <option value='0'>Classic</option>" +
        "    <option value='1'>Avalanche</option>" +
        "  </select></div>" +

        "<div class='cfg-row'><span class='cfg-label'>AI TIME LIMIT (s, 0 = unlimited)</span>" +
        "  <input id='cfg-time' type='number' min='0' step='0.5' value='1'></div>" +

        "<div class='cfg-row'><span class='cfg-label'>STARTING PLAYER</span>" +
        "  <select id='cfg-start'>" +
        "    <option value='1'>Player (You)</option>" +
        "    <option value='-1'>AI</option>" +
        "  </select></div>" +

        "<button id='cfg-go' class='cfg-btn'>START</button>";
    document.body.appendChild(sideR);

    var togR = document.createElement("button");
    togR.className = "side-toggle toggle-right";
    togR.innerText = "SETTINGS";
    togR.onclick = function() { sideR.classList.toggle("open"); };
    document.body.appendChild(togR);

    document.getElementById("cfg-go").onclick = function() {
        if (Module._get_ai_thinking()) return;

        var stones = parseInt(document.getElementById("cfg-stones").value) || 4;
        if (stones < 1)  stones = 1;
        if (stones > 16) stones = 16;

        var dist  = parseInt(document.getElementById("cfg-dist").value);
        var mode  = parseInt(document.getElementById("cfg-mode").value);
        var tl    = parseFloat(document.getElementById("cfg-time").value);
        if (isNaN(tl) || tl < 0) tl = 1;
        var start = parseInt(document.getElementById("cfg-start").value);

        Module._restart_game(stones, dist, mode, tl, start);
        sideR.classList.remove("open");
    };

    /* ── Main board area ─────────────────────────────────────────── */
    var main = document.createElement("div");
    main.className = "main-content";
    document.body.appendChild(main);

    var boardDiv = document.createElement("div");
    boardDiv.className = "board-container";
    boardDiv.id = "board";
    main.appendChild(boardDiv);

    /* P2 store (left) */
    var p2Store = document.createElement("div");
    p2Store.className = "store";
    p2Store.id = "cell-13";
    boardDiv.appendChild(p2Store);

    var rows = document.createElement("div");
    boardDiv.appendChild(rows);

    /* Top row – AI pits (12 → 7) */
    var topRow = document.createElement("div");
    topRow.className = "row";
    for (var i = 12; i >= 7; i--) {
        var p = document.createElement("div");
        p.className = "pit ai-pit";
        p.id = "cell-" + i;
        topRow.appendChild(p);
    }
    rows.appendChild(topRow);

    /* Bottom row – player pits (0 → 5) */
    var botRow = document.createElement("div");
    botRow.className = "row";
    for (var i = 0; i <= 5; i++) {
        var p = document.createElement("div");
        p.className = "pit player-pit";
        p.id = "cell-" + i;
        p.onclick = (function(idx) {
            return function() {
                if (Module._get_ai_thinking()) return;
                if (Module._get_current_player() !== 1) return;
                Module._do_web_move(idx);
            };
        })(i);
        botRow.appendChild(p);
    }
    rows.appendChild(botRow);

    /* P1 store (right) */
    var p1Store = document.createElement("div");
    p1Store.className = "store";
    p1Store.id = "cell-6";
    boardDiv.appendChild(p1Store);

    /* Status line */
    var statusText = document.createElement("div");
    statusText.style.marginTop = "20px";
    statusText.id = "status-text";
    main.appendChild(statusText);

    /* Footer */
    var footer = document.createElement("div");
    footer.className = "footer";
    footer.innerHTML = "<a href='https://github.com/LifesLight/CMancala' target='_blank'>CMancala v6.0</a>";
    document.body.appendChild(footer);

    /* ── View update ─────────────────────────────────────────────── */
    window.updateView = function() {
        var thinking = Module._get_ai_thinking();
        var gameOver = Module._get_game_over();
        var last     = Module._get_last_move();
        var board    = document.getElementById("board");

        board.classList.toggle("disabled", !!(thinking || gameOver));

        for (var i = 0; i < 14; i++) {
            var el = document.getElementById("cell-" + i);
            if (!el) continue;
            el.innerText = Module._get_stone_count(i);
            el.classList.toggle("highlight-move", i === last);
        }

        /* Stats panel */
        var solved  = Module._get_stat_solved();
        var depth   = Module._get_stat_depth();
        var evalVal = Module._get_stat_eval();
        var nodes   = Module._get_stat_nodes();
        var time    = Module._get_stat_time();

        document.getElementById("s-status").innerText =
            solved ? "SOLVED" : "DEPTH: " + depth;
        document.getElementById("s-eval").innerText =
            (evalVal === -9999) ? "N/A" : (evalVal > 0 ? "+" + evalVal : "" + evalVal);
        document.getElementById("s-nodes").innerText = /* This is now LAST nodes */
            (nodes / 1e6).toFixed(3) + " M";
        
        /* NEW: Update total nodes explored */
        var totalNodes = Module._get_stat_total_nodes();
        document.getElementById("s-total-nodes").innerText =
            (totalNodes / 1e6).toFixed(3) + " M";

        var tput = time > 0 ? ((nodes / time) / 1e6).toFixed(2) : "0.00";
        document.getElementById("s-tput").innerText = tput + " M n/s";

        /* Status text */
        var st = document.getElementById("status-text");
        if (gameOver)      st.innerText = ">> GAME OVER";
        else if (thinking) st.innerText = ">> AI THINKING...";
        else               st.innerText = ">> YOUR TURN";
    };

    setTimeout(updateView, 100);
});
#endif

/* ── Entry point ───────────────────────────────────────────────────── */

void startInterface() {
    init_game_with_config(4, 0, 0, 1.0, 1);
#ifdef __EMSCRIPTEN__
    launch_gui();
#endif
}

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

static Board boardHistory[MAX_HISTORY];
static int   boardHistoryMoves[MAX_HISTORY];
static int   boardHistoryCaptures[MAX_HISTORY];
static int   boardHistoryModified[MAX_HISTORY]; 
static int   historyCount = 0;

static int   currentCaptureMask = 0;
static int   currentModifiedMask = 0; 
static bool  isCheated = false;

static bool isGameInitialized = false;
static bool aiThinking = false;
static double totalNodesExplored = 0.0;

static int calc_capture_mask(const Board* prev, const Board* curr, int move) {
    if (move < 0 || move > 13) return 0;
    
    int player = prev->color;
    int stones = prev->cells[move];
    if (stones == 0) return 0;
    
    int last_pit = move;
    int stones_left = stones;
    int expected_sow_to_store = 0;
    
    // Calculate how many stones should land in the store purely via sowing
    while (stones_left > 0) {
        last_pit = (last_pit + 1) % 14;
        if (player == 1 && last_pit == 13) continue;
        if (player == -1 && last_pit == 6) continue;
        
        if (player == 1 && last_pit == 6) expected_sow_to_store++;
        if (player == -1 && last_pit == 13) expected_sow_to_store++;
        
        stones_left--;
    }
    
    int actual_store_diff = 0;
    if (player == 1) {
        actual_store_diff = curr->cells[6] - prev->cells[6];
    } else {
        actual_store_diff = curr->cells[13] - prev->cells[13];
    }
    
    int mask = 0;
    // If store gained more than the naturally sowed stones, it's a guaranteed capture.
    if (actual_store_diff > expected_sow_to_store) {
        // Only highlight the opponent's captured pit as red.
        mask |= (1 << (12 - last_pit));
    }
    
    return mask;
}

static int calc_modified_mask(const Board* prev, const Board* curr, int capMask) {
    int mask = 0;
    for (int i = 0; i < 14; i++) {
        if (prev->cells[i] != curr->cells[i]) mask |= (1 << i);
    }
    // If a capture happened, mathematically tag the pit that triggered it as modified (grey)
    if (capMask != 0) {
        for (int i = 0; i <= 12; i++) {
            if (i == 6) continue; 
            if (capMask & (1 << i)) mask |= (1 << (12 - i));
        }
    }
    return mask;
}

EMSCRIPTEN_KEEPALIVE int  get_stone_count(int i) { return isGameInitialized ? globalContext.board->cells[i] : 0; }
EMSCRIPTEN_KEEPALIVE int  get_current_player()   { return isGameInitialized ? globalContext.board->color : 0; }
EMSCRIPTEN_KEEPALIVE int  get_last_move()        { return isGameInitialized ? globalContext.metadata.lastMove : -1; }
EMSCRIPTEN_KEEPALIVE int  get_ai_thinking()      { return aiThinking; }
EMSCRIPTEN_KEEPALIVE int  get_game_over()        { return isGameInitialized && isBoardTerminal(globalContext.board); }

EMSCRIPTEN_KEEPALIVE int  get_current_captures() { 
    if (globalContext.config.gameSettings.moveFunction == AVALANCHE_MOVE) return 0;
    return currentCaptureMask; 
}
EMSCRIPTEN_KEEPALIVE int  get_current_modified() { return currentModifiedMask; } 

EMSCRIPTEN_KEEPALIVE int  get_is_cheated()       { return isCheated; }
EMSCRIPTEN_KEEPALIVE int  get_history_count()    { return historyCount; }

EMSCRIPTEN_KEEPALIVE int get_history_stone_count(int turn, int cell) {
    if (turn >= 0 && turn < historyCount) return boardHistory[turn].cells[cell];
    return 0;
}
EMSCRIPTEN_KEEPALIVE int get_history_move(int turn) {
    if (turn >= 0 && turn < historyCount) return boardHistoryMoves[turn];
    return -1;
}
EMSCRIPTEN_KEEPALIVE int get_history_captures(int turn) {
    if (globalContext.config.gameSettings.moveFunction == AVALANCHE_MOVE) return 0;
    if (turn >= 0 && turn < historyCount) return boardHistoryCaptures[turn];
    return 0;
}
EMSCRIPTEN_KEEPALIVE int get_history_modified(int turn) { 
    if (turn >= 0 && turn < historyCount) return boardHistoryModified[turn];
    return 0;
}

static void push_history(Board* b, int move, int capMask, int modMask) {
    if (historyCount < MAX_HISTORY) {
        boardHistory[historyCount] = *b;
        boardHistoryMoves[historyCount] = move;
        boardHistoryCaptures[historyCount] = capMask;
        boardHistoryModified[historyCount] = modMask;
        historyCount++;
    }
}

EMSCRIPTEN_KEEPALIVE
int can_undo() {
    if (historyCount <= 1 || aiThinking) return 0;
    for (int i = historyCount - 1; i > 0; i--) {
        if (boardHistory[i - 1].color == 1) return 1;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void undo_move() {
    if (aiThinking) return;
    int target = -1;
    for (int i = historyCount - 1; i > 0; i--) {
        if (boardHistory[i - 1].color == 1) {
            target = i - 1;
            break;
        }
    }
    if (target != -1) {
        isCheated = true;
        globalBoard = boardHistory[target];
        historyCount = target + 1;
        currentCaptureMask = 0;
        currentModifiedMask = 0;
        globalContext.metadata.lastMove = -1;
        EM_ASM({ updateView(); });
    }
}

EMSCRIPTEN_KEEPALIVE int    get_stat_solved() { return globalContext.metadata.lastSolved; }
EMSCRIPTEN_KEEPALIVE int    get_stat_depth()  { return globalContext.metadata.lastDepth; }
EMSCRIPTEN_KEEPALIVE double get_stat_nodes()  { return (double)globalContext.metadata.lastNodes; }
EMSCRIPTEN_KEEPALIVE double get_stat_time()   { return globalContext.metadata.lastTime; }
EMSCRIPTEN_KEEPALIVE double get_stat_total_nodes() { return totalNodesExplored; }
EMSCRIPTEN_KEEPALIVE int    get_stat_eval() {
    if (globalContext.metadata.lastEvaluation == INT32_MAX) return -9999;
    return globalContext.metadata.lastEvaluation;
}

static void init_game_with_config(int stones, int distribution, int moveFunc, double timeLimit, int startColor, int seedInput) {
    if (stones < 1) stones = 1;
    if (stones > 18) stones = 18;
    SolverConfig solverConfig = {.solver = LOCAL_SOLVER, .depth = 0, .timeLimit = timeLimit, .clip = false, .compressCache = AUTO, .progressBar = false};
    int actualSeed = seedInput;
    if (actualSeed == 0) actualSeed = (int)time(NULL);
    GameSettings gameSettings = {
        .stones = stones, 
        .distribution = distribution ? RANDOM_DIST : UNIFORM_DIST, 
        .seed = actualSeed,
        .startColor = startColor, 
        .player1 = HUMAN_AGENT, 
        .player2 = AI_AGENT, 
        .moveFunction = moveFunc ? AVALANCHE_MOVE : CLASSIC_MOVE
    };
    Config config = {.autoplay = false, .gameSettings = gameSettings, .solverConfig = solverConfig};
    srand(gameSettings.seed);
    initializeBoardFromConfig(&globalBoard, &config);
    setMoveFunction(config.gameSettings.moveFunction);
    globalContext.board = &globalBoard;
    globalContext.lastBoard = &globalLastBoard;
    globalContext.config = config;
    globalContext.metadata = (Metadata){.lastMove = -1, .lastEvaluation = INT32_MAX, .lastDepth = 0, .lastSolved = false, .lastTime = 0, .lastNodes = 0};
    isGameInitialized = true;
    aiThinking = false;
    isCheated = (distribution == RANDOM_DIST && seedInput > 0);
    totalNodesExplored = 0.0;
    currentCaptureMask = 0;
    currentModifiedMask = 0;
    historyCount = 0;
    push_history(globalContext.board, -1, 0, 0);
}

#ifdef __EMSCRIPTEN__
static void ai_think_callback(void *arg) {
    (void)arg;
    aspirationRoot(&globalContext, &globalContext.config.solverConfig);
    totalNodesExplored += globalContext.metadata.lastNodes;
    int move = globalContext.metadata.lastMove;
    if (move != -1) {
        copyBoard(globalContext.board, globalContext.lastBoard);
        makeMoveManual(globalContext.board, move);
        currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board, move);
        currentModifiedMask = calc_modified_mask(globalContext.lastBoard, globalContext.board, currentCaptureMask);
        push_history(globalContext.board, move, currentCaptureMask, currentModifiedMask);
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

EMSCRIPTEN_KEEPALIVE
void do_web_move(int index) {
    if (!isGameInitialized || aiThinking) return;
    if (globalContext.board->color != 1 || globalContext.board->cells[index] == 0) return;
    copyBoard(globalContext.board, globalContext.lastBoard);
    makeMoveManual(globalContext.board, index);
    globalContext.metadata.lastMove = index;
    currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board, index);
    currentModifiedMask = calc_modified_mask(globalContext.lastBoard, globalContext.board, currentCaptureMask);
    push_history(globalContext.board, index, currentCaptureMask, currentModifiedMask);
    EM_ASM({ updateView(); });
    if (!isBoardTerminal(globalContext.board) && globalContext.board->color == -1) kick_off_ai();
}

EMSCRIPTEN_KEEPALIVE
void restart_game(int stones, int distribution, int moveFunc, double timeLimit, int startColor, int seed) {
    init_game_with_config(stones, distribution, moveFunc, timeLimit, startColor, seed);
#ifdef __EMSCRIPTEN__
    EM_ASM({ updateView(); });
    if (globalContext.board->color == -1) kick_off_ai();
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(void, launch_gui, (const char* v_ptr), {
    document.body.innerHTML = "";
    const vStr = UTF8ToString(v_ptr);
    const css = `
        * { box-sizing: border-box; }
        body { background: #222; color: #fff; font-family: monospace; margin: 0; overflow: hidden; }
        .sidebar { position: fixed; top: 0; width: 280px; height: 100%; background: #2d2d2d; padding: 20px; padding-top: 50px; transition: transform .3s; z-index: 10; overflow-y: auto; }
        .sidebar-left { left: 0; border-right: 2px solid #505050; transform: translateX(-100%); }
        .sidebar-left.open { transform: translateX(0); }
        .sidebar-right { right: 0; border-left: 2px solid #505050; transform: translateX(100%); }
        .sidebar-right.open { transform: translateX(0); }
        .side-toggle { position: fixed; top: 10px; padding: 10px 15px; background: #3c3c3c; cursor: pointer; z-index: 11; border: 1px solid #646464; color: #fff; border-radius: 20px; }
        .toggle-left { left: 10px; }
        .toggle-right { right: 10px; }
        .stat-box { margin-bottom: 20px; border-bottom: 1px solid #464646; padding-bottom: 10px; }
        .stat-label { color: #969696; font-size: 12px; display: block; }
        .stat-val { font-size: 18px; color: #0f0; }
        .cfg-row { margin-bottom: 14px; }
        .cfg-label { color: #969696; font-size: 12px; display: block; margin-bottom: 4px; }
        .cfg-row input, .cfg-row select { width: 100%; padding: 6px; background: #444; color: #fff; border: 1px solid #646464; border-radius: 4px; }
        .cfg-btn { width: 100%; padding: 10px; margin-top: 10px; background: #0a0; color: #000; border: none; cursor: pointer; font-size: 16px; font-weight: bold; border-radius: 10px; }
        .bottom-btn { background: transparent; border: none; color: #888; font-size: 14px; cursor: pointer; font-family: monospace; }
        .bottom-btn:hover { color: #fff; text-decoration: underline; }
        .main-content { display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; }
        .board-container-relative { position: relative; }
        .board-wrapper { display: flex; background: #333; border-radius: 10px; transition: background 0.5s; }
        .board-wrapper.cheated { background: #3d3336; }
        .board-wrapper.main { padding: 20px; gap: 20px; }
        .board-wrapper.mini { padding: 10px; gap: 10px; border-radius: 6px; background: #2a2a2a; }
        .pit { border-radius: 50%; display: flex; align-items: center; justify-content: center; background: #444; transition: background 0.2s, color 0.2s; }
        .store { border-radius: 20px; display: flex; align-items: center; justify-content: center; background: #555; transition: background 0.2s, color 0.2s; }
        .row { display: flex; }
        .main .pit { width: 60px; height: 60px; font-size: 24px; border: 2px solid #555; }
        .main .store { width: 80px; font-size: 32px; border: 2px solid #666; }
        .main .row { gap: 10px; margin: 10px 0; }
        .mini .pit { width: 38px; height: 38px; font-size: 14px; border: 1px solid #444; }
        .mini .store { width: 50px; font-size: 18px; border: 1px solid #555; border-radius: 10px; }
        .mini .row { gap: 5px; margin: 5px 0; }
        .player-pit { cursor: pointer; border-color: #0f0 !important; }
        .ai-pit { border-color: #f00 !important; }
        .highlight-modified { background: #5a5a5a !important; }
        .highlight-move { background: #999 !important; color: #000; }
        .highlight-capture { background-color: #e73121 !important; color: #fff; }
        .highlight-last-drop { background: #6e6e6e !important; color: #000; }
        .disabled { opacity: .5; pointer-events: none; }
        .undo-pos { position: absolute; bottom: -35px; left: 30px; }
        .history-container { display: none; flex-direction: column; gap: 20px; padding: 15px; background: #1a1a1a; border-radius: 10px; max-height: 35vh; overflow-y: auto; margin-top: 10px; border: 1px solid #444; }
        .hist-item { display: flex; justify-content: center; border-bottom: 1px solid #333; padding-bottom: 15px; }
        .hist-item:last-child { border-bottom: none; padding-bottom: 0; }
        .footer { position: fixed; bottom: 10px; width: 100%; text-align: center; pointer-events: none; }
        .footer a { color: #888; text-decoration: none; font-size: 14px; pointer-events: auto; }
    `;
    const style = document.createElement("style"); style.textContent = css; document.head.appendChild(style);
    window.createBoardDOM = function(container, prefix, isMini) {
        container.innerHTML = "";
        const wrap = document.createElement("div"); wrap.className = "board-wrapper " + (isMini ? "mini" : "main"); wrap.id = prefix + "board";
        const p2Store = document.createElement("div"); p2Store.className = "store"; p2Store.id = prefix + "13"; wrap.appendChild(p2Store);
        const rows = document.createElement("div");
        const tRow = document.createElement("div"); tRow.className = "row";
        for (let i = 12; i >= 7; i--) { const p = document.createElement("div"); p.className = "pit ai-pit"; p.id = prefix + i; tRow.appendChild(p); }
        const bRow = document.createElement("div"); bRow.className = "row";
        for (let i = 0; i <= 5; i++) {
            const p = document.createElement("div"); p.className = "pit player-pit"; p.id = prefix + i;
            if (!isMini) {
                p.onclick = () => { if (Module._get_ai_thinking() || Module._get_current_player() !== 1) return; Module._do_web_move(i); };
                p.onmouseenter = () => { window.hoveredPit = i; window.updateView(); };
                p.onmouseleave = () => { window.hoveredPit = -1; window.updateView(); };
            }
            bRow.appendChild(p);
        }
        rows.appendChild(tRow); rows.appendChild(bRow); wrap.appendChild(rows);
        const p1Store = document.createElement("div"); p1Store.className = "store"; p1Store.id = prefix + "6"; wrap.appendChild(p1Store);
        container.appendChild(wrap);
    };
    const sideL = document.createElement("div"); sideL.className = "sidebar sidebar-left";
    sideL.innerHTML = `<h3>SOLVER STATS</h3><div class='stat-box'><span class='stat-label'>STATUS</span><span id='s-status' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>EVALUATION</span><span id='s-eval' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>LAST NODES</span><span id='s-nodes' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>TOTAL NODES</span><span id='s-total-nodes' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>THROUGHPUT</span><span id='s-tput' class='stat-val'>-</span></div>`;
    document.body.appendChild(sideL);
    const togL = document.createElement("button"); togL.className = "side-toggle toggle-left"; togL.innerText = "STATS";
    togL.onclick = () => sideL.classList.toggle("open"); document.body.appendChild(togL);
    const sideR = document.createElement("div"); sideR.className = "sidebar sidebar-right";
    sideR.innerHTML = `<h3>GAME SETTINGS</h3><div class='cfg-row'><span class='cfg-label'>STONES (1-18)</span><input id='cfg-stones' type='number' value='4' min='1' max='18' oninput="if(this.value > 18) this.value = 18; if(this.value < 1 && this.value !== '') this.value = 1;"></div><div class='cfg-row'><span class='cfg-label'>DISTRIBUTION</span><select id='cfg-dist'><option value='0'>Uniform</option><option value='1'>Random</option></select></div><div class='cfg-row' id='seed-row' style='display:none'><span class='cfg-label'>SEED (0=rand)</span><input id='cfg-seed' type='number' value='0'></div><div class='cfg-row'><span class='cfg-label'>MODE</span><select id='cfg-mode'><option value='0'>Classic</option><option value='1'>Avalanche</option></select></div><div class='cfg-row'><span class='cfg-label'>AI TIME LIMIT (s, 0=inf)</span><input id='cfg-time' type='number' value='1'></div><div class='cfg-row'><span class='cfg-label'>STARTING PLAYER</span><select id='cfg-start'><option value='1'>Player</option><option value='-1'>AI</option></select></div><button id='cfg-go' class='cfg-btn'>START GAME</button>`;
    document.body.appendChild(sideR);
    document.getElementById('cfg-dist').onchange = (e) => { document.getElementById('seed-row').style.display = e.target.value == '1' ? 'block' : 'none'; };
    const togR = document.createElement("button"); togR.className = "side-toggle toggle-right"; togR.innerText = "SETTINGS";
    togR.onclick = () => sideR.classList.toggle("open"); document.body.appendChild(togR);
    document.getElementById("cfg-go").onclick = () => {
        if (Module._get_ai_thinking()) return;
        Module._restart_game(parseInt(document.getElementById("cfg-stones").value), parseInt(document.getElementById("cfg-dist").value), parseInt(document.getElementById("cfg-mode").value), parseFloat(document.getElementById("cfg-time").value), parseInt(document.getElementById("cfg-start").value), parseInt(document.getElementById("cfg-seed").value));
        sideR.classList.remove("open");
    };
    const main = document.createElement("div"); main.className = "main-content"; document.body.appendChild(main);
    const bRel = document.createElement("div"); bRel.className = "board-container-relative"; main.appendChild(bRel);
    const bCont = document.createElement("div"); bRel.appendChild(bCont); window.createBoardDOM(bCont, "main-", false);
    const uBtn = document.createElement("button"); uBtn.id = "undo-btn"; uBtn.className = "bottom-btn undo-pos"; uBtn.innerText = "undo move"; uBtn.onclick = () => Module._undo_move(); bRel.appendChild(uBtn);
    const sTxt = document.createElement("div"); sTxt.id = "status-text"; sTxt.style.marginTop = "25px"; sTxt.style.marginBottom = "15px"; sTxt.style.fontSize = "18px"; main.appendChild(sTxt);
    const hBtn = document.createElement("button"); hBtn.id = "hist-btn"; hBtn.className = "bottom-btn"; hBtn.style.marginTop = "0px"; hBtn.innerText = "▼ history ▼";
    hBtn.onclick = () => { const hc = document.getElementById("history-container"); if (hc.style.display === "none" || !hc.style.display) { hc.style.display = "flex"; hBtn.innerText = "▲ history ▲"; window.syncHistory(); } else { hc.style.display = "none"; hBtn.innerText = "▼ history ▼"; } };
    main.appendChild(hBtn);
    const hCont = document.createElement("div"); hCont.id = "history-container"; hCont.className = "history-container"; main.appendChild(hCont);
    document.body.insertAdjacentHTML('beforeend', "<div class='footer'><a href='https://github.com/LifesLight/CMancala' target='_blank'>CMancala v" + vStr + "</a></div>");
    
    window.syncHistory = function() {
        const hc = document.getElementById("history-container"); if (hc.style.display === "none") return;
        const count = Module._get_history_count(); hc.innerHTML = "";
        for (let t = count - 2; t >= 0; t--) {
            const item = document.createElement("div"); item.className = "hist-item";
            const bC = document.createElement("div"); window.createBoardDOM(bC, "h" + t + "-", true); item.appendChild(bC); hc.appendChild(item);
            const mv = Module._get_history_move(t); 
            const cp = Module._get_history_captures(t);
            const md = Module._get_history_modified(t);
            for (let i = 0; i < 14; i++) {
                const el = document.getElementById("h" + t + "-" + i);
                if (el) { 
                    el.innerText = Module._get_history_stone_count(t, i); 
                    el.classList.remove("highlight-capture", "highlight-move", "highlight-modified");
                    if (cp & (1 << i)) el.classList.add("highlight-capture");
                    else if (i === mv) el.classList.add("highlight-move");
                    else if (md & (1 << i)) el.classList.add("highlight-modified");
                }
            }
        }
        if (count <= 1) hc.innerHTML = "<div style='text-align:center; color:#666; padding: 15px;'>No past moves to show</div>";
    };

    window.updateView = function() {
        const thinking = Module._get_ai_thinking(); const gameOver = Module._get_game_over();
        const mBoard = document.getElementById("main-board");
        mBoard.classList.toggle("disabled", !!(thinking || gameOver));
        mBoard.classList.toggle("cheated", !!Module._get_is_cheated());
        document.getElementById("undo-btn").classList.toggle("disabled", !Module._can_undo());
        
        const lastMove = Module._get_last_move();
        const captures = Module._get_current_captures();
        const modified = Module._get_current_modified();

        const hoveredPit = window.hoveredPit !== undefined ? window.hoveredPit : -1;
        let drops = [];
        let lastDrop = -1;
        if (hoveredPit >= 0 && !thinking && !gameOver && Module._get_current_player() === 1) {
            let stones = Module._get_stone_count(hoveredPit);
            if (stones > 0) {
                let curr = hoveredPit;
                while (stones > 0) {
                    curr = (curr + 1) % 14;
                    if (curr === 13) continue;
                    if (!drops.includes(curr)) drops.push(curr);
                    stones--;
                }
                lastDrop = curr;
            }
        }

        for (let i = 0; i < 14; i++) {
            const el = document.getElementById("main-" + i); if (!el) continue;
            el.innerText = Module._get_stone_count(i);
            
            el.classList.remove("highlight-capture", "highlight-move", "highlight-modified", "highlight-last-drop");
            
            if (lastDrop !== -1) {
                if (i === lastDrop) el.classList.add("highlight-last-drop");
                else if (drops.includes(i)) el.classList.add("highlight-modified");
            } else {
                if (captures & (1 << i)) el.classList.add("highlight-capture");
                else if (i === lastMove) el.classList.add("highlight-move");
                else if (modified & (1 << i)) el.classList.add("highlight-modified");
            }
        }
        
        document.getElementById("s-status").innerText = Module._get_stat_solved() ? "SOLVED" : "DEPTH: " + Module._get_stat_depth();
        let ev = Module._get_stat_eval(); document.getElementById("s-eval").innerText = (ev === -9999) ? "N/A" : (ev > 0 ? "+" + ev : ev);
        document.getElementById("s-nodes").innerText = (Module._get_stat_nodes() / 1e6).toFixed(3) + " M";
        document.getElementById("s-total-nodes").innerText = (Module._get_stat_total_nodes() / 1e6).toFixed(3) + " M";
        let ti = Module._get_stat_time(); document.getElementById("s-tput").innerText = (ti > 0 ? ((Module._get_stat_nodes() / ti) / 1e6).toFixed(2) : "0.00") + " M n/s";
        const st = document.getElementById("status-text");
        if (gameOver) st.innerText = ">> GAME OVER"; else if (thinking) st.innerText = ">> AI THINKING..."; else st.innerText = ">> YOUR TURN";
        window.syncHistory();
    };
    setTimeout(updateView, 100);
});
#endif
void startInterface() {
    init_game_with_config(4, 0, 0, 1.0, 1, 0);
#ifdef __EMSCRIPTEN__
    launch_gui(MANCALA_VERSION);
#endif
}

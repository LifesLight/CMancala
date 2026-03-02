#include "user/interface.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "logic/solver/cache.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <zstd.h>
#include <sys/stat.h>
#endif

/**
 * Requires Prebuild EGDB's
 * 
 * Example:
 * cat EGDB/egdb_{17..20}.bin > egdb_bundle_upgrade.bin
 * zstd -19 egdb_bundle_upgrade.bin -o egdb17-20.zst
 */

// Extern reference
extern void setCacheSize(int size);
extern void fillCacheStats(CacheStats* stats, bool calcFrag, bool calcStoneDist, bool calcDepthDist);
extern void generateEGDB(int max_stones);
extern void getEGDBStats(uint64_t* sizeBytes, uint64_t* hits, int* minStones, int* maxStones);
extern void configureStoneCountEGDB(int totalStones);
extern void freeEGDB(void);

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
static bool isAutoplay = true; 
static double totalNodesExplored = 0.0;

// Web Stats Globals
static double webStatCacheFill = 0.0;
static double webStatCacheSolved = 0.0;
static double webStatLRUSwap = 0.0;
static uint64_t webStatImprov = 0;
static uint64_t webStatEvict = 0;
static uint64_t webStatHits = 0;
static double webStatExact = 0.0;
static double webStatLower = 0.0;
static double webStatUpper = 0.0;
static int webStatHasDepth = 0;

/* ================== UI HIGHLIGHT LOGIC ================== */

static int calc_capture_mask(const Board* prev, const Board* curr, int move) {
    if (getMoveFunction() == AVALANCHE_MOVE) {
        return 0;
    }

    if (move < 0 || move > 13) return 0;

    int player = prev->color;
    int stones = prev->cells[move];
    if (stones == 0) return 0;
    
    int last_pit = move;
    int stones_left = stones;
    int expected_sow_to_store = 0;
    
    while (stones_left > 0) {
        last_pit = (last_pit + 1) % 14;
        if (player == 1 && last_pit == 13) continue;
        if (player == -1 && last_pit == 6) continue;
        
        if (player == 1 && last_pit == 6) expected_sow_to_store++;
        if (player == -1 && last_pit == 13) expected_sow_to_store++;
        
        stones_left--;
    }
    
    int actual_store_diff = (player == 1) ? (curr->cells[6] - prev->cells[6]) : (curr->cells[13] - prev->cells[13]);
    
    int mask = 0;
    if (actual_store_diff > expected_sow_to_store) {
        mask |= (1 << (12 - last_pit)); 
    }
    
    return mask;
}

static int calc_modified_mask(const Board* prev, const Board* curr, int capMask) {
    int mask = 0;
    for (int i = 0; i < 14; i++) {
        if (prev->cells[i] != curr->cells[i]) mask |= (1 << i);
    }
    if (capMask != 0) {
        for (int i = 0; i <= 12; i++) {
            if (i == 6) continue; 
            if (capMask & (1 << i)) mask |= (1 << (12 - i));
        }
    }
    return mask;
}

/* ================== EMSCRIPTEN EXPORTS ================== */

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void* web_malloc(size_t size) { return malloc(size); }
EMSCRIPTEN_KEEPALIVE void web_free(void* ptr) { free(ptr); }

EMSCRIPTEN_KEEPALIVE 
int load_egdb_to_vfs(uint8_t* comp_data, size_t comp_size, int min_stones, int max_stones) {
    uint64_t ways[21][13] = {0};
    for (int p = 1; p <= 12; p++) ways[0][p] = 1;
    for (int s = 0; s <= 20; s++) ways[s][1] = 1;
    for (int p = 2; p <= 12; p++) {
        for (int s = 1; s <= 20; s++) {
            ways[s][p] = ways[s][p - 1] + ways[s - 1][p];
        }
    }

    unsigned long long const dSize = ZSTD_getFrameContentSize(comp_data, comp_size);
    if (dSize == ZSTD_CONTENTSIZE_ERROR || dSize == ZSTD_CONTENTSIZE_UNKNOWN) return 0;

    uint8_t* d_data = malloc(dSize);
    if (!d_data) return 0;

    size_t const dec_size = ZSTD_decompress(d_data, dSize, comp_data, comp_size);
    if (ZSTD_isError(dec_size)) {
        free(d_data);
        return 0;
    }

    mkdir("EGDB", 0777); 

    size_t offset = 0;
    for (int s = min_stones; s <= max_stones; s++) {
        size_t layer_size = ways[s][12];
        if (offset + layer_size > dec_size) break;

        char path[256];
        snprintf(path, sizeof(path), "EGDB/egdb_%d.bin", s);
        FILE* f = fopen(path, "wb");
        if (f) {
            fwrite(d_data + offset, 1, layer_size, f);
            fclose(f);
        }
        offset += layer_size;
    }
    free(d_data);

    freeEGDB();
    generateEGDB(max_stones);
    return 1;
}
#endif

EMSCRIPTEN_KEEPALIVE int  get_stone_count(int i) { return isGameInitialized ? globalContext.board->cells[i] : 0; }
EMSCRIPTEN_KEEPALIVE int  get_current_player()   { return isGameInitialized ? globalContext.board->color : 0; }
EMSCRIPTEN_KEEPALIVE int  get_last_move()        { return isGameInitialized ? globalContext.metadata.lastMove : -1; }
EMSCRIPTEN_KEEPALIVE int  get_ai_thinking()      { return aiThinking; }
EMSCRIPTEN_KEEPALIVE int  get_game_over()        { return isGameInitialized && isBoardTerminal(globalContext.board); }

EMSCRIPTEN_KEEPALIVE int  get_current_captures() { 
    if (getMoveFunction() == AVALANCHE_MOVE) return 0;
    return currentCaptureMask; 
}

EMSCRIPTEN_KEEPALIVE int  get_current_modified() { 
    if (getMoveFunction() == AVALANCHE_MOVE) return 0;
    return currentModifiedMask; 
} 

EMSCRIPTEN_KEEPALIVE int  get_is_cheated()       { return isCheated; }
EMSCRIPTEN_KEEPALIVE int  get_history_count()    { return historyCount; }
EMSCRIPTEN_KEEPALIVE int  get_autoplay()         { return isAutoplay; }

EMSCRIPTEN_KEEPALIVE int get_history_stone_count(int turn, int cell) {
    if (turn >= 0 && turn < historyCount) return boardHistory[turn].cells[cell];
    return 0;
}
EMSCRIPTEN_KEEPALIVE int get_history_move(int turn) {
    if (turn >= 0 && turn < historyCount) return boardHistoryMoves[turn];
    return -1;
}
EMSCRIPTEN_KEEPALIVE int get_history_captures(int turn) {
    if (getMoveFunction() == AVALANCHE_MOVE) return 0;
    if (turn >= 0 && turn < historyCount) return boardHistoryCaptures[turn];
    return 0;
}
EMSCRIPTEN_KEEPALIVE int get_history_modified(int turn) { 
    if (getMoveFunction() == AVALANCHE_MOVE) return 0;
    if (turn >= 0 && turn < historyCount) return boardHistoryModified[turn];
    return 0;
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

#ifdef __EMSCRIPTEN__
static void kick_off_ai(void);
#endif

EMSCRIPTEN_KEEPALIVE void set_web_cache_size(int size) {
    if (aiThinking) return;
    setCacheSize(size);
}

EMSCRIPTEN_KEEPALIVE void set_autoplay(int val) {
    isAutoplay = !!val;
    if (isAutoplay && !aiThinking && isGameInitialized && globalContext.board->color == -1 && !isBoardTerminal(globalContext.board)) {
        kick_off_ai();
    }
}

EMSCRIPTEN_KEEPALIVE void do_ai_step() {
    if (!isGameInitialized || aiThinking) return;
    if (globalContext.board->color == -1 && !isBoardTerminal(globalContext.board)) {
        kick_off_ai();
    }
}

// Calculate stats
EMSCRIPTEN_KEEPALIVE void update_web_cache_stats() {
    if (aiThinking) return;
    CacheStats stats;
    fillCacheStats(&stats, false, false, false);

    webStatImprov = stats.overwriteImprove;
    webStatEvict = stats.overwriteEvict;
    webStatHits = stats.hits;
    webStatHasDepth = stats.hasDepth ? 1 : 0;

    if (stats.cacheSize > 0) {
        webStatCacheFill = (double)stats.setEntries / (double)stats.cacheSize * 100.0;
    } else {
        webStatCacheFill = 0.0;
    }

    if (stats.setEntries > 0) {
        webStatCacheSolved = (double)stats.solvedEntries / (double)stats.setEntries * 100.0;
        webStatExact = (double)stats.exactCount / (double)stats.setEntries * 100.0;
        webStatLower = (double)stats.lowerCount / (double)stats.setEntries * 100.0;
        webStatUpper = (double)stats.upperCount / (double)stats.setEntries * 100.0;
    } else {
        webStatCacheSolved = 0.0;
        webStatExact = 0.0; webStatLower = 0.0; webStatUpper = 0.0;
    }

    if (stats.hits > 0) {
        webStatLRUSwap = (double)stats.lruSwaps / (double)stats.hits * 100.0;
    } else {
        webStatLRUSwap = 0.0;
    }
}

EMSCRIPTEN_KEEPALIVE double get_c_fill()   { return webStatCacheFill; }
EMSCRIPTEN_KEEPALIVE double get_c_solved() { return webStatCacheSolved; }
EMSCRIPTEN_KEEPALIVE double get_c_swap()   { return webStatLRUSwap; }
EMSCRIPTEN_KEEPALIVE double get_c_imp()    { return (double)webStatImprov; }
EMSCRIPTEN_KEEPALIVE double get_c_evi()    { return (double)webStatEvict; }
EMSCRIPTEN_KEEPALIVE double get_c_hits()   { return (double)webStatHits; }
EMSCRIPTEN_KEEPALIVE double get_c_exact()  { return webStatExact; }
EMSCRIPTEN_KEEPALIVE double get_c_lower()  { return webStatLower; }
EMSCRIPTEN_KEEPALIVE double get_c_upper()  { return webStatUpper; }
EMSCRIPTEN_KEEPALIVE int    get_c_has_depth() { return webStatHasDepth; }

EMSCRIPTEN_KEEPALIVE double get_egdb_hits() {
    uint64_t size, hits; int minS, maxS;
    getEGDBStats(&size, &hits, &minS, &maxS);
    return (double)hits;
}
EMSCRIPTEN_KEEPALIVE double get_egdb_size_mb() {
    uint64_t size, hits; int minS, maxS;
    getEGDBStats(&size, &hits, &minS, &maxS);
    return (double)size / 1048576.0;
}
EMSCRIPTEN_KEEPALIVE int get_egdb_max_stones() {
    uint64_t size, hits; int minS, maxS;
    getEGDBStats(&size, &hits, &minS, &maxS);
    return maxS;
}

/* ================== GAME ENGINE ================== */

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

static void init_game_with_config(int stones, int distribution, int moveFunc, double timeLimit, int startColor, int seedInput) {
    if (stones < 1) stones = 1;
    if (stones > 18) stones = 18;

    configureStoneCountEGDB(stones * 12);

    SolverConfig solverConfig = {
        .solver = LOCAL_SOLVER, 
        .depth = 0, 
        .timeLimit = timeLimit, 
        .clip = false, 
        .compressCache = AUTO, 
        .progressBar = false
    };
    int actualSeed = seedInput;
    if (actualSeed == 0) actualSeed = (int)time(NULL);
    GameSettings gameSettings = {
        .stones = stones, 
        .distribution = distribution ? RANDOM_DIST : UNIFORM_DIST, 
        .seed = actualSeed,
        .startColor = startColor, 
        .player1 = HUMAN_AGENT, 
        .player2 = AI_AGENT
    };
    Config config = {.autoplay = false, .gameSettings = gameSettings, .solverConfig = solverConfig};
    
    setMoveFunction(moveFunc ? AVALANCHE_MOVE : CLASSIC_MOVE);

    srand(gameSettings.seed);
    initializeBoardFromConfig(&globalBoard, &config);
    
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

EMSCRIPTEN_KEEPALIVE
void set_custom_board(int c0, int c1, int c2, int c3, int c4, int c5, int c6,
                      int c7, int c8, int c9, int c10, int c11, int c12, int c13,
                      int startColor) {
    if (!isGameInitialized) return;
    globalBoard.cells[0] = c0;   globalBoard.cells[1] = c1;
    globalBoard.cells[2] = c2;   globalBoard.cells[3] = c3;
    globalBoard.cells[4] = c4;   globalBoard.cells[5] = c5;
    globalBoard.cells[6] = c6;   globalBoard.cells[7] = c7;
    globalBoard.cells[8] = c8;   globalBoard.cells[9] = c9;
    globalBoard.cells[10] = c10; globalBoard.cells[11] = c11;
    globalBoard.cells[12] = c12; globalBoard.cells[13] = c13;
    globalBoard.color = startColor;

    configureStoneCountEGDB(c0 + c1 + c2 + c3 + c4 + c5 + c6 + 
                            c7 + c8 + c9 + c10 + c11 + c12 + c13);

    isCheated = true;
    historyCount = 0;
    push_history(globalContext.board, -1, 0, 0);
}

EMSCRIPTEN_KEEPALIVE
void finalize_board() {
    if (!isGameInitialized) return;
    if (isBoardTerminal(globalContext.board)) {
        processBoardTerminal(globalContext.board);
        historyCount = 0;
        push_history(globalContext.board, -1, 0, 0);
    }
}

EMSCRIPTEN_KEEPALIVE
int get_last_seed() {
    return globalContext.config.gameSettings.seed;
}

EMSCRIPTEN_KEEPALIVE
void clear_highlights() {
    currentCaptureMask = 0;
    currentModifiedMask = 0;
    globalContext.metadata.lastMove = -1;
}

#ifdef __EMSCRIPTEN__
static void ai_think_callback(void *arg) {
    (void)arg;
    aspirationRoot(&globalContext, &globalContext.config.solverConfig);
    totalNodesExplored += globalContext.metadata.lastNodes;
    int move = globalContext.metadata.lastMove;
    if (move != -1) {
        *globalContext.lastBoard = *globalContext.board;
        makeMoveManual(globalContext.board, move);
        currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board, move);
        currentModifiedMask = calc_modified_mask(globalContext.lastBoard, globalContext.board, currentCaptureMask);
        push_history(globalContext.board, move, currentCaptureMask, currentModifiedMask);
    }
    EM_ASM({ updateView(); });
    
    if (!isBoardTerminal(globalContext.board) && globalContext.board->color == -1 && isAutoplay) {
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
    *globalContext.lastBoard = *globalContext.board;
    makeMoveManual(globalContext.board, index);
    globalContext.metadata.lastMove = index;
    currentCaptureMask = calc_capture_mask(globalContext.lastBoard, globalContext.board, index);
    currentModifiedMask = calc_modified_mask(globalContext.lastBoard, globalContext.board, currentCaptureMask);
    push_history(globalContext.board, index, currentCaptureMask, currentModifiedMask);
    EM_ASM({ updateView(); });

    if (!isBoardTerminal(globalContext.board) && globalContext.board->color == -1) {
        if (isAutoplay) kick_off_ai();
    }
}

EMSCRIPTEN_KEEPALIVE
void restart_game(int stones, int distribution, int moveFunc, double timeLimit, int startColor, int seed) {
    init_game_with_config(stones, distribution, moveFunc, timeLimit, startColor, seed);
#ifdef __EMSCRIPTEN__
    EM_ASM({ updateView(); });
#endif
}

/* ================== FRONTEND GUI ================== */

#ifdef __EMSCRIPTEN__
EM_JS(void, launch_gui, (const char* v_ptr), {
    document.body.innerHTML = "";
    document.body.classList.remove("loading");
    
    const vStr = UTF8ToString(v_ptr);

    window.isTouchDevice = false;
    window.addEventListener('touchstart', () => { window.isTouchDevice = true; }, { passive: true });

    window.domCache = {};
    window.getEl = function(id) {
        let el = window.domCache[id];
        if (!el) { el = document.getElementById(id); window.domCache[id] = el; }
        return el;
    };
    window.lastHistoryCount = -1;
    window.hoverTimeout = null;
    window.customEditMode = false;
    window.inSetup = false;
    window.lastUsedSeed = 0;
    window.egdbReady = false;

    window.createBoardDOM = function(container, prefix, isMini) {
        container.innerHTML = "";
        const wrap = document.createElement("div"); wrap.className = "board-wrapper " + (isMini ? "mini" : "main"); wrap.id = prefix + "board";
        const p2Store = document.createElement("div"); p2Store.className = "store"; p2Store.id = prefix + "13"; wrap.appendChild(p2Store);
        const rows = document.createElement("div");
        const tRow = document.createElement("div"); tRow.className = "row";
        
        for (let i = 12; i >= 7; i--) { 
            const p = document.createElement("div"); p.className = "pit ai-pit"; p.id = prefix + i; tRow.appendChild(p); 
        }
        
        const bRow = document.createElement("div"); bRow.className = "row";
        for (let i = 0; i <= 5; i++) {
            const p = document.createElement("div"); p.className = "pit player-pit"; p.id = prefix + i;
            if (!isMini) {
                p.onclick = () => { 
                    if (window.customEditMode || window.inSetup || !window.egdbReady) return;
                    if (Module._get_ai_thinking() || Module._get_current_player() !== 1) return; 
                    if (window.hoverTimeout) { clearTimeout(window.hoverTimeout); window.hoverTimeout = null; }
                    window.hoveredPit = -1; 
                    Module._do_web_move(i); 
                };
                p.onmouseenter = () => { 
                    if (window.customEditMode || window.inSetup || !window.egdbReady) return;
                    if (!window.isTouchDevice) { 
                        if (window.hoverTimeout) { clearTimeout(window.hoverTimeout); window.hoverTimeout = null; }
                        window.hoveredPit = i; 
                        window.updateView(); 
                    } 
                };
                p.onmouseleave = () => { 
                    if (window.customEditMode || window.inSetup || !window.egdbReady) return;
                    if (!window.isTouchDevice) { 
                        window.hoverTimeout = setTimeout(() => {
                            window.hoveredPit = -1; 
                            window.updateView(); 
                        }, 50);
                    } 
                };
            }
            bRow.appendChild(p);
        }
        rows.appendChild(tRow); rows.appendChild(bRow); wrap.appendChild(rows);
        
        const p1Store = document.createElement("div"); p1Store.className = "store"; p1Store.id = prefix + "6"; wrap.appendChild(p1Store);
        container.appendChild(wrap);
    };

    window.clearBoardHighlights = function() {
        for (let i = 0; i < 14; i++) {
            const el = document.getElementById("main-" + i);
            if (!el) continue;
            const old = el.getAttribute("data-hl") || "";
            if (old) { el.classList.remove(old); el.setAttribute("data-hl", ""); }
        }
    };

    window.enterCustomEdit = function(values) {
        window.customEditMode = true;
        window.domCache = {};
        Module._clear_highlights();
        window.clearBoardHighlights();
        for (let i = 0; i < 14; i++) {
            const el = document.getElementById("main-" + i);
            if (!el) continue;
            const isStore = (i === 6 || i === 13);
            const val = values ? values[i] : 0;
            const inp = document.createElement("input");
            inp.type = "number";
            inp.className = isStore ? "store-input" : "pit-input";
            inp.value = val;
            inp.min = 0;
            inp.max = 16;
            inp.inputMode = "numeric";
            inp.id = "cst-" + i;
            inp.onclick = (e) => { e.stopPropagation(); };
            el.textContent = "";
            el.appendChild(inp);
        }
        const mBoard = document.getElementById("main-board");
        if (mBoard) { mBoard.classList.remove("disabled"); mBoard.classList.add("cheated"); }
    };

    window.exitCustomEdit = function() {
        window.customEditMode = false;
        window.domCache = {};
        for (let i = 0; i < 14; i++) {
            const el = document.getElementById("main-" + i);
            if (!el) continue;
            el.innerHTML = "";
            el.textContent = String(Module._get_stone_count(i));
        }
    };

    window.readCustomBoard = function() {
        const cells = [];
        for (let i = 0; i < 14; i++) {
            const inp = document.getElementById("cst-" + i);
            let v = inp ? parseInt(inp.value) : 0;
            if (isNaN(v) || v < 0) v = 0;
            if (v > 16) v = 16;
            cells.push(v);
        }
        return cells;
    };

    window.updateButtons = function() {
        const goBtn = document.getElementById("cfg-go");
        const resetBtn = document.getElementById("cfg-reset");
        const dist = parseInt(document.getElementById("cfg-dist").value);
        const isCustom = (dist === 2);
        if (goBtn) goBtn.classList.toggle("disabled", isCustom && !window.inSetup);
        if (resetBtn) resetBtn.classList.toggle("disabled", window.inSetup);
    };

    window.readSettings = function() {
        clampInput("cfg-stones", 1, 18);
        return {
            stones: parseInt(document.getElementById("cfg-stones").value),
            dist: parseInt(document.getElementById("cfg-dist").value),
            moveFunc: parseInt(document.getElementById("cfg-mode").value),
            timeVal: (function() { let t = parseFloat(document.getElementById("cfg-time").value); if (isNaN(t) || t < 0) { t = 0; document.getElementById("cfg-time").value = "0"; } return t; })(),
            startColor: parseInt(document.getElementById("cfg-start").value),
            seed: parseInt(document.getElementById("cfg-seed").value) || 0
        };
    };

    const sideL = document.createElement("div"); sideL.className = "sidebar sidebar-left";
    let statsHtml = `<h3>SOLVER STATS</h3><div class='stat-box'><span class='stat-label'>STATUS</span><span id='s-status' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>EVALUATION</span><span id='s-eval' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>LAST NODES</span><span id='s-nodes' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>TOTAL NODES</span><span id='s-total-nodes' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>THROUGHPUT</span><span id='s-tput' class='stat-val'>-</span></div>`;
    
    statsHtml += `<div class='stat-box'><span class='stat-label'>LAST TIME</span><span id='s-time' class='stat-val'>-</span></div>`;
    statsHtml += `<div class='expert-only'><h3>CACHE STATS</h3><div class='stat-box'><span class='stat-label'>CACHE USED</span><span id='c-fill' class='stat-val'>0.00%</span></div><div class='stat-box'><span class='stat-label'>CACHE SOLVED</span><span id='c-solved' class='stat-val'>0.00%</span></div><div class='stat-box'><span class='stat-label'>LAST CACHE HITS</span><span id='c-hits' class='stat-val'>0</span></div><div class='stat-box'><span class='stat-label'>LRU SWAP RATE</span><span id='c-swap' class='stat-val'>0.00%</span></div><div class='stat-box'><span class='stat-label'>OVERWRITES</span><span class='stat-sub'>IMPROVE: <span id='c-imp'>0</span></span><span class='stat-sub'>EVICT: <span id='c-evi'>0</span></span></div><div class='stat-box'><span class='stat-label'>BOUNDS</span><span class='stat-sub'>EXACT: <span id='c-ex'>0.0</span>%</span><span class='stat-sub'>LOWER: <span id='c-lo'>0.0</span>%</span><span class='stat-sub'>UPPER: <span id='c-up'>0.0</span>%</span></div><h3>EGDB STATS</h3><div class='stat-box'><span class='stat-label'>LOADED</span><span id='e-loaded' class='stat-val'>-</span></div><div class='stat-box'><span class='stat-label'>HITS</span><span id='e-hits' class='stat-val'>0</span></div></div>`;

    sideL.innerHTML = statsHtml;
    document.body.appendChild(sideL);
    
    const togL = document.createElement("button"); togL.className = "side-toggle toggle-left"; togL.innerText = "STATS";
    togL.onclick = () => sideL.classList.toggle("open"); document.body.appendChild(togL);
    
    const sideR = document.createElement("div"); sideR.className = "sidebar sidebar-right";
    
    let html = `<h3>GAME SETTINGS</h3><div class='cfg-row'><span class='cfg-label'>STONES (1-18)</span><input id='cfg-stones' type='number' value='4' min='1' max='18'></div><div class='cfg-row'><span class='cfg-label'>DISTRIBUTION</span><select id='cfg-dist'><option value='0'>Uniform</option><option value='1'>Random</option><option value='2'>Custom</option></select></div><div class='cfg-row' id='seed-row' style='display:none'><span class='cfg-label'>SEED (0=rand)</span><input id='cfg-seed' type='number' value='0'></div><div class='cfg-row'><span class='cfg-label'>MODE</span><select id='cfg-mode'><option value='0'>Classic</option><option value='1'>Avalanche</option></select></div><div class='cfg-row'><span class='cfg-label'>AI TIME LIMIT (s, 0=inf)</span><input id='cfg-time' type='number' value='1' min='0'></div><div class='cfg-row'><span class='cfg-label'>STARTING PLAYER</span><select id='cfg-start'><option value='1'>Player</option><option value='-1'>AI</option></select></div>`;

    html += `<div class='expert-only'><div class='cfg-row'><span class='cfg-label'>CACHE SIZE EXP (18-28)</span><input id='cfg-cache' type='number' value='24' min='18' max='28'></div><div class='cfg-row'><button id='cfg-egdb' class='cfg-btn' style='background:#05a; color:#fff;'>LOAD EGDB 18</button></div><div class='chk-row'><span class='cfg-label'>AI AUTOPLAY</span><input id='cfg-autoplay' type='checkbox' checked></div></div>`;

    html += `<button id='cfg-go' class='cfg-btn'>START GAME</button>`;
    html += `<button id='cfg-reset' class='cfg-btn-reset disabled'>RESET</button>`;
    
    sideR.innerHTML = html;
    document.body.appendChild(sideR);
    
    document.getElementById('cfg-dist').onchange = (e) => {
        document.getElementById('seed-row').style.display = e.target.value == '1' ? 'block' : 'none';
        if (window.inSetup) {
            if (window.customEditMode) window.exitCustomEdit();
            if (e.target.value == '2') window.enterCustomEdit(null);
        }
        window.updateButtons();
    };
    const togR = document.createElement("button"); togR.className = "side-toggle toggle-right"; togR.innerText = "SETTINGS";
    togR.onclick = () => sideR.classList.toggle("open"); document.body.appendChild(togR);
    
    const apChk = document.getElementById("cfg-autoplay");
    if(apChk) apChk.onchange = (e) => { Module._set_autoplay(e.target.checked ? 1 : 0); };

    window.clampInput = function(id, min, max) {
        const el = document.getElementById(id);
        if (!el) return;
        let v = parseInt(el.value);
        if (isNaN(v)) return;
        if (v < min) { el.value = min; }
        if (v > max) { el.value = max; }
    };

    document.getElementById("cfg-go").onclick = () => {
        if (!window.egdbReady || Module._get_ai_thinking()) return;

        const cfg = window.readSettings();
        const isCustom = (cfg.dist === 2);
        if (isCustom && !window.inSetup) return;

        const cacheInput = document.getElementById("cfg-cache");
        if (cacheInput) {
            clampInput("cfg-cache", 18, 28);
            Module._set_web_cache_size(parseInt(cacheInput.value));
        }
        const apInput = document.getElementById("cfg-autoplay");
        if (apInput) Module._set_autoplay(apInput.checked ? 1 : 0);

        if (isCustom) {
            const cells = window.readCustomBoard();
            window.customEditMode = false;
            window.domCache = {};
            Module._restart_game(cfg.stones, 0, cfg.moveFunc, cfg.timeVal, cfg.startColor, 0);
            Module._set_custom_board(cells[0], cells[1], cells[2], cells[3], cells[4], cells[5], cells[6],
                                     cells[7], cells[8], cells[9], cells[10], cells[11], cells[12], cells[13],
                                     cfg.startColor);
            Module._finalize_board();
        } else if (cfg.dist === 1) {
            if (window.customEditMode) { window.customEditMode = false; window.domCache = {}; }
            const useSeed = window.inSetup ? window.lastUsedSeed : cfg.seed;
            Module._restart_game(cfg.stones, 1, cfg.moveFunc, cfg.timeVal, cfg.startColor, useSeed);
        } else {
            if (window.customEditMode) { window.customEditMode = false; window.domCache = {}; }
            Module._restart_game(cfg.stones, 0, cfg.moveFunc, cfg.timeVal, cfg.startColor, 0);
        }

        window.lastUsedSeed = Module._get_last_seed();
        window.inSetup = false;
        window.lastHistoryCount = -1;
        window.updateButtons();
        updateView();
        document.querySelector(".sidebar-right").classList.remove("open");

        if (!Module._get_game_over() && Module._get_current_player() === -1 && Module._get_autoplay()) {
            Module._do_ai_step();
        }
    };

    document.getElementById("cfg-reset").onclick = () => {
        if (!window.egdbReady || window.inSetup || Module._get_ai_thinking()) return;

        Module._clear_highlights();
        window.clearBoardHighlights();
        window.lastHistoryCount = -1;

        const cfg = window.readSettings();

        if (cfg.dist === 2) {
            window.inSetup = true;
            window.enterCustomEdit(null);
        } else if (cfg.dist === 1) {
            window.inSetup = true;
            Module._restart_game(cfg.stones, 1, cfg.moveFunc, cfg.timeVal, cfg.startColor, window.lastUsedSeed);
        } else {
            Module._restart_game(cfg.stones, 0, cfg.moveFunc, cfg.timeVal, cfg.startColor, 0);
        }

        window.updateButtons();
        window.updateView();
    };
    
    window.downloadEGDB = async function(minStones, maxStones, filename, btnEl) {
        if (btnEl) window.egdbReady = false;
        if (btnEl) btnEl.classList.add("disabled");
        window.updateView();

        try {
            let stEl = window.getEl("status-text");
            if (stEl) stEl.textContent = ">> LOADING...";
            if (btnEl) btnEl.innerText = "LOADING...";
            
            const res = await fetch(filename);
            if (!res.ok) throw new Error("Fetch failed");
            const buf = await res.arrayBuffer();
            
            await new Promise(r => setTimeout(r, 10)); 
            
            const u8 = new Uint8Array(buf);
            const ptr = Module._web_malloc(u8.length);
            HEAPU8.set(u8, ptr);
            
            const success = Module._load_egdb_to_vfs(ptr, u8.length, minStones, maxStones);
            Module._web_free(ptr);
            
            if (success) {
                window.egdbReady = true;
                if (btnEl) {
                    btnEl.innerText = "EGDB " + maxStones + " READY";
                    btnEl.style.background = "#444";
                    btnEl.style.color = "#888";
                }
            } else {
                if (stEl) stEl.textContent = ">> ERROR";
                if (btnEl) btnEl.innerText = "ERROR";
            }
        } catch(e) {
            console.error(e);
            let stEl = window.getEl("status-text");
            if (stEl) stEl.textContent = ">> ERROR";
            if (btnEl) {
                btnEl.innerText = "FAILED";
                btnEl.classList.remove("disabled");
            }
        }
        window.updateView();
    };

    document.getElementById("cfg-egdb").onclick = function() {
        if (this.classList.contains("disabled")) return;
        window.downloadEGDB(11, 18, 'egdb11-18.zst', this);
    };
    
    const main = document.createElement("div"); main.className = "main-content"; document.body.appendChild(main);
    const bRel = document.createElement("div"); bRel.className = "board-container-relative"; main.appendChild(bRel);
    const bCont = document.createElement("div"); bRel.appendChild(bCont); window.createBoardDOM(bCont, "main-", false);
    
    const uBtn = document.createElement("button"); uBtn.id = "undo-btn"; uBtn.className = "bottom-btn undo-pos"; uBtn.innerText = "undo move"; uBtn.onclick = () => Module._undo_move(); bRel.appendChild(uBtn);

    const stepBtn = document.createElement("button"); stepBtn.id = "step-btn"; stepBtn.className = "bottom-btn step-pos expert-only"; stepBtn.innerText = "step AI >"; 
    stepBtn.style.display = "none";
    stepBtn.onclick = () => Module._do_ai_step();
    bRel.appendChild(stepBtn);

    const sTxt = document.createElement("div"); sTxt.id = "status-text"; sTxt.style.marginTop = "25px"; sTxt.style.marginBottom = "15px"; sTxt.style.fontSize = "18px"; main.appendChild(sTxt);
    const hBtn = document.createElement("button"); hBtn.id = "hist-btn"; hBtn.className = "bottom-btn"; hBtn.style.marginTop = "0px"; hBtn.innerText = "▼ history ▼";
    
    hBtn.onclick = () => { 
        const hc = window.getEl("history-container"); 
        if (hc.style.display !== "flex") { 
            hc.style.display = "flex"; hBtn.innerText = "▲ history ▲"; window.syncHistory(); 
        } else { 
            hc.style.display = "none"; hBtn.innerText = "▼ history ▼"; 
        } 
    };
    main.appendChild(hBtn);

    const hCont = document.createElement("div"); hCont.id = "history-container"; hCont.className = "history-container"; main.appendChild(hCont);
window.toggleExpert = function() {
        const isExpert = document.body.classList.toggle("expert-mode");
        if (!isExpert) {
            const cInput = document.getElementById("cfg-cache");
            if (cInput) { cInput.value = "24"; Module._set_web_cache_size(24); }
            const aInput = document.getElementById("cfg-autoplay");
            if (aInput) { aInput.checked = true; Module._set_autoplay(1); }
        } else {
            window.updateView();
        }
    };
    
    window.openAbout = function() {
        const modal = window.getEl('about-modal');
        if (modal) modal.style.display = 'flex';
    };
    window.closeAbout = function(e) {
        if (e) e.stopPropagation();
        const modal = window.getEl('about-modal');
        if (modal) modal.style.display = 'none';
    };

    let modalHtml = "<div id='about-modal' class='modal-overlay' style='display:none;' onclick='window.closeAbout(event)'>";
    modalHtml += "<div class='modal-content' onclick='event.stopPropagation()'>";
    modalHtml += "<h2>About CMancala</h2>";
    modalHtml += "<p><strong>Version:</strong> " + vStr + "</p>";
    modalHtml += "<p><strong>Source Code:</strong> <a href='https://github.com/LifesLight/CMancala' target='_blank'>github.com/LifesLight/CMancala</a></p>";
    modalHtml += "<p><strong>Copyright:</strong> &copy; Alexander Kurtz</p>";
    modalHtml += "<p><strong>License:</strong> MIT License</p>";
    
    modalHtml += "<hr style='border-color:#444; margin:15px 0;'>";
    modalHtml += "<h3>Acknowledgments & Third-Party Code</h3>";
    
    modalHtml += "<p>EGDB generation algorithm inspired by <a href='https://github.com/girving/kalah' target='_blank'>girving/kalah</a>:</p>";
    modalHtml += "<div class='license-box'>";
    modalHtml += "Copyright 2009, Geoffrey Irving.<br>All rights reserved.<br><br>";
    modalHtml += "Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:<br><br>";
    modalHtml += "1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.<br>";
    modalHtml += "2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.<br>";
    modalHtml += "3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.<br><br>";
    modalHtml += "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";
    modalHtml += "</div>";

    modalHtml += "<p style='margin-top:15px'>This software uses <strong>Zstandard (zstd)</strong>.</p>";
    modalHtml += "<div class='license-box'>";
    modalHtml += "BSD License<br>For Zstandard software<br><br>";
    modalHtml += "Copyright (c) Meta Platforms, Inc. and affiliates. All rights reserved.<br><br>";
    modalHtml += "Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:<br><br>";
    modalHtml += "* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.<br>";
    modalHtml += "* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.<br>";
    modalHtml += "* Neither the name Facebook, nor Meta, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.<br><br>";
    modalHtml += "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS &quot;AS IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";
    modalHtml += "</div>";

    modalHtml += "<button onclick='window.closeAbout(event)' class='modal-close-btn'>Close</button>";
    modalHtml += "</div></div>";

    document.body.insertAdjacentHTML('beforeend', modalHtml);
    document.body.insertAdjacentHTML('beforeend', "<div class='footer'><button class='bottom-btn' onclick='window.openAbout()'>About CMancala</button><button class='expert-btn' onclick='window.toggleExpert()'>EXP</button></div>");
    
    window.syncHistory = function() {
        const hc = window.getEl("history-container"); 
        if (!hc || hc.style.display !== "flex") return;
        
        const count = Module._get_history_count(); 
        if (window.lastHistoryCount === count) return;
        window.lastHistoryCount = count;
        
        hc.innerHTML = "";
        for (let t = count - 2; t >= 0; t--) {
            const item = document.createElement("div"); item.className = "hist-item";
            const bC = document.createElement("div"); window.createBoardDOM(bC, "h" + t + "-", true); item.appendChild(bC); hc.appendChild(item);
            
            const mv = Module._get_history_move(t); 
            const cp = Module._get_history_captures(t);
            const md = Module._get_history_modified(t);
            
            for (let i = 0; i < 14; i++) {
                const el = document.getElementById("h" + t + "-" + i);
                if (el) { 
                    el.textContent = Module._get_history_stone_count(t, i); 
                    el.className = (i > 6 && i < 13) ? "pit ai-pit" : (i <= 5 ? "pit player-pit" : "store");
                    
                    if (cp & (1 << i)) el.classList.add("highlight-capture");
                    else if (i === mv) el.classList.add("highlight-move");
                    else if (md & (1 << i)) el.classList.add("highlight-modified");
                }
            }
        }
        if (count <= 1) hc.innerHTML = "<div style='text-align:center; color:#666; padding: 15px;'>No past moves to show</div>";
    };

    window.updateView = function() {
        if (window.customEditMode) {
            const stEl = window.getEl("status-text");
            if (stEl && stEl.textContent !== ">> CUSTOM EDIT") stEl.textContent = ">> CUSTOM EDIT";
            window.updateButtons();
            return;
        }

        const thinking = Module._get_ai_thinking(); 
        const gameOver = Module._get_game_over();
        const currentPlayer = Module._get_current_player();
        const autoplay = Module._get_autoplay();

        const fmt = (n) => n >= 1e6 ? (n/1e6).toFixed(2)+"M" : (n >= 1e3 ? (n/1e3).toFixed(1)+"k" : String(n));

        const mBoard = window.getEl("main-board");
        const isDis = !!(thinking || gameOver || window.inSetup || !window.egdbReady);
        if (mBoard.classList.contains("disabled") !== isDis) mBoard.classList.toggle("disabled", isDis);
        const isCheat = !!Module._get_is_cheated();
        if (mBoard.classList.contains("cheated") !== isCheat) mBoard.classList.toggle("cheated", isCheat);
        
        const undoBtn = window.getEl("undo-btn");
        const cannotUndo = !Module._can_undo() || window.inSetup;
        if (undoBtn.classList.contains("disabled") !== cannotUndo) undoBtn.classList.toggle("disabled", cannotUndo);
        
        const showStep = (!autoplay && currentPlayer === -1 && !thinking && !gameOver && !window.inSetup);
        const sBtn = window.getEl("step-btn");
        if(sBtn) {
            const disp = showStep ? "block" : "none";
            if (sBtn.style.display !== disp) sBtn.style.display = disp;
        }

        const lastMove = window.inSetup ? -1 : Module._get_last_move();
        const captures = window.inSetup ? 0 : Module._get_current_captures();
        const modified = window.inSetup ? 0 : Module._get_current_modified();

        const hoveredPit = window.hoveredPit !== undefined ? window.hoveredPit : -1;
        
        let drops = 0;
        let lastDrop = -1;
        
        if (hoveredPit >= 0 && !thinking && !gameOver && !window.inSetup && currentPlayer === 1) {
            let stones = Module._get_stone_count(hoveredPit);
            if (stones > 0) {
                let curr = hoveredPit;
                while (stones > 0) {
                    curr = (curr + 1) % 14;
                    if (curr === 13) continue;
                    drops |= (1 << curr);
                    stones--;
                }
                lastDrop = curr;
            }
        }

        for (let i = 0; i < 14; i++) {
            const el = window.getEl("main-" + i); if (!el) continue;
            
            const stStr = String(Module._get_stone_count(i));
            if (el.textContent !== stStr) el.textContent = stStr;
            
            let newClass = "";
            if (lastDrop !== -1) {
                if (i === lastDrop) newClass = "highlight-last-drop";
                else if (drops & (1 << i)) newClass = "highlight-modified";
            } else {
                if (captures & (1 << i)) newClass = "highlight-capture";
                else if (i === lastMove) newClass = "highlight-move";
                else if (modified & (1 << i)) newClass = "highlight-modified";
            }
            
            const currClass = el.getAttribute("data-hl") || "";
            if (currClass !== newClass) {
                if (currClass) el.classList.remove(currClass);
                if (newClass) el.classList.add(newClass);
                el.setAttribute("data-hl", newClass);
            }
        }
        
        const setTxt = (id, txt) => {
            const el = window.getEl(id);
            if (el && el.textContent !== txt) el.textContent = txt;
        };

        setTxt("s-status", Module._get_stat_solved() ? "SOLVED" : "DEPTH: " + Module._get_stat_depth());
        let ev = Module._get_stat_eval(); 
        setTxt("s-eval", (ev === -9999) ? "N/A" : (ev > 0 ? "+" + ev : String(ev)));
        
        setTxt("s-nodes", fmt(Module._get_stat_nodes()));
        setTxt("s-total-nodes", fmt(Module._get_stat_total_nodes()));
        
        let ti = Module._get_stat_time(); 
        setTxt("s-tput", (ti > 0 ? ((Module._get_stat_nodes() / ti) / 1e6).toFixed(2) : "0.00") + " M n/s");
        setTxt("s-time", ti.toFixed(3) + " s");
        
        if (document.body.classList.contains("expert-mode")) {
            Module._update_web_cache_stats();
            setTxt("c-fill", Module._get_c_fill().toFixed(2) + "%");
            setTxt("c-solved", Module._get_c_has_depth() ? (Module._get_c_solved().toFixed(2) + "%") : " - %");
            setTxt("c-hits", fmt(Module._get_c_hits()));
            setTxt("c-swap", Module._get_c_swap().toFixed(2) + "%");
            setTxt("c-imp", fmt(Module._get_c_imp()));
            setTxt("c-evi", fmt(Module._get_c_evi()));
            setTxt("c-ex", Module._get_c_exact().toFixed(1));
            setTxt("c-lo", Module._get_c_lower().toFixed(1));
            setTxt("c-up", Module._get_c_upper().toFixed(1));

            const eMax = Module._get_egdb_max_stones();
            setTxt("e-loaded", eMax > 0 ? (eMax + " Stones (" + Module._get_egdb_size_mb().toFixed(1) + "MB)") : "None");
            setTxt("e-hits", fmt(Module._get_egdb_hits()));
        }

        const stEl = window.getEl("status-text");
        if (window.egdbReady) {
            let stStr = "";
            if (window.inSetup) stStr = ">> PRESS START";
            else if (gameOver) stStr = ">> GAME OVER"; 
            else if (thinking) stStr = ">> AI THINKING..."; 
            else if (currentPlayer === -1 && !autoplay) stStr = ">> WAITING FOR STEP";
            else stStr = (currentPlayer === 1) ? ">> YOUR TURN" : ">> AI TURN";
            if (stEl && stEl.textContent !== stStr) stEl.textContent = stStr;
        }
        
        window.updateButtons();
        window.syncHistory();
    };
    
    window.lastUsedSeed = Module._get_last_seed();
    window.downloadEGDB(1, 10, 'egdb10.zst', null);
});
#endif

void startInterface() {
    init_game_with_config(4, 0, 0, 1.0, 1, 0);
#ifdef __EMSCRIPTEN__
    launch_gui(MANCALA_VERSION);
#endif
}

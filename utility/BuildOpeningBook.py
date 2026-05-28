import subprocess
import re
import sys
import os

# --- CONFIGURATION ---
MANCALA_EXEC = "./build/Mancala"
OUTPUT_FILE = "opening_book.txt"
STONES_LIST = [3, 4, 5]
EGDB = 32
CACHE = 30
MAX_AI_DEPTH = 3
# ---------------------

class MancalaEngine:
    def __init__(self, executable_path):
        self.proc = subprocess.Popen(
            [executable_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        self.buf = ""

    def read_until_prompt(self):
        """Reads character by character until it hits one of the engine prompts."""
        while True:
            char = self.proc.stdout.read(1)
            if not char:
                print("\n[!] Engine crashed or closed unexpectedly.")
                sys.exit(1)
            
            self.buf += char
            
            if self.buf.endswith("(C) << "):
                res = self.buf
                self.buf = ""
                return "(C)", res
            elif self.buf.endswith("(G) << "):
                res = self.buf
                self.buf = ""
                return "(G)", res
            elif self.buf.endswith("(P) << "):
                res = self.buf
                self.buf = ""
                return "(P)", res

    def send(self, cmd):
        """Sends a command to the engine."""
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

def load_existing_book(filename):
    """Loads existing codes so we don't duplicate lines in the output file."""
    book = {}
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) >= 3:
                    book[parts[0]] = (parts[1], parts[2])
    return book

def extract_code(text):
    m = re.search(r'Code\s+"([^"]+)"', text)
    return m.group(1) if m else None

def extract_metadata(text):
    move, eval_score = None, None
    m_move = re.search(r'Move:\s+(\d+)', text)
    m_eval = re.search(r'Evaluation:\s+(-?\d+)', text)
    if m_move: move = m_move.group(1)
    if m_eval: eval_score = m_eval.group(1)
    return move, eval_score

def explore(code, ai_depth, engine, book, file_handle, explored_depths):
    """Recursively explores the game tree up to a certain AI move depth."""
    if ai_depth >= MAX_AI_DEPTH:
        return
        
    if code in explored_depths and explored_depths[code] <= ai_depth:
        return
    explored_depths[code] = ai_depth
    
    # 1. Load the state
    engine.send(f"load {code}")
    engine.read_until_prompt()
    
    # 2. Step the engine to see what happens
    engine.send("step")
    prompt, out = engine.read_until_prompt()
    
    if prompt == "(P)":
        # HUMAN's
        engine.send("menu")
        engine.read_until_prompt()
        
        for move_idx in range(1, 7):
            engine.send(f"load {code}")
            engine.read_until_prompt()
            
            engine.send("step")
            p, out = engine.read_until_prompt()
            
            if p != "(P)":
                continue
                
            engine.send(str(move_idx))
            p, out = engine.read_until_prompt()
            
            # Verify if the move was actually valid
            if "Move:" not in out:
                if p == "(P)":
                    engine.send("menu")
                    engine.read_until_prompt()
                continue # Skip invalid human moves
                
            if p == "(P)":
                engine.send("menu")
                engine.read_until_prompt()
                engine.send("encode")
                p_enc, out_enc = engine.read_until_prompt()
                child_code = extract_code(out_enc)
                if child_code and child_code != code:
                    explore(child_code, ai_depth, engine, book, file_handle, explored_depths)
                    
            elif p == "(G)":
                engine.send("encode")
                p_enc, out_enc = engine.read_until_prompt()
                child_code = extract_code(out_enc)
                if child_code and child_code != code:
                    explore(child_code, ai_depth, engine, book, file_handle, explored_depths)
                    
    elif prompt == "(G)":
        # AI's turn
        engine.send("last")
        p, out = engine.read_until_prompt()
        move, eval_score = extract_metadata(out)
        
        if move is not None and eval_score is not None:
            # Save if not in book yet
            if code not in book:
                entry = f"{code} {move} {eval_score}"
                print(f" [Depth {ai_depth}] Evaluated & Saved: {entry}")
                file_handle.write(entry + "\n")
                file_handle.flush()
                book[code] = (move, eval_score)
            else:
                print(f" [Depth {ai_depth}] Skipped DB Write (Already tracked): {code}")
                
            engine.send("encode")
            p_enc, out_enc = engine.read_until_prompt()
            child_code = extract_code(out_enc)
            
            if child_code:
                explore(child_code, ai_depth + 1, engine, book, file_handle, explored_depths)


def generate_book():
    print(">> Initializing Engine & Book...")
    book = load_existing_book(OUTPUT_FILE)
    explored_depths = {}
    
    engine = MancalaEngine(MANCALA_EXEC)
    engine.read_until_prompt()
    
    with open(OUTPUT_FILE, "a") as file_handle:
        for stones in STONES_LIST:
            print(f"\n=========================================")
            print(f">> Generating Tree for {stones} Stones (Max Depth: {MAX_AI_DEPTH})")
            print(f"=========================================")
            
            # --- Scenario 1: AI Starts (Player 1) ---
            print(f"\n>> Configuring: AI starts (P1)")
            config_scen_1 = [
                "config", "autoplay false", "time 0", 
                "player 1 ai", "player 2 human", "starting 1",
                f"egdb {EGDB}", f"cache {CACHE}", f"stones {stones}", "start"
            ]
            for cmd in config_scen_1:
                engine.send(cmd)
                engine.read_until_prompt()
                
            engine.send("encode")
            p, out = engine.read_until_prompt()
            code1 = extract_code(out)
            explore(code1, 0, engine, book, file_handle, explored_depths)
            
            # --- Scenario 2: Human Starts (Player 2) ---
            print(f"\n>> Configuring: Human starts (P2)")
            config_scen_2 = [
                "config", "autoplay false", "time 0", 
                "player 1 ai", "player 2 human", "starting 2",
                f"egdb {EGDB}", f"cache {CACHE}", f"stones {stones}", "start"
            ]
            for cmd in config_scen_2:
                engine.send(cmd)
                engine.read_until_prompt()
                
            engine.send("encode")
            p, out = engine.read_until_prompt()
            code2 = extract_code(out)
            explore(code2, 0, engine, book, file_handle, explored_depths)
        
    engine.send("quit")
    print("\n>> Generation finished!")

if __name__ == "__main__":
    generate_book()
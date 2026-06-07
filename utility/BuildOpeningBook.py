import subprocess
import os
import sys

# --- CONFIGURATION ---
MANCALA_EXEC = "./build/Mancala"
OUTPUT_FILE = "opening_book.txt"
STONES_LIST = [3, 4, 5]
EGDB = 36
CACHE = 32
MAX_AI_DEPTH = 4

STORE_OPTIMAL_LINE = True
# ---------------------

class MancalaAPI:
    def __init__(self, executable_path, egdb, cache):
        cmd = [
            executable_path, "--api", 
            "--egdb", str(egdb), 
            "--cache", str(cache)
        ]
        self.proc = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )

    def send(self, cmd):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def receive(self):
        while True:
            line = self.proc.stdout.readline()
            if not line:
                print("\n[!] API crashed or closed unexpectedly.")
                sys.exit(1)
            line = line.strip()

            if line.startswith("OK") or line.startswith("ERROR"):
                return line

    def get_root(self, stones):
        self.send(f"ROOT {stones}")
        res = self.receive()
        if res.startswith("OK CODE"):
            return res.split()[2]
        return None

    def solve(self, code):
        self.send(f"SOLVE {code}")
        res = self.receive()
        if res.startswith("OK EVAL"):
            parts = res.split()
            eval_score = int(parts[2])
            move = int(parts[4])
            next_code = parts[6]
            return eval_score, move, next_code
        return None, None, None

    def get_moves(self, code):
        self.send(f"MOVES {code}")
        res = self.receive()
        if res.startswith("OK"):
            parts = res.split()[1:]
            moves = []
            for i in range(0, len(parts), 4):
                if parts[i] == "MOVE" and parts[i+2] == "NEXT_CODE":
                    moves.append((int(parts[i+1]), parts[i+3]))
            return moves
        return []

    def close(self):
        self.send("QUIT")
        self.proc.wait()

def load_existing_book(filename):
    book = set()
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) >= 3:
                    book.add(parts[0])
    return book

def get_ui_move(internal_move):
    if internal_move < 6:
        return internal_move + 1
    else:
        return internal_move - 6

def generate_book():
    book_codes = load_existing_book(OUTPUT_FILE)
    explored_depths = {}

    print(">> Initializing Engine (EGDB and Cache)...")
    api = MancalaAPI(MANCALA_EXEC, EGDB, CACHE)

    with open(OUTPUT_FILE, "a") as file_handle:
        for stones in STONES_LIST:
            print(f"\n=========================================")
            print(f">> Processing {stones} Stones")

            root_code = api.get_root(stones)
            if not root_code:
                print("Failed to get root code")
                continue

            if STORE_OPTIMAL_LINE:
                print(f"\n>> Generating Optimal PV Line for {stones} Stones")
                code = root_code
                depth = 0
                while True:
                    eval_score, move, next_code = api.solve(code)
                    if move is None or move == -1:
                        break

                    if code not in book_codes:
                        ui_move = get_ui_move(move)
                        entry = f"{code} {ui_move} {eval_score}"
                        print(f" [Opt-Line {depth}] Saved: {entry}")
                        file_handle.write(entry + "\n")
                        file_handle.flush()
                        book_codes.add(code)

                    code = next_code
                    depth += 1

            print(f"\n>> Generating Tree for {stones} Stones (Max Depth: {MAX_AI_DEPTH})")

            def explore(code, current_depth):
                if current_depth >= MAX_AI_DEPTH:
                    return

                if code in explored_depths and explored_depths[code] <= current_depth:
                    return
                explored_depths[code] = current_depth

                eval_score, best_move, _ = api.solve(code)
                if best_move is None or best_move == -1:
                    return

                if code not in book_codes:
                    ui_move = get_ui_move(best_move)
                    entry = f"{code} {ui_move} {eval_score}"
                    print(f" [Depth {current_depth}] Saved: {entry}")
                    file_handle.write(entry + "\n")
                    file_handle.flush()
                    book_codes.add(code)

                moves = api.get_moves(code)
                for move, next_code in moves:
                    explore(next_code, current_depth + 1)

            explore(root_code, 0)

    api.close()
    print("\n>> Generation finished!")

if __name__ == "__main__":
    generate_book()
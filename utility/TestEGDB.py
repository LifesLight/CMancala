import subprocess
import re
import random
import sys
import os

# --- CONFIGURATION ---
EXECUTABLE_PATH = "./build_pgo/Mancala"
ITERATIONS = 1000
BASE_DEPTH = 0
DEPTH_VARIATION = 0
LOG_FILE = "egdb_mismatch_log.txt"
EGDB_SIZE = 24
# ---------------------

def get_random_depth():
    return random.randint(BASE_DEPTH - DEPTH_VARIATION, BASE_DEPTH + DEPTH_VARIATION)

def get_random_seed():
    return random.randint(0, 2**31 - 1)

def parse_scores(output_str):
    """
    Returns: (egdb_scores, no_egdb_scores)
    """
    board_matches = re.findall(r"\(G\) >> └───┤(.*?)(?:├───┘)", output_str)
    parsed_results = []

    for match in board_matches:
        scores = []
        parts = match.split('│')
        for p in parts:
            p = p.strip()
            if p: 
                try:
                    scores.append(int(p))
                except ValueError:
                    pass 
        parsed_results.append(scores)

    if len(parsed_results) < 2:
        return None, None

    return parsed_results[0], parsed_results[1]

def run_test(iteration):
    depth = get_random_depth()
    seed = get_random_seed()

    input_commands = (
        f"autoplay false\n"
        f"stones 3\n"
        f"distribution random\n"
        f"seed {seed}\n"
        f"cache 24\n"
        f"start\n"
        f"analyze --solver local --depth {depth}\n"
        f"cache 23"
        f"egdb {EGDB_SIZE}\n"
        f"analyze --solver local --depth {depth}\n"
        f"quit\n"
    )

    try:
        process = subprocess.run(
            EXECUTABLE_PATH,
            input=input_commands,
            capture_output=True,
            text=True,
            shell=False 
        )

        output = process.stdout
        egdb_res, normal_res = parse_scores(output)

        if egdb_res is None or normal_res is None:
            print(f"[{iteration}] Error: Parsing failed. Seed: {seed}")
            return

        if egdb_res == normal_res:
            print(f"[{iteration}/{ITERATIONS}] Match! (Seed: {seed}, Depth: {depth})")
        else:
            print(f"[{iteration}/{ITERATIONS}] MISMATCH! EGDB vs Normal.")
            log_mismatch(seed, depth, egdb_res, normal_res, output)

    except FileNotFoundError:
        print(f"Error: Executable not found at {EXECUTABLE_PATH}")
        sys.exit(1)

def log_mismatch(seed, depth, egdb_res, normal_res, full_output):
    with open(LOG_FILE, "a") as f:
        f.write("="*60 + "\n")
        f.write(f"EGDB VERIFICATION MISMATCH\n")
        f.write(f"Seed:  {seed} | Depth: {depth}\n")
        f.write(f"Local + EGDB: {egdb_res}\n")
        f.write(f"Local Normal: {normal_res}\n")
        f.write("-" * 60 + "\n")
        f.write(full_output) 
        f.write("\n" + "="*60 + "\n\n")

def main():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    print(f"Verifying EGDB vs Normal Search (Iter: {ITERATIONS}, EGDB: {EGDB_SIZE} stones)")

    for i in range(1, ITERATIONS + 1):
        run_test(i)

    print("\nTesting complete.")

if __name__ == "__main__":
    main()
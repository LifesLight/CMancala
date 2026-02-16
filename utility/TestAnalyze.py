import subprocess
import re
import random
import sys
import os

# --- CONFIGURATION ---
EXECUTABLE_PATH = "./build-pgo/Mancala"
ITERATIONS = 100
BASE_DEPTH = 0
DEPTH_VARIATION = 0
LOG_FILE = "mismatch_log.txt"
# ---------------------

def get_random_depth():
    return random.randint(BASE_DEPTH - DEPTH_VARIATION, BASE_DEPTH + DEPTH_VARIATION)

def get_random_seed():
    return random.randint(0, 2**31 - 1)

def parse_scores(output_str):
    """
    Parses the stdout from the Mancala program to find the scores.
    Returns: (global_scores, local_scores)
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

    global_scores = parsed_results[0]
    local_scores = parsed_results[1]

    return global_scores, local_scores

def run_test(iteration):
    depth = get_random_depth()
    seed = get_random_seed()

    input_commands = (
        f"autoplay false\n"
        f"stones 2\n"
        f"distribution random\n"
        f"seed {seed}\n"
        f"cache 24\n"
        f"compress true\n"
        f"start\n"
        f"analyze --solver global --depth {depth}\n"
        f"analyze --solver local --depth {depth}\n"
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

        # Parse the output
        global_res, local_res = parse_scores(output)

        if global_res is None or local_res is None:
            print(f"[{iteration}] Could not parse output tables. (Seed: {seed})")
            return

        # Compare results
        if global_res == local_res:
            print(f"[{iteration}/{ITERATIONS}] Match (Seed: {seed}, Depth: {depth})")
        else:
            print(f"[{iteration}/{ITERATIONS}] MISMATCH detected! Logging to file...")
            log_mismatch(seed, depth, global_res, local_res, output)

    except FileNotFoundError:
        print(f"Error: Could not find executable at {EXECUTABLE_PATH}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")

def log_mismatch(seed, depth, global_res, local_res, full_output):
    with open(LOG_FILE, "a") as f:
        f.write("="*40 + "\n")
        f.write(f"MISMATCH DETECTED\n")
        f.write(f"Seed:  {seed}\n")
        f.write(f"Depth: {depth}\n")
        f.write(f"Global Solver: {global_res}\n")
        f.write(f"Local Solver:  {local_res}\n")
        f.write("-" * 20 + " Full Output Snippet " + "-" * 20 + "\n")
        lines = full_output.splitlines()
        f.write("\n".join(lines[-30:])) 
        f.write("\n" + "="*40 + "\n\n")

def main():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    print(f"Starting test suite for {EXECUTABLE_PATH}")
    print(f"Running {ITERATIONS} iterations with Depth {BASE_DEPTH} +/- {DEPTH_VARIATION}")

    for i in range(1, ITERATIONS + 1):
        run_test(i)

    print("\nTesting complete.")

if __name__ == "__main__":
    main()
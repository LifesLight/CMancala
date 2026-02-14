import subprocess
import re
import random
import sys
import os

# --- CONFIGURATION ---
EXECUTABLE_PATH = "./build-pgo/Mancala"
ITERATIONS = 250
SOLVER_TYPE = "global"
STONES = 2
CACHE_SIZE = 21
LOG_FILE = "clip_mismatch.log"
# ---------------------

def get_random_seed():
    return random.randint(0, 2**31 - 1)

def parse_scores(output_str):
    """
    Parses the stdout from the Mancala program to find the scores.
    Expects two analyzes: First Unclipped, Second Clipped.
    """
    # Regex to capture the bottom row of the board which contains the scores
    # Matches: (G) >> └───┤  3│  3│...├───┘
    board_matches = re.findall(r"\(G\) >> └───┤(.*?)(?:├───┘)", output_str)

    parsed_results = []

    for match in board_matches:
        scores = []
        # Split by the vertical bar char
        parts = match.split('│')
        for p in parts:
            p = p.strip()
            if p: 
                try:
                    scores.append(int(p))
                except ValueError:
                    pass 
        parsed_results.append(scores)

    # We expect at least 2 results (Unclipped first, Clipped second)
    # Note: 'render' command might output a board too, so we take the last two
    if len(parsed_results) < 2:
        return None, None

    unclipped_scores = parsed_results[-2]
    clipped_scores = parsed_results[-1]

    return unclipped_scores, clipped_scores

def check_consistency(unclipped, clipped):
    """
    Returns True if consistent, False otherwise.
    Rule: 
    - If Unclipped > 0 (Win), Clipped MUST be 1.
    - If Unclipped <= 0 (Draw/Loss), Clipped MUST NOT be 1.
    """
    if len(unclipped) != len(clipped):
        return False, "Length Mismatch"

    for i, (u_val, c_val) in enumerate(zip(unclipped, clipped)):
        if u_val > 0:
            # Expecting a WIN in clipped mode (which is clamped to 1)
            if c_val != 1:
                return False, f"Index {i}: Unclipped was {u_val} (Win), but Clipped was {c_val}"
        else:
            # Expecting NOT A WIN in clipped mode
            if c_val == 1:
                return False, f"Index {i}: Unclipped was {u_val} (Non-Win), but Clipped was {c_val} (False Positive)"

    return True, "OK"

def run_test(iteration):
    seed = get_random_seed()

    # Commands: Setup -> Analyze Unclipped -> Analyze Clipped
    input_commands = (
        f"autoplay false\n"
        f"cache {CACHE_SIZE}\n"
        f"stones {STONES}\n"
        f"seed {seed}\n"
        f"start\n"
        f"analyze --solver {SOLVER_TYPE} --depth 0 --clip false\n"
        f"analyze --solver {SOLVER_TYPE} --depth 0 --clip true\n"
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

        # Parse
        unclipped, clipped = parse_scores(output)

        if unclipped is None or clipped is None:
            print(f"[{iteration}] Could not parse output. (Seed: {seed})")
            return

        # Check logic
        is_valid, msg = check_consistency(unclipped, clipped)

        if is_valid:
            print(f"[{iteration}/{ITERATIONS}] Match (Seed: {seed})")
        else:
            print(f"[{iteration}/{ITERATIONS}] MISMATCH: {msg}")
            log_mismatch(seed, unclipped, clipped, msg, output)

    except FileNotFoundError:
        print(f"Error: Could not find executable at {EXECUTABLE_PATH}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")

def log_mismatch(seed, unclipped, clipped, msg, full_output):
    with open(LOG_FILE, "a") as f:
        f.write("="*40 + "\n")
        f.write(f"MISMATCH DETECTED\n")
        f.write(f"Seed:   {seed}\n")
        f.write(f"Solver: {SOLVER_TYPE}\n")
        f.write(f"Reason: {msg}\n")
        f.write(f"Unclipped Scores: {unclipped}\n")
        f.write(f"Clipped Scores:   {clipped}\n")
        f.write("-" * 20 + " Last Output " + "-" * 20 + "\n")
        lines = full_output.splitlines()
        # Log last 40 lines to catch the boards
        f.write("\n".join(lines[-40:])) 
        f.write("\n" + "="*40 + "\n\n")

def main():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    print(f"Starting Clip-Validation Suite for {EXECUTABLE_PATH}")
    print(f"Solver: {SOLVER_TYPE} | Stones: {STONES} | Cache: 2^{CACHE_SIZE}")
    print(f"Running {ITERATIONS} iterations...")

    for i in range(1, ITERATIONS + 1):
        run_test(i)

    print("\nTesting complete.")

if __name__ == "__main__":
    main()

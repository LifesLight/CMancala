import subprocess
import re
import random
import sys
import os

# --- CONFIGURATION ---
EXECUTABLE_PATH = "./build-pgo/Mancala"
ITERATIONS = 100
LOG_FILE = "mismatch_step_log.txt"
# ---------------------

def get_random_seed():
    return random.randint(0, 2**31 - 1)

def parse_results(output_str):
    """
    Parses the stdout to find Move and Evaluation from the 'last' command metadata.
    We look for the Global output first, then the Local output.

    Returns: A list of dicts [{'move': int, 'eval': int}, ...]
    """
    moves = re.findall(r"\(G\) >>\s+Move:\s+(-?\d+)", output_str)

    evals = re.findall(r"\(G\) >>\s+Evaluation:\s+(-?\d+)", output_str)

    results = []

    count = min(len(moves), len(evals))

    for i in range(count):
        results.append({
            'move': int(moves[i]),
            'eval': int(evals[i])
        })

    return results

def run_test(iteration):
    seed = get_random_seed()

    input_commands = (
        f"autoplay false\n"
        f"stones 2\n"
        f"time 0\n"
        f"depth 100\n"
        f"cache 17\n"
        f"compress true\n"
        f"distribution random\n"
        f"seed {seed}\n"
        f"player 1 ai\n"
        f"solver global\n"
        f"start\n"
        f"step\n"
        f"last\n"
        f"config\n"
        f"solver local\n"
        f"start\n"
        f"step\n"
        f"last\n"
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
        results = parse_results(output)

        if len(results) < 2:
            print(f"[{iteration}] Parsing Error: Expected 2 results, got {len(results)}. (Seed: {seed})")
            return

        global_res = results[0]
        local_res = results[1]

        move_match = (global_res['move'] == local_res['move'])
        eval_match = (global_res['eval'] == local_res['eval'])

        if move_match and eval_match:
            print(f"[{iteration}/{ITERATIONS}] Match (Seed: {seed}) | Move: {global_res['move']} | Eval: {global_res['eval']}")
        else:
            print(f"[{iteration}/{ITERATIONS}] MISMATCH detected! Logging...")
            log_mismatch(seed, global_res, local_res, output)

    except FileNotFoundError:
        print(f"Error: Could not find executable at {EXECUTABLE_PATH}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")

def log_mismatch(seed, global_res, local_res, full_output):
    with open(LOG_FILE, "a") as f:
        f.write("="*50 + "\n")
        f.write(f"MISMATCH DETECTED (Seed: {seed})\n")
        f.write(f"{'Type':<10} | {'Move':<10} | {'Evaluation':<10}\n")
        f.write("-" * 40 + "\n")
        f.write(f"{'Global':<10} | {global_res['move']:<10} | {global_res['eval']:<10}\n")
        f.write(f"{'Local':<10} | {local_res['move']:<10} | {local_res['eval']:<10}\n")
        f.write("-" * 50 + "\n")
        f.write("Full Output Context (Last 50 lines):\n")
        lines = full_output.splitlines()
        f.write("\n".join(lines[-50:])) 
        f.write("\n" + "="*50 + "\n\n")

def main():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    print(f"Starting BEST MOVE/EVAL test suite for {EXECUTABLE_PATH}")
    print(f"Running {ITERATIONS} iterations...")

    for i in range(1, ITERATIONS + 1):
        run_test(i)

    print("\nTesting complete.")

if __name__ == "__main__":
    main()
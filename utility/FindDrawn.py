import subprocess
import re
import random
import sys
import os

# --- CONFIGURATION ---
EXECUTABLE_PATH = "./build-pgo/Mancala"
STONES = 3
ITERATIONS = 10000
# ---------------------

def get_random_seed():
    return random.randint(0, 2**31 - 1)

def parse_results(output_str):
    """
    Parses the stdout to find the Evaluation from 'last' 
    and the board state code from 'encode'.
    """
    # Find the evaluation score
    evals = re.findall(r"\(G\) >>\s+Evaluation:\s+(-?\d+)", output_str)

    # Find the encoded board state (capturing whatever is inside the quotes)
    codes = re.findall(r"\(G\) >>\s+Code\s+\"([^\"]+)\"", output_str)

    # Use the last occurrence in case the output has multiple
    evaluation = int(evals[-1]) if evals else None
    code = codes[-1] if codes else None

    return evaluation, code

def run_tests():
    output_file = f"{STONES}_drawn"
    draws_found = 0

    print(f"Starting brute force for drawn positions (Eval 0) with {STONES} stones.")
    print(f"Executable: {EXECUTABLE_PATH}")
    print(f"Running {ITERATIONS} iterations...")
    print(f"Output file: {output_file}\n")

    for iteration in range(1, ITERATIONS + 1):
        seed = get_random_seed()

        # Batched inputs: configures engine, makes a random step, gets the eval, then encodes the board
        input_commands = (
            f"autoplay false\n"
            f"stones {STONES}\n"
            f"time 0\n"
            f"depth 0\n"
            f"cache 24\n"
            f"distribution random\n"
            f"seed {seed}\n"
            f"player 1 ai\n"
            f"solver local\n"
            f"start\n"
            f"step\n"
            f"last\n"
            f"encode\n"
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
            evaluation, code = parse_results(output)

            # Error handling for parsing
            if evaluation is None:
                print(f"[{iteration}/{ITERATIONS}] Parsing Error: Could not find Evaluation. (Seed: {seed})")
                continue

            # Check if we hit an even position
            if evaluation == 0:
                draws_found += 1
                print(f"[{iteration}/{ITERATIONS}] DRAW FOUND! (Seed: {seed}) | Code: {code}")

                if code:
                    with open(output_file, "a") as f:
                        f.write(f"{code}\n")
                else:
                    print(f"[{iteration}/{ITERATIONS}] Error: Eval was 0 but no code was found.")
            else:
                # To prevent console spam, only print progress occasionally if no draw is found
                if iteration % 100 == 0:
                    print(f"[{iteration}/{ITERATIONS}] No draw. Eval: {evaluation}. Searching...")

        except FileNotFoundError:   
            print(f"Error: Could not find executable at {EXECUTABLE_PATH}")
            sys.exit(1)
        except Exception as e:
            print(f"An error occurred: {e}")
            sys.exit(1)

    print(f"\nFinished {ITERATIONS} iterations. Found a total of {draws_found} drawn positions.")

if __name__ == "__main__":
    run_tests()

import subprocess
import sys
import os
from pathlib import Path

# --- CONFIG ---
EXECUTABLE_PATH = "./build-pgo/Mancala"
STORE_DIR = Path("./sweep")
# -------------

def run(n, stones):
    STORE_DIR.mkdir(parents=True, exist_ok=True)
    store_path = STORE_DIR / f"{stones}solve_local@{n}"

    input_commands = (
        "autoplay false\n"
        "player 1 ai\n"
        "time 0\n"
        "depth 0\n"
        f"stones {stones}\n"
        "solver local\n"
        f"cache {n}\n"
        "start\n"
        "step\n"
        f"store {store_path}\n"
    )

    try:
        subprocess.run(
            EXECUTABLE_PATH,
            input=input_commands,
            text=True,
            shell=False,
            check=True
        )
    except FileNotFoundError:
        print(f"binary not found: {EXECUTABLE_PATH}")
        sys.exit(1)

def main():
    if len(sys.argv) != 4:
        print("usage: python run.py <stones> <start> <end>")
        sys.exit(1)

    stones = int(sys.argv[1])

    start = int(sys.argv[2])
    end = int(sys.argv[3])

    for n in range(start, end + 1):
        print(f"running cache/store {n}")
        run(n, stones)

if __name__ == "__main__":
    main()

import os
import sys

# --- CONFIGURATION ---
INPUT_TXT = "opening_book.txt"
OUTPUT_HEADER = "include/logic/solver/opening_book.h"
# ---------------------

def build_cpp_header(txt_filename, header_filename):
    print(f"Reading '{txt_filename}'...")
    if not os.path.exists(txt_filename):
        print(f"Error: '{txt_filename}' not found.")
        return

    book_entries = {}
    with open(txt_filename, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 3:
                code_str = parts[0]
                ui_move = int(parts[1])
                eval_score = int(parts[2])

                padded = "00000" + code_str
                high = int(padded[:16], 16)
                low = int(padded[16:], 16)

                is_p1 = (low & 1) == 1
                if not is_p1:
                    continue

                internal_move = ui_move - 1
                book_entries[(high, low)] = (internal_move, eval_score)

    # Sort keys lexicographically by (high, low) for Binary Search
    sorted_keys = sorted(book_entries.keys())

    out_dir = os.path.dirname(header_filename)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

    print(f"Writing '{header_filename}' with {len(sorted_keys)} canonical entries...")
    with open(header_filename, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n#include <stdbool.h>\n#include \"logic/board.h\"\n\n")
        f.write("typedef struct {\n    uint64_t high;\n    uint64_t low;\n")
        f.write("    int8_t bestMove;\n    int16_t eval;\n} OpeningBookEntry;\n\n")

        f.write("static const OpeningBookEntry OPENING_BOOK[] = {\n")
        for k in sorted_keys:
            move, score = book_entries[k]
            f.write(f"    {{ 0x{k[0]:016x}ULL, 0x{k[1]:016x}ULL, {move}, {score} }},\n")
        f.write("};\n\n")

        f.write("static const int OPENING_BOOK_SIZE = sizeof(OPENING_BOOK) / sizeof(OpeningBookEntry);\n\n")

        f.write("""static inline bool probeOpeningBook(const Board* board, int* bestMove, int* eval) {
    if (OPENING_BOOK_SIZE == 0) return false;

    uint64_t low = 0;
    uint64_t high = 0;

    bool mirrored = (board->color == -1);

    low |= 1ULL;

    int shift = 1;
    for (int i = 0; i < 14; i++) {
        int idx = mirrored ? (i + 7) % 14 : i;

        if (shift < 64) {
            low |= ((uint64_t)(board->cells[idx] & 0xFF)) << shift;
        } else {
            high |= ((uint64_t)(board->cells[idx] & 0xFF)) << (shift - 64);
        }
        shift += 8;
    }

    int L = 0;
    int R = OPENING_BOOK_SIZE - 1;
    while (L <= R) {
        int m = L + (R - L) / 2;
        if (OPENING_BOOK[m].high == high && OPENING_BOOK[m].low == low) {
            *bestMove = mirrored ? (OPENING_BOOK[m].bestMove + 7) : OPENING_BOOK[m].bestMove;
            *eval = OPENING_BOOK[m].eval;
            return true;
        }
        if (OPENING_BOOK[m].high > high || (OPENING_BOOK[m].high == high && OPENING_BOOK[m].low > low)) {
            R = m - 1;
        } else {
            L = m + 1;
        }
    }
    return false;
}
""")
    print("Compilation complete")

if __name__ == "__main__":
    build_cpp_header(INPUT_TXT, OUTPUT_HEADER)
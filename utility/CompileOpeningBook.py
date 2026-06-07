import os

# --- CONFIGURATION ---
INPUT_TXT = "opening_book.txt"
OUTPUT_DATA_H = "include/logic/solver/opening_book_data.h"
# ---------------------

def build_lut(txt_filename, out_h):
    print(f"Reading '{txt_filename}'...")
    if not os.path.exists(txt_filename):
        print(f"Error: '{txt_filename}' not found.")
        return

    book_entries = {}
    metadata_lines = []

    with open(txt_filename, 'r') as f:
        for line in f:
            if line.startswith("#"):
                metadata_lines.append(line.strip())
                continue

            parts = line.strip().split()
            if len(parts) >= 3:
                code_str = parts[0]
                ui_move = int(parts[1])
                eval_score = int(parts[2])

                if len(code_str) <= 16:
                    high = 0
                    low = int(code_str, 16)
                else:
                    high = int(code_str[:-16], 16)
                    low = int(code_str[-16:], 16)

                cells = [0] * 14
                color_bit = low & 1

                shift = 1
                for i in range(14):
                    if shift < 64:
                        cells[i] = (low >> shift) & 0xFF
                    else:
                        cells[i] = (high >> (shift - 64)) & 0xFF
                    shift += 8

                a0 = 0 if color_bit == 1 else 7
                b0 = 7 if color_bit == 1 else 0

                m = (cells[a0] | cells[a0+1] | cells[a0+2] | cells[a0+3] | cells[a0+4] | cells[a0+5] |
                     cells[b0] | cells[b0+1] | cells[b0+2] | cells[b0+3] | cells[b0+4] | cells[b0+5])
                if m > 15: 
                    continue

                # Check score pit budget (6 bits -> max 63)
                if cells[a0+6] > 63 or cells[b0+6] > 63:
                    continue

                h = (cells[a0] |
                     (cells[a0+1] << 4) |
                     (cells[a0+2] << 8) |
                     (cells[a0+3] << 12) |
                     (cells[a0+4] << 16) |
                     (cells[a0+5] << 20) |
                     (cells[b0] << 24) |
                     (cells[b0+1] << 28) |
                     (cells[b0+2] << 32) |
                     (cells[b0+3] << 36) |
                     (cells[b0+4] << 40) |
                     (cells[b0+5] << 44) |
                     (cells[a0+6] << 48) |
                     (cells[b0+6] << 54))

                internal_move = ui_move - 1
                book_entries[h] = (internal_move, eval_score)

    sorted_keys = sorted(book_entries.keys())

    out_dir = os.path.dirname(out_h)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

    print(f"Writing '{out_h}' with {len(sorted_keys)} entries...")

    with open(out_h, 'w') as f:
        f.write("#pragma once\n\n#include <stdint.h>\n\n")

        f.write("/*\n")
        f.write(" * Automatically Generated Opening Book\n")

        if metadata_lines:
            f.write(" *\n * Dataset Generation Settings:\n")
            for m_line in metadata_lines:
                clean_line = m_line.lstrip("#").strip()
                f.write(f" *   {clean_line}\n")

        f.write(" */\n\n")

        def chunk_list(lst, n):
            for i in range(0, len(lst), n):
                yield lst[i:i + n]

        f.write("static const uint64_t BOOK_HASHES[] = {\n")
        for chunk in chunk_list(sorted_keys, 6):
            f.write("    " + ", ".join(f"0x{k:016x}ULL" for k in chunk) + ",\n")
        f.write("};\n\n")

        f.write("static const uint8_t BOOK_MOVES[] = {\n")
        for chunk in chunk_list(sorted_keys, 16):
            f.write("    " + ", ".join(f"{book_entries[k][0]}" for k in chunk) + ",\n")
        f.write("};\n\n")

        f.write("static const int8_t BOOK_EVALS[] = {\n")
        for chunk in chunk_list(sorted_keys, 16):
            f.write("    " + ", ".join(f"{book_entries[k][1]}" for k in chunk) + ",\n")
        f.write("};\n\n")

        f.write("static const int BOOK_SIZE = sizeof(BOOK_HASHES) / sizeof(uint64_t);\n")

    print("Compilation complete.")

if __name__ == "__main__":
    build_lut(INPUT_TXT, OUTPUT_DATA_H)
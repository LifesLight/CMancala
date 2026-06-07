/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/opening_book.h"
#include "logic/solver/opening_book_data.h"

bool probeOpeningBook(const Board* board, int* bestMove, int* eval) {
    if (BOOK_SIZE == 0) return false;

    int a0 = (board->color == 1) ? 0 : 7;
    int b0 = (board->color == 1) ? 7 : 0;

    uint8_t m = board->cells[a0] | board->cells[a0+1] | board->cells[a0+2] | board->cells[a0+3] | board->cells[a0+4] | board->cells[a0+5] |
                board->cells[b0] | board->cells[b0+1] | board->cells[b0+2] | board->cells[b0+3] | board->cells[b0+4] | board->cells[b0+5];

    if (m > 15) return false;
    if (board->cells[a0+6] > 63 || board->cells[b0+6] > 63) return false;

    uint64_t hash = ((uint64_t)board->cells[a0])           |
                    (((uint64_t)board->cells[a0+1]) << 4)  |
                    (((uint64_t)board->cells[a0+2]) << 8)  |
                    (((uint64_t)board->cells[a0+3]) << 12) |
                    (((uint64_t)board->cells[a0+4]) << 16) |
                    (((uint64_t)board->cells[a0+5]) << 20) |
                    (((uint64_t)board->cells[b0])   << 24) |
                    (((uint64_t)board->cells[b0+1]) << 28) |
                    (((uint64_t)board->cells[b0+2]) << 32) |
                    (((uint64_t)board->cells[b0+3]) << 36) |
                    (((uint64_t)board->cells[b0+4]) << 40) |
                    (((uint64_t)board->cells[b0+5]) << 44) |
                    (((uint64_t)board->cells[a0+6]) << 48) |
                    (((uint64_t)board->cells[b0+6]) << 54);

    int L = 0;
    int R = BOOK_SIZE - 1;
    while (L <= R) {
        int m_idx = L + (R - L) / 2;
        if (BOOK_HASHES[m_idx] == hash) {
            *bestMove = (board->color == 1) ? BOOK_MOVES[m_idx] : (BOOK_MOVES[m_idx] + 7);
            *eval = BOOK_EVALS[m_idx];
            return true;
        }
        if (BOOK_HASHES[m_idx] > hash) {
            R = m_idx - 1;
        } else {
            L = m_idx + 1;
        }
    }
    return false;
}
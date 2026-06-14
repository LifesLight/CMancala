#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "logic/board.h"
#include "logic/solver/opening_book_data.h"

bool probeOpeningBook(const Board *board, int *bestMove, int *eval);

/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solveCache.h"

typedef struct {
#ifdef GUARANTEE_VALIDATION
    uint64_t key;
#endif

    VALIDATION_TYPE validation;

#ifdef CACHE_GUARD
    uint8_t protector;
#endif

    int8_t value;
    int8_t boundType;
    uint8_t depth;
} Entry;

Entry* cache;

uint32_t cacheSize = 0;
uint64_t overwrites;
uint64_t lastOverwrites;
uint64_t invalidReads;
uint64_t lastInvalidReads;
uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t lastHits;
uint64_t lastHitsLegalDepth;

#ifdef GUARANTEE_VALIDATION
uint64_t detectedInvalidations;
uint64_t lastDetectedInvalidations;
#endif


#ifdef CACHE_GUARD
/**
 * A entry can be overwritten if it has "lifetime 0", which is indicated by the first 7 bits being 0.
 * The last bit is used to indicate if the entry was accessed this iteration,
 * this doesn't affect the protection status this iteration.
*/
const uint8_t freshLifetime = ((uint16_t)CACHE_GUARD + 1) & 0xFE;

// Protect the entry for the next iteration
void protectIndex(uint32_t index) {
    cache[index].protector |= 0x80;
}

void removeProtection(uint32_t index) {
    cache[index].protector &= 0x7F;
}

// Check if the entry was protected this iteration
bool isProtected(uint32_t index) {
    return (cache[index].protector & 0x80) != 0;
}

// Get how many iterations the entry is still protected
uint8_t getLifetime(uint32_t index) {
    return cache[index].protector & 0x7F; // Mask out the protected bit
}

// Set the lifetime
void setLifetime(uint8_t lifetime, uint32_t index) {
    cache[index].protector = (cache[index].protector & 0x80) | (lifetime & 0x7F);
}

uint32_t countPerAge[CACHE_GUARD + 1];
#endif

/**
 * Compute the validation hash
*/
VALIDATION_TYPE computeValidation(uint64_t hash) {
#if VALIDATION_SIZE == 64
    return hash;
#else
    return hash % VALMAX;
#endif
}

uint32_t getCacheSize() {
    return cacheSize;
}

/**
 * Returns false if cant be translated
*/
bool translateBoard(Board* board, uint64_t *code) {
    // Check if window can be stored
    uint64_t hash = 0;

    // Make first bit the current player
    hash |= ((uint64_t)((board->color == 1) ? 1 : 0));

    int offset = 1;
    uint8_t value;

    // 5 bits per cell
    for (int i = 0; i < 6; i++) {
        value = board->cells[i];
        if (value > 31) {
            return false;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    for (int i = 7; i < 13; i++) {
        value = board->cells[i];
        if (value > 31) {
            return false;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    *code = hash;
    return true;
}

// Hash function to get into the array index
uint32_t indexHash(uint64_t hash) {
    return hash % cacheSize;
}

void startCache(uint32_t size) {
    cacheSize = size;
    free(cache);
    cache = malloc(sizeof(Entry) * cacheSize);

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        cache[i].value = UNSET_VALUE;
        cache[i].validation = 0;
        cache[i].boundType = 0;
        cache[i].depth = 0;


#ifdef GUARANTEE_VALIDATION
        cache[i].key = 0;
#endif

#ifdef CACHE_GUARD
        cache[i].protector = 0;
#endif
    }

#ifdef CACHE_GUARD
    lastOverwrites = 0;
    lastInvalidReads = 0;
#endif

#ifdef GUARANTEE_VALIDATION
    detectedInvalidations = 0;
    lastDetectedInvalidations = 0;
#endif

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
    hitsLegalDepth = 0;

    lastOverwrites = 0;
    lastInvalidReads = 0;
    lastHits = 0;
    lastHitsLegalDepth = 0;
}

void cacheNode(Board* board, int evaluation, int boundType, int depth) {
    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > INT8_MAX || evaluation < INT8_MIN + 1) {
        return;
    }

    // Translate the board to a unique hash value
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return;
    }

    // Compute primary index and validation
    const uint32_t index = indexHash(hashValue);
    const VALIDATION_TYPE validation = computeValidation(hashValue);

#ifdef CACHE_GUARD
    // Check if the entry is protected
    if (getLifetime(index) > 0) {
        return;
    }
#endif

    // Check if the entry already exists
    if (cache[index].validation == validation) {
#ifdef GUARANTEE_VALIDATION
        if (cache[index].key != hashValue) {
            detectedInvalidations++;
        }
#endif

        // No reason to store a lower depth cache
        if (cache[index].depth > depth) {
            return;
        }
    }

    // Handle overwrites
    if (cache[index].value != UNSET_VALUE) {
        overwrites++;
    }

    // Update cache entry
    cache[index].boundType = boundType;
    cache[index].validation = validation;
    cache[index].value = evaluation;
    cache[index].depth = depth;

#ifdef GUARANTEE_VALIDATION
    cache[index].key = hashValue;
#endif

#ifdef CACHE_GUARD
    cache[index].protector = 0;
#endif

    return;
}

bool getCachedValue(Board* board, int currentDepth, int *eval, int *boundType) {
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return false;
    }

    // Get the primary hash
    const int index = indexHash(hashValue);
    const VALIDATION_TYPE validation = computeValidation(hashValue);

    // Check if the entry is unset
    if (cache[index].value == UNSET_VALUE) {
        return false;
    }

    // Check if the key matches
    if (cache[index].validation != validation) {
        invalidReads++;
        return false;
    }

#ifdef GUARANTEE_VALIDATION
    if (cache[index].key != hashValue) {
        detectedInvalidations++;
        return false;
    }
#endif

    // We have a hit
    hits++;


    // Our cached entry needs to be at least as deep as our current depth.
    if (cache[index].depth < currentDepth) {
        return false;
    }

    hitsLegalDepth++;

#ifdef CACHE_GUARD
    // Protect the entry
    protectIndex(index);
#endif

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    *eval = cache[index].value + scoreDelta;
    *boundType = cache[index].boundType;

    return true;
}

/**
 * Render the cache stats
*/
typedef struct {
    int start;
    int size;
    int type;
} Chunk;

int compareChunksSize(const void *a, const void *b) {
    return ((Chunk*)b)->size - ((Chunk*)a)->size;
}

int compareChunksStart(const void *a, const void *b) {
    return ((Chunk*)a)->start - ((Chunk*)b)->start;
}

void renderCacheStats() {
    char message[100];

    uint64_t setEntries = 0;

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        if (cache[i].value != UNSET_VALUE) {
            setEntries++;
        }
    }

    double fillPercentage = (double)setEntries / (double)cacheSize;
    sprintf(message, "  Cache size: %d (%.2f", cacheSize, fillPercentage * 100);
    strcat(message, "%)");
    renderOutput(message, CHEAT_PREFIX);

    sprintf(message, "  Overwrites: %"PRIu64"", lastOverwrites);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Collisions: %"PRIu64"", lastInvalidReads);
    renderOutput(message, CHEAT_PREFIX);

#ifdef GUARANTEE_VALIDATION
    sprintf(message, "  Detected:   %"PRIu64"", lastDetectedInvalidations);
    renderOutput(message, CHEAT_PREFIX);
#endif

    sprintf(message, "  Hits (Bad): %"PRIu64" (%"PRIu64")", lastHitsLegalDepth, lastHits - lastHitsLegalDepth);
    renderOutput(message, CHEAT_PREFIX);

#ifdef CACHE_GUARD
    renderOutput("  Guard durations", CHEAT_PREFIX);
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);
    for (int i = 0; i <= CACHE_GUARD; i++) {
        sprintf(message, "  %d: %d", i, countPerAge[i]);
        renderOutput(message, CHEAT_PREFIX);
    }
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);
#endif

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index | Chunk Size", CHEAT_PREFIX);
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    // Store all chunks
    Chunk* chunks = malloc(sizeof(Chunk) * cacheSize);
    int chunkCount = 0;
    int currentType = cache[0].value != UNSET_VALUE;
    int chunkStart = 0;
    int chunkSize = 1;

    // Iterate through the cache to identify chunks
    for (int i = 1; i < (int64_t)cacheSize; i++) {
        if ((cache[i].value != UNSET_VALUE) == currentType) { 
            chunkSize++;
        } else {
            chunks[chunkCount].type = currentType;
            chunks[chunkCount].start = chunkStart;
            chunks[chunkCount].size = chunkSize;
            chunkCount++;

            currentType = cache[i].value != UNSET_VALUE;
            chunkStart = i;
            chunkSize = 1;
        }
    }

    // Add the last chunk
    chunks[chunkCount].type = currentType;
    chunks[chunkCount].start = chunkStart;
    chunks[chunkCount].size = chunkSize;
    chunkCount++;

    // Sort the chunks by size in descending order
    qsort(chunks, chunkCount, sizeof(Chunk), compareChunksSize);

    // Allocate memory for top chunks
    Chunk* topChunks = malloc(sizeof(Chunk) * min(chunkCount, OUTPUT_CHUNK_COUNT));

    // Store top chunks separately
    for (int i = 0; i < min(chunkCount, OUTPUT_CHUNK_COUNT); i++) {
        topChunks[i] = chunks[i];
    }

    // Sort top chunks by start index in ascending order
    qsort(topChunks, min(chunkCount, OUTPUT_CHUNK_COUNT), sizeof(Chunk), compareChunksStart);

    // Print the largest OUTPUT_CHUNK_COUNT chunks
    for (int i = 0; i < min(chunkCount, OUTPUT_CHUNK_COUNT); i++) {
        sprintf(message, "     %s   | %10d  | %10d", topChunks[i].type ? "Set  " : "Unset", topChunks[i].start, topChunks[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }

    if (chunkCount > OUTPUT_CHUNK_COUNT) {
        renderOutput("  ...", CHEAT_PREFIX);
    }

    renderOutput("  ---------------------------------------", CHEAT_PREFIX);


    free(chunks);
    free(topChunks);
}


void stepCache() {
    lastOverwrites = overwrites;
    lastInvalidReads = invalidReads;
    lastHits = hits;
    lastHitsLegalDepth = hitsLegalDepth;

#ifdef GUARANTEE_VALIDATION
    lastDetectedInvalidations = detectedInvalidations;
    detectedInvalidations = 0;
#endif

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
    hitsLegalDepth = 0;

#ifdef CACHE_GUARD
    // Track how many have what age
    memset(countPerAge, 0, sizeof(countPerAge));

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        if (cache[i].value == UNSET_VALUE) {
            continue;
        }

        if (isProtected(i)) {
            uint8_t remaining = getLifetime(i);
            if (remaining > 0) {
                remaining--;
                setLifetime(remaining, i);
            }
        } else {
            setLifetime(CACHE_GUARD, i);
        }

        countPerAge[getLifetime(i)]++;

        removeProtection(i);
    }
#endif
}

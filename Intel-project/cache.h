#pragma once

/**
 * @file cache.h
 * @brief definitions associated to a a two-level hierarchy of cache memories
 */

#include "addr.h" // for word_t
#include <stdint.h>

#define L1_ICACHE_WORDS_PER_LINE 4
#define L1_ICACHE_LINE   16u // 16 bytes (4 words) per line
#define L1_ICACHE_WAYS   4u
#define L1_ICACHE_LINES  64u  // Do not modify this!
#define L1_ICACHE_TAG_REMAINING_BITS   10 // 2(select byte) + 2(select word) + 6(select line)
#define L1_ICACHE_TAG_BITS             22 // 32 - L1_ICACHE_TAG_REMAINING_BITS

#define L1_DCACHE_WORDS_PER_LINE L1_ICACHE_WORDS_PER_LINE
#define L1_DCACHE_LINE   L1_ICACHE_LINE
#define L1_DCACHE_WAYS   L1_ICACHE_WAYS
#define L1_DCACHE_LINES  L1_ICACHE_LINES
#define L1_DCACHE_TAG_REMAINING_BITS L1_ICACHE_TAG_REMAINING_BITS
#define L1_DCACHE_TAG_BITS           L1_ICACHE_TAG_BITS

#define L2_CACHE_WORDS_PER_LINE L1_ICACHE_WORDS_PER_LINE
#define L2_CACHE_LINE   L1_ICACHE_LINE
#define L2_CACHE_WAYS   8u
#define L2_CACHE_LINES  512u  // Do not modify this!
#define L2_CACHE_TAG_REMAINING_BITS   13 // 2(select byte) + 2(select word) + 9(select line)
#define L2_CACHE_TAG_BITS             19 // 32 - L1_ICACHE_TAG_REMAINING_BITS

/**
 * L1 ICACHE, L1 DCACHE:
 *  - byte addressing
 *  - physically addressed
 *  - 4-way set-associative
 *  - 4 words/way, where word = 4 bytes (=> 128 bits/way)
 *  - 64 sets (= 64 blocks per way) (= 6 bits to index)
 *  - total capacity = 4kiB
 *  - write-through policy (no dirty bit)
 *  - write-allocate on write miss
 *
 * L2 CACHE:
 *  - byte addressing
 *  - physically addressed
 *  - 8-way set-associative
 *  - 4 words/way, where word = 4 bytes (=> 128 bits/way)
 *  - 512 sets (= 512 blocks per way) (= 9 bits to index)
 *  - total capacity = 64kiB
 *  - write-through policy (no dirty bit)
 *  - write-allocate on write miss
 *
 *  Exclusive policy (https://en.wikipedia.org/wiki/Cache_inclusion_policy)
 *      Consider the case when L2 is exclusive of L1. Suppose there is a
 *      processor read request for block X. If the block is found in L1 cache,
 *      then the data is read from L1 cache and returned to the processor. If
 *      the block is not found in the L1 cache, but present in the L2 cache,
 *      then the cache block is moved from the L2 cache to the L1 cache. If
 *      this causes a block to be evicted from L1, the evicted block is then
 *      placed into L2. This is the only way L2 gets populated. Here, L2
 *      behaves like a victim cache. If the block is not found neither in L1 nor
 *      in L2, then it is fetched from main memory and placed just in L1 and not
 *      in L2.
 *
 */

typedef struct l1_icache_entry {
    uint8_t v : 1;
    uint8_t age : 2;
    uint32_t tag : L1_ICACHE_TAG_BITS;
    word_t line[L1_ICACHE_WORDS_PER_LINE];
} l1_icache_entry_t;

/////////////////////// ALIAS???????????????????????????????
typedef l1_icache_entry_t l1_dcache_entry_t;


typedef struct l2_cache_entry {
    uint8_t v : 1;
    uint8_t age : 3;
    uint32_t tag : L2_CACHE_TAG_BITS;
    word_t line[L2_CACHE_WORDS_PER_LINE];
} l2_cache_entry_t;

typedef enum {L1_ICACHE, L1_DCACHE, L2_CACHE} cache_t;



// --------------------------------------------------
#define cache_cast(TYPE) ((TYPE *)cache)

// --------------------------------------------------
#define cache_entry(TYPE, WAYS, LINE_INDEX, WAY) \
        (cache_cast(TYPE) + (LINE_INDEX) * (WAYS) + (WAY))

// --------------------------------------------------
#define cache_valid(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->v

// --------------------------------------------------
#define cache_age(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->age

// --------------------------------------------------
#define cache_tag(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->tag

// --------------------------------------------------
#define cache_line(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->line

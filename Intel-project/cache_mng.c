/**
 * @file cache_mng.c
 * @brief cache management functions
 */

#include "error.h"
#include "util.h"
#include "cache.h"
#include "cache_mng.h"
#include "addr.h"
#include <stdint.h>
#include <stdlib.h>
#include "lru.h"


#include <inttypes.h> // for PRIx macros

//=========================================================================
#define PRINT_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: %1" PRIx8 ", TAG: 0x%03" PRIx16 ", values: ( ", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_age(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_tag(TYPE, WAYS, LINE_INDEX, WAY)); \
            for(int i_ = 0; i_ < WORDS_PER_LINE; i_++) \
                fprintf(OUTFILE, "0x%08" PRIx32 " ", \
                        cache_line(TYPE, WAYS, LINE_INDEX, WAY)[i_]); \
            fputs(")\n", OUTFILE); \
    } while(0)

#define PRINT_INVALID_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: -, TAG: -----, values: ( ---------- ---------- ---------- ---------- )\n", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY)); \
    } while(0)

#define DUMP_CACHE_TYPE(OUTFILE, TYPE, WAYS, LINES, WORDS_PER_LINE)  \
    do { \
        for(uint16_t index = 0; index < LINES; index++) { \
            foreach_way(way, WAYS) { \
                fprintf(output, "%02" PRIx8 "/%04" PRIx16 ": ", way, index); \
                if(cache_valid(TYPE, WAYS, index, way)) \
                    PRINT_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE); \
                else \
                    PRINT_INVALID_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE);\
            } \
        } \
    } while(0)

//=========================================================================
// see cache_mng.h
int cache_dump(FILE* output, const void* cache, cache_t cache_type)
{
    M_REQUIRE_NON_NULL(output);
    M_REQUIRE_NON_NULL(cache);

    fputs("WAY/LINE: V: AGE: TAG: WORDS\n", output);
    switch (cache_type) {
    case L1_ICACHE:
        DUMP_CACHE_TYPE(output, l1_icache_entry_t, L1_ICACHE_WAYS,
                        L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
        break;
    case L1_DCACHE:
        DUMP_CACHE_TYPE(output, l1_dcache_entry_t, L1_DCACHE_WAYS,
                        L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
        break;
    case L2_CACHE:
        DUMP_CACHE_TYPE(output, l2_cache_entry_t, L2_CACHE_WAYS,
                        L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
        break;
    default:
        debug_print("%d: unknown cache type", cache_type);
        return ERR_BAD_PARAMETER;
    }
    putc('\n', output);

    return ERR_NONE;
}


// DECLARATION OF AUXILIARY FUNCTIONS
uint32_t phy_to_uint32(phy_addr_t paddr);
int transfer_to_l1(void * cache, void * entry, cache_t cache_type, uint8_t way, uint16_t index);
uint8_t LRU_way(void * cache, cache_t type, uint16_t index);
uint8_t invalid_way(void * cache, cache_t type, uint32_t line_index );


// ################################################ IMPLEMENTATION OF AUXILIARY FUNCTIONS ##################################################################

// This function transform a paddr into a uint32
uint32_t phy_to_uint32(phy_addr_t paddr){
    return (paddr.phy_page_num << PAGE_OFFSET) | paddr.page_offset;
}

// This function will update the line in l1 depending of the words find in l2
int transfer_to_l1(void * cache, void * entry, cache_t type, uint8_t way, uint16_t index) {
    switch (type) {
        case L1_ICACHE :
            for (int i = 0; i < L1_ICACHE_WORDS_PER_LINE; ++i ) {
                ((l1_icache_entry_t*)entry)->line[i] = cache_line(l2_cache_entry_t, L2_CACHE_LINES, index, way)[i];
            }

            ((l1_icache_entry_t *)entry)->v = 1;

        break;

        case L1_DCACHE:
            for (int i = 0; i < L1_DCACHE_WORDS_PER_LINE; ++i ) {
                ((l1_dcache_entry_t *)entry)->line[i] = cache_line(l2_cache_entry_t, L2_CACHE_LINES, index, way)[i];
            }

            ((l1_dcache_entry_t *)entry)->v = 1;
        break;

        default :
            return ERR_BAD_PARAMETER;
        }

    return ERR_NONE;

} 

// Function that return the way where we have to put the entry, using the correct remplacement policy
uint8_t LRU_way(void* cache, cache_t type, uint16_t line_index) {

    uint8_t age_max = 0;
    uint8_t way = 0;

    if (type == L1_ICACHE) {
        foreach_way(i, L1_ICACHE_WAYS) {
            if (cache_age(l1_icache_entry_t, L1_ICACHE_WAYS, line_index, i) >= age_max){
                way = i;
                age_max = cache_age(l1_icache_entry_t, L1_ICACHE_WAYS, line_index, i);

            }
        }
    }

    if (type == L1_DCACHE) {
        foreach_way(i, L1_DCACHE_WAYS) {
            if (cache_age(l1_dcache_entry_t, L1_DCACHE_WAYS, line_index, i) >= age_max){
                way = i;
                age_max = cache_age(l1_dcache_entry_t, L1_DCACHE_WAYS, line_index, i);
            }
        }
    }

    if (type == L2_CACHE) {
        foreach_way(i, L2_CACHE_WAYS) {
            if (cache_age(l2_cache_entry_t, L2_CACHE_WAYS, line_index, i) >= age_max) {
                way = i;
                age_max = cache_age(l2_cache_entry_t, L2_CACHE_WAYS, line_index, i);
            }
        }
    }

    return way;
}

//Function that find the way if one of them contain an invalid line
uint8_t invalid_way(void * cache, cache_t type, uint32_t line_index ){

    switch (type) {
        case L1_ICACHE :
            foreach_way(i, L1_ICACHE_WAYS){
                if (0 == cache_valid(l1_icache_entry_t, L1_DCACHE_WAYS, line_index, i)){
                    return i;
                }
            }
        break;

        case L1_DCACHE :
            foreach_way(i, L1_DCACHE_WAYS) {
                if (0 == cache_valid(l1_dcache_entry_t, L1_DCACHE_WAYS, line_index, i)){
                    return i;
                }
            }
        break;

        case L2_CACHE :
            foreach_way(i, L2_CACHE_WAYS) {
                if (0 == cache_valid(l2_cache_entry_t, L2_CACHE_WAYS, line_index, i)){
                    return i;
                }
            }
        break;

        default :
            return HIT_WAY_MISS ;
        }

    return HIT_WAY_MISS;
}



/*
Define Macro for init, flush, insert, hit and read
 */

#define BYTE_MAX (int) 255
#define BYTE_SIZE 8
#define INDEX_OFFSET 4
#define INDEX_L2_MASK 0b111111111
#define INDEX_L1_MASK 0b111111

// ######################################################## CACHE_INIT MACRO #############################################################

#define init_cache(TYPE, TAG_REMAINING_BITS, WORDS_PER_LINE) \
 TYPE* VAR = (TYPE*) cache_entry; \
    VAR->v = 1; \
    VAR->tag = phy >> TAG_REMAINING_BITS; \
    VAR->age = 0; \
    M_REQUIRE_NON_NULL(memcpy(VAR->line, mem_space + phy, WORDS_PER_LINE*sizeof(word_t)));



// ######################################################## CACHE_FLUSH MACRO #############################################################

#define flush_cache(TYPE, CACHE_LINES, CACHE_WAYS, WORDS_PER_LINE) \
TYPE* var = (TYPE*) cache;\
        /*SETTING ALL ENTRIES TO 0*/ \
        for (size_t i = 0; i < CACHE_LINES*CACHE_WAYS; ++i){ \
            var[i].age = 0; \
            var[i].v = 0; \
            var[i].tag = 0; \
            M_REQUIRE_NON_NULL(memset(((TYPE *)cache)->line, 0, WORDS_PER_LINE * sizeof(word_t)));\
             \
        }



// ######################################################## CACHE_INSERT MACRO #############################################################

#define insert_cache(TYPE, CACHE_LINES, CACHE_WAYS, WORDS_PER_LINE) \
    TYPE* var = (TYPE*) cache_line_in; \
        M_REQUIRE(cache_line_index < CACHE_LINES, ERR_BAD_PARAMETER,%s, "bad line index"); \
        M_REQUIRE(cache_way < CACHE_WAYS, ERR_BAD_PARAMETER,%s, "bad cache way"); \
        \
        /*SETTING CACHE LINE ENTRIES*/ \
        M_REQUIRE_NON_NULL(memcpy(cache_line(TYPE, CACHE_WAYS, cache_line_index, cache_way), (*var).line, WORDS_PER_LINE*sizeof(word_t))); \
        cache_age(TYPE, CACHE_WAYS, cache_line_index, cache_way) = (*var).age; \
        cache_valid(TYPE, CACHE_WAYS, cache_line_index, cache_way) = (*var).v; \
        cache_tag(TYPE, CACHE_WAYS, cache_line_index, cache_way) = (*var).tag;



// ######################################################## CACHE_HIT MACRO #############################################################

#define hit_cache(TYPE, CACHE_LINES, CACHE_WAYS, TAG_REMAINING_BITS) \
    line_index = (phy/16) % CACHE_LINES; \
    tag = phy >> TAG_REMAINING_BITS; \
    cache = (TYPE*) cache; \
    /*LOOP OVER CACHE WAYS*/ \
    foreach_way(ways, CACHE_WAYS){ \
        /*CHECK IF THE LINE IS VALID*/ \
        if (cache_valid(TYPE, CACHE_WAYS, line_index, ways) == 0){ \
            *hit_way = HIT_WAY_MISS; \
            *hit_index = HIT_INDEX_MISS; \
            return ERR_NONE; \
        } \
        /*CHECK IF NOT VALID BUT TAG CORRESPONDS*/ \
        if (cache_valid(TYPE, CACHE_WAYS, line_index, ways) && tag == cache_tag(TYPE, CACHE_WAYS, line_index, ways)){ \
            *hit_way = ways; \
            *hit_index = line_index; \
            *p_line = cache_line(TYPE, CACHE_WAYS,  *hit_index, *hit_way ); \
            LRU_age_update(TYPE, CACHE_WAYS, *hit_way, *hit_index); \
            return ERR_NONE; \
        } \
    } \
            *hit_way = HIT_WAY_MISS; \
            *hit_index = HIT_INDEX_MISS;



// ######################################################## CACHE_READ MACRO #############################################################

// Macro for cache_read, case hit in L2 and insert in L1
#define cache_read_L2(TYPE, CACHE_WAYS, CACHE_LINES) \
        \
        /*CREATE L1 ENTRY TO INSERT*/ \
        TYPE *  entry = malloc(sizeof(TYPE)); \
        M_REQUIRE_NON_NULL(entry);\
        entry->tag = tag_l1; \
        M_REQUIRE(transfer_to_l1(l2_cache, entry, cache_type , hit_way, hit_index) == 0, ERR_BAD_PARAMETER, %s,"error in transfer_to_l1"); \
        cache_valid(l2_cache_entry_t, L2_CACHE_LINES, hit_index, hit_way) = 0; \
        uint8_t way_find = invalid_way(l1_cache, cache_type, line_index_l1); \
        \
        /*CASE THERE IS AN AVAILABLE LINE (INVALID) IN CACHE*/ \
        if(way_find != HIT_WAY_MISS){ \
                M_REQUIRE(cache_insert(line_index_l1, way_find, entry, l1_cache, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert"); \
                LRU_age_increase(TYPE, CACHE_WAYS, way_find, line_index_l1); \
        } \
        \
        /*CASE THERE IS NO PLACE, REMOVE OLDEST FROM L1 TO PUT IN L2 AND INSERT NEWEST IN L1*/ \
        else {\
            \
            /*FIND THE WAY OF THE "OLDEST" (HIGHEST AGE)*/ \
            uint8_t way_delete = LRU_way(l1_cache, cache_type, line_index_l1); \
            TYPE entry_delete = *cache_entry(TYPE, CACHE_LINES, line_index_l1, way_delete); \
            \
            /*INSERT NEW ENTRY IN L1 AND UPDATE AGE*/ \
            M_REQUIRE(cache_insert(line_index_l1, way_delete, entry, l1_cache, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, %s, "error in cache_insert"); \
            LRU_age_update(TYPE, CACHE_WAYS, way_delete, line_index_l1); \
            \
            /*FIND A WAY IN L2 TO INSERT DETLETED ENTRY*/ \
            uint8_t way_in_l2 = invalid_way(l2_cache, L2_CACHE, hit_index); \
            /*UPDATE WORD TO READ*/ \
            *word = cache_line(TYPE, CACHE_WAYS, line_index_l1, way_delete)[w_select]; \
            cache = l2_cache; \
            \
            /*CASE THERE IS AVAILABLE PLACE IN L2*/ \
            if (way_in_l2 != HIT_WAY_MISS) { \
                cache_insert(hit_index, way_in_l2, &entry_delete, l2_cache, L2_CACHE ); \
                LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, way_in_l2, hit_index ); \
            } \
            /*CASE THERE IS NO PLACE IN L2, DELETE OLDEST ENTRY AND INSERT NEW ONE*/ \
            else { \
                uint8_t way_evicted = LRU_way(l2_cache, L2_CACHE, hit_index); \
                M_REQUIRE(cache_insert(hit_index, way_evicted, &entry_delete, l2_cache, L2_CACHE ) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert"); \
                LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, way_evicted, hit_index); \
            } \
        } 



// Macro for cache_read, case miss in L1 and in L2, writeback in L1
#define cache_read_memory(TYPE, CACHE_WAYS) \
        TYPE* entry = malloc(sizeof(TYPE)); \
        M_REQUIRE_NON_NULL(entry); \
        /* FETCH FROM MEMORY*/ \
        M_REQUIRE(cache_entry_init(mem_space, paddr, entry, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_entry_init"); \
        \
        /* WRITEBACK IN L1*/ \
        uint8_t way_find = invalid_way(l1_cache, cache_type, line_index_l1); \
        if (way_find == HIT_WAY_MISS) { \
             way_find = LRU_way(l1_cache, cache_type, line_index_l1); \
        } \
        \
        M_REQUIRE(cache_insert(line_index_l1, way_find, entry, l1_cache, cache_type ) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert"); \
        /*UPDATE AGE*/ \
        LRU_age_update(TYPE, CACHE_WAYS, way_find, line_index_l1); \
        \
        *word = cache_line(TYPE, CACHE_WAYS, line_index_l1, way_find)[w_select]; 




// ########################################################## FUNCTIONS ###################################################################
/*
    FUNCTION BEHAVIOUR COMMENTS IN MACROS
*/


int cache_entry_init(const void * mem_space,
                     const phy_addr_t * paddr,
                     void * cache_entry,
                     cache_t cache_type){

        M_REQUIRE_NON_NULL(cache_entry);
        M_REQUIRE_NON_NULL(mem_space);
        M_REQUIRE_NON_NULL(paddr);
                         
                         
        uint32_t phy = phy_to_uint32(*paddr);

        
        if(cache_type == L1_DCACHE){
            init_cache(l1_dcache_entry_t, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_WORDS_PER_LINE);
        } else if(cache_type == L1_ICACHE){
            init_cache(l1_icache_entry_t, L1_ICACHE_TAG_REMAINING_BITS, L1_ICACHE_WORDS_PER_LINE);
        } else if(cache_type == L2_CACHE){
            init_cache(l2_cache_entry_t, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_WORDS_PER_LINE);
        } else{
            return ERR_BAD_PARAMETER;
        }
                         

    return ERR_NONE;
}



int cache_flush(void *cache, cache_t cache_type){

    M_REQUIRE_NON_NULL(cache);

        if(cache_type == L1_DCACHE){
            flush_cache(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS, L1_DCACHE_WORDS_PER_LINE);
        } else if(cache_type == L1_ICACHE){
            flush_cache(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS, L1_ICACHE_WORDS_PER_LINE);
        } else if(cache_type == L2_CACHE){
            flush_cache(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS, L2_CACHE_WORDS_PER_LINE);
        } else{
            return ERR_BAD_PARAMETER;
        }
                         
   
    return ERR_NONE;
}



int cache_insert(uint16_t cache_line_index,
                 uint8_t cache_way,
                 const void * cache_line_in,
                 void * cache,
                 cache_t cache_type){


    M_REQUIRE_NON_NULL(cache);
    M_REQUIRE_NON_NULL(cache_line_in);

        if(cache_type == L1_DCACHE){
            insert_cache(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS, L1_DCACHE_WORDS_PER_LINE);
        } else if(cache_type == L1_ICACHE){
            insert_cache(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS, L1_ICACHE_WORDS_PER_LINE);
        } else if(cache_type == L2_CACHE){
            insert_cache(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS, L2_CACHE_WORDS_PER_LINE);
        } else{
            return ERR_BAD_PARAMETER;
        }


    return ERR_NONE;
}




int cache_hit (const void * mem_space,
               void * cache,
               phy_addr_t * paddr,
               const uint32_t ** p_line,
               uint8_t *hit_way,
               uint16_t *hit_index,
               cache_t cache_type){


    M_REQUIRE_NON_NULL(cache);
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(p_line);
    M_REQUIRE_NON_NULL(hit_index);
    M_REQUIRE_NON_NULL(hit_way);


    uint32_t phy = phy_to_uint32(*paddr);
    uint32_t line_index;
    uint32_t tag;

    if(cache_type == L1_DCACHE){
        hit_cache(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS, L1_DCACHE_TAG_REMAINING_BITS);
    } else if(cache_type == L1_ICACHE){
        hit_cache(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS, L1_ICACHE_TAG_REMAINING_BITS);
    } else if(cache_type == L2_CACHE){
        hit_cache(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS, L2_CACHE_TAG_REMAINING_BITS);
    } else {
        return ERR_BAD_PARAMETER;
    }

    return ERR_NONE;
}


int cache_read(const void * mem_space,
               phy_addr_t * paddr,
               mem_access_t access,
               void * l1_cache,
               void * l2_cache,
               uint32_t * word,
               cache_replace_t replace) {

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(word);

    uint32_t addr = phy_to_uint32(*paddr);

    //M_REQUIRE(paddr->page_offset % (sizeof(word_t) * WORDS_PER_LINE ) == 0, ERR_BAD_PARAMETER, " ");
    
    const uint32_t * p_line_inl1 = calloc(L1_ICACHE_WORDS_PER_LINE, sizeof(word_t));
    M_REQUIRE_NON_NULL(p_line_inl1);
    uint8_t w_select = (addr >> 2)& 0b11;

    uint8_t hit_way = 0 ;
    uint16_t hit_index = 0;
    cache_t cache_type = access == INSTRUCTION ? L1_ICACHE : L1_DCACHE;

    void * cache = l1_cache;
    M_REQUIRE(cache_hit(mem_space, l1_cache, paddr, &p_line_inl1, &hit_way, &hit_index, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, %s," error in cache_hit");

    // ############################### CASE FIND THE VALUE IN L1 ############################################
    if (hit_way != HIT_WAY_MISS) {
        if (cache_type == L1_DCACHE){
             *word = cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way)[w_select];
        }

        if (cache_type == L1_ICACHE){
            *word = cache_line(l1_icache_entry_t, L1_ICACHE_WAYS, hit_index, hit_way)[w_select];
        }
    }

    // ############################## CASE WE DID NOT FIND THE VALUE IN L1 ##################################
    else
        {

            //CHECK IF DATA IN L2
            M_REQUIRE(cache_hit(mem_space, l2_cache, paddr, &p_line_inl1, &hit_way, &hit_index, L2_CACHE) == 0, ERR_BAD_PARAMETER, %s," error in cache_hit");

            int line_index_l1 = (addr >> INDEX_OFFSET) & INDEX_L1_MASK;
            int tag_l1 = addr >> L1_ICACHE_TAG_REMAINING_BITS;

            //HIT IN L2
            if (hit_way != HIT_WAY_MISS) {


                if (cache_type == L1_ICACHE){
                    cache_read_L2(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE_LINES);
                }
                
                if (cache_type == L1_DCACHE) {
                    cache_read_L2(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE_LINES);
                }
            }

    // ############################## CASE WE DID NOT FIND THE VALUE IN L2 ##################################
            else {

                if (cache_type == L1_ICACHE) {

                    cache_read_memory(l1_icache_entry_t, L1_ICACHE_WAYS);

                }

                if (cache_type == L1_DCACHE) {

                    cache_read_memory(l1_dcache_entry_t, L1_DCACHE_WAYS);
                        
                }
            }
        }

    return ERR_NONE;
}





int cache_read_byte(const void * mem_space,
                    phy_addr_t * p_paddr,
                    mem_access_t access,
                    void * l1_cache,
                    void * l2_cache,
                    uint8_t * p_byte,
                    cache_replace_t replace){


    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(p_paddr);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(p_byte);

    // INDEX OF BYTE TO SELECT
    uint8_t index = p_paddr->page_offset % 4;
    
    //READ WORD
    word_t * word = malloc(sizeof(word_t));
    M_REQUIRE_NON_NULL(word);
    M_REQUIRE(cache_read(mem_space, p_paddr, access, l1_cache, l2_cache, word, replace) == ERR_NONE, ERR_BAD_PARAMETER, %s, "error in cache read");
    
    // RETURN CORRESPONDING BYTE IN THE CORRESPONDING WORD
    *p_byte = (*word >> (index * BYTE_SIZE)) & BYTE_MAX;

    return ERR_NONE;
}



int cache_write(void * mem_space,
                phy_addr_t * paddr,
                void * l1_cache,
                void * l2_cache,
                const uint32_t * word,
                cache_replace_t replace){

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(word);

    // CREATE DIFFERENT VARIABLE THAT WE NEED TO USE DURING THE FUNCTION (NAME EXPLICITLY)
    uint32_t addr = phy_to_uint32(*paddr);
    uint8_t w_select = (addr >> 2) & 0b11;

    const uint32_t * p_line_inl1 = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
    const uint32_t * p_line_inl2 =  calloc(L2_CACHE_WORDS_PER_LINE, sizeof(word_t));
    M_REQUIRE_NON_NULL(p_line_inl1);
    M_REQUIRE_NON_NULL(p_line_inl2);

    uint8_t hit_way = 0;
    uint16_t hit_index = 0;

    void * cache = l1_cache;
    M_REQUIRE(cache_hit(mem_space, l1_cache, paddr, &p_line_inl1, &hit_way, &hit_index, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_hit ");


    // #################################################### CASE WE HIT IN THE L1_DCACHE ###############################################################
    if (hit_way != HIT_WAY_MISS){

        // COPY THE LINE WE WILL MODIFY
        uint32_t * line_to_use = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
        M_REQUIRE_NON_NULL(line_to_use);
        M_REQUIRE_NON_NULL(memcpy(line_to_use, p_line_inl1, L1_DCACHE_WORDS_PER_LINE*sizeof(word_t)));

        //UPDATE THE WORLD (WRITE)
        line_to_use[w_select] = *word;

        //INITIALIZE A NEW ENTRY WITH THE NEW LINE TO INSERT IT THEN
        l1_dcache_entry_t * entry = malloc(sizeof(l1_dcache_entry_t));
        M_REQUIRE_NON_NULL(entry);
        entry->tag = cache_tag(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way);
        M_REQUIRE_NON_NULL(memcpy(entry->line, line_to_use, L1_DCACHE_WORDS_PER_LINE * sizeof(word_t)));
        entry->v =1;
        entry->age =0;

        //INSERT THE NEW ENTRY AND UPDATE AGE
        M_REQUIRE(cache_insert(hit_index, hit_way, entry,  l1_cache, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
        LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_way, hit_index); 

        //COPY IN MEMORY THE LINE
        M_REQUIRE_NON_NULL(memcpy(mem_space + phy_to_uint32(*paddr), line_to_use, L1_ICACHE_WORDS_PER_LINE*sizeof(word_t)));

        return ERR_NONE;
    }


    uint8_t * hit_way_l2 = malloc(sizeof(uint8_t));
    uint16_t  * hit_index_l2 = malloc(sizeof(uint16_t));
    M_REQUIRE_NON_NULL(hit_way_l2);
    M_REQUIRE_NON_NULL(hit_index_l2);

    M_REQUIRE(cache_hit(mem_space, l2_cache, paddr, &p_line_inl2, hit_way_l2, hit_index_l2, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, "error in cache_hit");

    // ################################################################ CASE WE HIT IN L2 BUT NOT L1 ###########################################################
    if (*hit_way_l2 != HIT_WAY_MISS){

        //COPY THE LINE WE WILL MODIFY
        uint32_t * line_to_use = calloc(L2_CACHE_WORDS_PER_LINE, sizeof(word_t));
        M_REQUIRE_NON_NULL(line_to_use);
        cache = l2_cache;
        M_REQUIRE_NON_NULL(memcpy(line_to_use, p_line_inl2, L2_CACHE_WORDS_PER_LINE * sizeof(word_t)));

        //UPDATE THE WORLD (WRITE)
        line_to_use[w_select] = *word;

        cache = l2_cache;

        //INITIALIZE A NEW ENTRY WITH THE NEW LINE TO INSERT IT THEN
        l2_cache_entry_t * entry_l2 = malloc(sizeof(l2_cache_entry_t));
        M_REQUIRE_NON_NULL(entry_l2);
        M_REQUIRE_NON_NULL(memcpy(entry_l2->line, line_to_use, L2_CACHE_WORDS_PER_LINE * sizeof(word_t)));
        entry_l2->tag = cache_tag(l2_cache_entry_t, L2_CACHE_WAYS, *hit_index_l2, *hit_way_l2) ;
        entry_l2->v = 1;
        entry_l2->age =0;

        //INSERT THE NEW ENTRY AND UPDATE AGE
        M_REQUIRE(cache_insert(*hit_index_l2, *hit_way_l2, entry_l2,  l2_cache, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
        LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, *hit_way_l2, *hit_index_l2);

        //COPY IN MEMORY THE LINE
        M_REQUIRE_NON_NULL(memcpy(mem_space + phy_to_uint32(*paddr), line_to_use , L2_CACHE_WORDS_PER_LINE * sizeof(word_t)));

        // CREATE AN ENTRY TO TRANSFER THE DATA FROM L2 TO L1D
        l1_dcache_entry_t *  entry = malloc(sizeof(l1_dcache_entry_t));
        M_REQUIRE_NON_NULL(entry);  
        transfer_to_l1(l2_cache, entry, L1_DCACHE , *hit_way_l2, *hit_index_l2);//L1_DCACHE??
        entry->v = 1;
        entry->tag = (addr >> L1_DCACHE_TAG_REMAINING_BITS);

        // FIND THE WAY WAY AND INDEX WHERE WE WILL INSERT THE ENTRY
        uint16_t index_in_l1 = *hit_index_l2  & INDEX_L1_MASK;
        uint8_t way_in_l1 = invalid_way(l1_cache, L1_DCACHE, index_in_l1);
        cache = l1_cache;

        // CASE WE FIND AN AVAILABLE WAY IN L1
        if (way_in_l1 != HIT_WAY_MISS){

            M_REQUIRE(cache_insert(index_in_l1, way_in_l1, entry, l1_cache, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
            LRU_age_increase(l1_dcache_entry_t, L1_DCACHE_WAYS, way_in_l1, index_in_l1);
            return ERR_NONE;
        }

        // CASE WE NEED TO EVICT A EN ENTRY FRON L1 TO L2 BECAUSE THERE WAS NO PLACE 
        else{

            // FIND THE WAY OF THE ENTRY TO EVICT
            uint8_t way_evicted = LRU_way(l1_cache, L1_DCACHE, index_in_l1);

            //CREATE QN ENTRY THAHT WE SET TO THE ENTRY WE EVICT
            l2_cache_entry_t * entry_evicted = malloc(sizeof(l2_cache_entry_t));
            M_REQUIRE_NON_NULL(entry_evicted);
            entry_evicted->age = 0;
            entry_evicted->tag = (entry->tag >> (L2_CACHE_TAG_REMAINING_BITS - L1_DCACHE_TAG_REMAINING_BITS));
            M_REQUIRE_NON_NULL(memcpy(entry_evicted->line, entry->line, L1_DCACHE_WORDS_PER_LINE*sizeof(word_t)));

            //INSERT THE ORIGINAL ENTRY IN L1 AND UPDATE AGE
            M_REQUIRE(cache_insert(index_in_l1, way_evicted, entry, l1_cache, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
            LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, way_evicted, index_in_l1);

            //FIND THE WAY TO INSERT IN L2 FOR THE EVICTED
            uint8_t way_for_l2 = invalid_way(l2_cache, L2_CACHE, *hit_index_l2);
            cache = l2_cache;

            //CASE THERE IS A PLACE IN L2
            if (way_for_l2 != HIT_WAY_MISS){
                //INSERT THE EVICTED ENTRY IN L2
                M_REQUIRE(cache_insert(*hit_index_l2, way_for_l2, entry_evicted, l2_cache, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
                LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, way_for_l2, *hit_index_l2);

            // CASE THERE IS NO PLACE IN L2 SO WE NEED TO DELETE ONE
            } else {
                // FIND THE WAY DEPENDING ON THE AGE TO INSERT THE EVICTED ENTRY
                uint8_t way_evicted2 = LRU_way(l2_cache, L2_CACHE, *hit_index_l2);
                M_REQUIRE(cache_insert(*hit_index_l2, way_evicted2, entry_evicted, l2_cache, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
                LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, way_evicted2, *hit_index_l2);
                }
        }

    return ERR_NONE;

    // ################################################# CASE NOT FOUND IN L1 AND L2 #############################################
    } else {

        // GET LINE FROM MEMORY, MODIFY IT AND INSERT IT BACK
        uint32_t * line_memory = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
        M_REQUIRE_NON_NULL(line_memory);
        M_REQUIRE_NON_NULL(memcpy(line_memory, mem_space+addr, L1_DCACHE_WORDS_PER_LINE*sizeof(word_t)));
        line_memory[w_select] = *word;
        M_REQUIRE_NON_NULL(memcpy(mem_space+addr, line_memory, L1_DCACHE_WORDS_PER_LINE*sizeof(word_t)));
        
        // CREATE AN L1 ENTRY TO INSERT NEW LINE IN L1D
        l1_dcache_entry_t * newd = malloc(sizeof(l1_dcache_entry_t));
        M_REQUIRE_NON_NULL(newd);
        M_REQUIRE(cache_entry_init(mem_space, paddr, newd, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_entry_init"); \

        //FIND THE WAY WHERE TO INSERT LINE
        uint16_t index_linel1 = (addr >> INDEX_OFFSET) & INDEX_L1_MASK;
        uint8_t wayld = invalid_way(l1_cache, L1_DCACHE, index_linel1);

        cache = l1_cache;

        //CASE THERE IS PLACE IN L1D CACHE
        if (wayld != HIT_WAY_MISS){
            M_REQUIRE(cache_insert(index_linel1, wayld, newd, l1_cache, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
            LRU_age_increase (l1_dcache_entry_t, L1_DCACHE_WAYS, wayld, index_linel1);
        }

        //CASE THERE IS NO PLACE, SO WE NEED TO EVICT ONE ENTRY AND PUT IT IN L2
        else{
            
            //FIND THE WAY OF THE ENTRY TO EVICT
            uint8_t way_evicted2 = LRU_way(l1_cache, L1_DCACHE, index_linel1);

            //CREATE AN ENTRY TO COPY THE EVICTED ONE
            l2_cache_entry_t * entry_evicted = malloc(sizeof(l2_cache_entry_t));
            M_REQUIRE_NON_NULL(entry_evicted);
            M_REQUIRE_NON_NULL(memcpy(entry_evicted->line, cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, index_linel1, way_evicted2), L1_DCACHE_WORDS_PER_LINE * sizeof(word_t)));
            entry_evicted->age = 0;
            entry_evicted->tag = (cache_tag(l1_dcache_entry_t, L1_DCACHE_WAYS, index_linel1, way_evicted2) >> (L1_DCACHE_TAG_REMAINING_BITS - L2_CACHE_WORDS_PER_LINE));

            // INSERT THE NEW ENTRY IN L1 AND UPDATE AGE
            M_REQUIRE(cache_insert(index_linel1, way_evicted2, newd, l1_cache, L1_DCACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
            LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, way_evicted2, index_linel1);

            //FIND THE LINE AND WAY WHERE INSERT THE EVICTED ENTRY
            uint16_t l2index = (phy_to_uint32(*paddr) >> INDEX_OFFSET) & INDEX_L2_MASK;
            cache = l2_cache;
            uint8_t way_l2 = invalid_way(l2_cache, L2_CACHE, l2index);

            //CASE THERE IS A PLACE IN L2
            if (way_l2 != HIT_WAY_MISS){ 
                M_REQUIRE(cache_insert(l2index, way_l2, entry_evicted, l2_cache, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
                LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, way_l2, l2index);
            } 

            //CASE THERE IS NO PLACE, SO WE FIND THE ENTRY TO DELETE USING REPLACEMENT POLICY
            else{
                way_l2 = LRU_way(l2_cache, L2_CACHE, l2index);
                M_REQUIRE(cache_insert(l2index, way_l2, entry_evicted, l2_cache, L2_CACHE) == ERR_NONE, ERR_BAD_PARAMETER, %s, " error in cache_insert");
                LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, way_l2, l2index);
            }
        }

    return ERR_NONE;

    }

return ERR_NONE;

}

int cache_write_byte(void * mem_space,
                     phy_addr_t * paddr,
                     void * l1_cache,
                     void * l2_cache,
                     uint8_t p_byte,
                     cache_replace_t replace){
    
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(p_byte);
    M_REQUIRE_NON_NULL(l1_cache);

    //INITIALISE WORD
    word_t * word = malloc(sizeof(word_t));
    M_REQUIRE_NON_NULL(word);

    //INDEX OF THE BYTE TO GET
    uint8_t index = phy_to_uint32(*paddr) % 4;
    
    M_REQUIRE(cache_read(mem_space, paddr, DATA, l1_cache, l2_cache, word, replace)== ERR_NONE, ERR_BAD_PARAMETER, %s, "error in cache read");

    //GET BYTE
    *word = *word | (p_byte << (BYTE_SIZE*index));

    M_REQUIRE(cache_write(mem_space, paddr, l1_cache, l2_cache, word, replace)== ERR_NONE, ERR_BAD_PARAMETER, %s, "error in cache write");

    return ERR_NONE;
}


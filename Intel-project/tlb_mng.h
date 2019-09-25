#pragma once

/**
 * @file tlb_mng.h
 * @brief TLB management functions for fully-associative TLB
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "tlb.h"
#include "addr.h"
#include "list.h"


//replacement_policy struct definition
typedef struct
{
    list_t* ll;
    node_t* (*push_back)(list_t* ll, const list_content_t* value);
    void (*move_back)(list_t* this, node_t* node);

} replacement_policy_t;



//=========================================================================
/**
 * @brief Clean a TLB (invalidate, reset...).
 *
 * This function erases all TLB data.
 * @param tlb pointer to the TLB
 * @return error code
 */
int tlb_flush(tlb_entry_t * tlb);

//=========================================================================
/**
 * @brief Check if a TLB entry exists in the TLB.
 *
 * On hit, return success (1) and update the physical page number passed as the pointer to the function.
 * On miss, return miss (0).
 *
 * @param vaddr pointer to virtual address
 * @param paddr (modified) pointer to physical address
 * @param tlb pointer to the beginning of the TLB
 * @param replacement_policy the eviction/replacement policy used by the TLB
 * @return hit (1) or miss (0)
 */
int tlb_hit(const virt_addr_t * vaddr,
            phy_addr_t * paddr,
            const tlb_entry_t * tlb,
            replacement_policy_t * replacement_policy);

//=========================================================================
/**
 * @brief Insert an entry to a tlb.
 * Eviction policy is least recently used (LRU).
 *
 * @param line_index the number of the line to overwrite
 * @param tlb_entry pointer to the tlb entry to insert
 * @param tlb pointer to the TLB
 * @return  error code
 */
int tlb_insert( uint32_t line_index,
                const tlb_entry_t * tlb_entry,
                tlb_entry_t * tlb);

//=========================================================================
/**
 * @brief Initialize a TLB entry
 * @param vaddr pointer to virtual address, to extract tlb tag
 * @param paddr pointer to physical address, to extract physical page number
 * @param tlb_entry pointer to the entry to be initialized
 * @return  error code
 */
int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    tlb_entry_t * tlb_entry);

//=========================================================================
/**
 * @brief Ask TLB for the translation.
 *
 * @param mem_space pointer to the memory space
 * @param vaddr pointer to virtual address
 * @param paddr (modified) pointer to physical address (returned from TLB)
 * @param tlb pointer to the beginning of the TLB
 * @param hit_or_miss (modified) hit (1) or miss (0)
 * @return error code
 */
int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                tlb_entry_t * tlb,
                replacement_policy_t * replacement_policy,
                int* hit_or_miss);

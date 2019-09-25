#pragma once

/**
 * @file tlb.h
 * @brief definitions associated to a fully-associative TLB
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "addr.h"

#include <stdint.h>

#define TLB_LINES 128 // the number of entries

//Definition of a tlb entry
typedef struct 
{
    uint64_t tag : VIRT_PAGE_NUM;
    uint32_t phy_page_num : PHY_PAGE_NUM;
    uint8_t v : 1;
    
} tlb_entry_t;

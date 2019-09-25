#pragma once

/**
 * @file addr.h
 * @brief Type definitions for a virtual and physical addresses.
 *
 * @date 2018-19
 */

#include <stdint.h>

#define PAGE_OFFSET     12
#define PAGE_SIZE       4096 // = 2^12 B = 4 kiB pages


#define PTE_ENTRY       9
#define PMD_ENTRY       9
#define PUD_ENTRY       9
#define PGD_ENTRY       9
/* the number of entries in a page directory = 2^9
* each entry size is equal to the size of a physical address = 32b
*/
#define PD_ENTRIES      512

#define VIRT_PAGE_NUM   36 // = PTE_ENTRY + PUD_ENTRY + PMD_ENTRY + PGD_ENTRY
#define VIRT_ADDR_RES   16
#define VIRT_ADDR       64 // = VIRT_ADDR_RES + 4*9 + PAGE_OFFSET

#define PHY_PAGE_NUM    20
#define PHY_ADDR        32 // = PHY_PAGE_NUM + PAGE_OFFSET

//===============================OUR PART====================================================

/*
word_t byte_t and pte_t are only 'integer' no need for structure
 */
typedef uint32_t word_t;
typedef uint8_t byte_t;
typedef uint32_t pte_t;


/*
virtu_addr_t and phy_addr_t are composed of different element, so we create stucture
 */
typedef struct{
	uint16_t reserved: VIRT_ADDR_RES;
 	uint16_t pgd_entry: PGD_ENTRY;
 	uint16_t pud_entry: PUD_ENTRY;
 	uint16_t pmd_entry: PMD_ENTRY;
 	uint16_t pte_entry: PTE_ENTRY;
 	uint16_t page_offset: PAGE_OFFSET;
} virt_addr_t;

typedef struct{
 	uint32_t phy_page_num: PHY_PAGE_NUM;
 	uint16_t page_offset: PAGE_OFFSET;
} phy_addr_t;











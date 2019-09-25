#pragma once

/**
 * @file addr_mng.h
 * @brief address management functions (init, print, etc.)
 */

#include "addr.h"

#include <stdio.h> // FILE

//=========================================================================
/**
 * @brief Initialize virt_addr_t structure. Reserved bits are zeroed.
 * @param vaddr (modified) the virtual address structure to be initialized
 * @param pgd_entry the value of the PGD offset of the virtual address
 * @param pud_entry the value of the PUD offset of the virtual address
 * @param pmd_entry the value of the PMD offset of the virtual address
 * @param pte_entry the value of the PT  offset of the virtual address
 * @param page_offest the value of the physical memory page offset of the virtual address
 * @return error code
 */
int init_virt_addr(virt_addr_t * vaddr,
                   uint16_t pgd_entry,
                   uint16_t pud_entry, uint16_t pmd_entry,
                   uint16_t pte_entry, uint16_t page_offset);

//=========================================================================
/**
 * @brief Initialize virt_addr_t structure from uint64_t. Reserved bits are zeros.
 * @param vaddr (modified) the virtual address structure to be initialized
 * @param vaddr64 the virtual address provided as a 64-bit pattern
 * @return error code
 */
int init_virt_addr64(virt_addr_t * vaddr, uint64_t vaddr64);

//=========================================================================
/**
 * @brief Initialize phy_addr_t structure.
 * @param paddr (modified) the physical address structure to be initialized
 * @param page_begin the address of the top of the physical page
 * @param page_offset the index (offset) inside the physical page
 * @return error code
 */
int init_phy_addr(phy_addr_t* paddr, uint32_t page_begin, uint32_t page_offset);

//=========================================================================
/**
 * @brief Convert virt_addr_t structure to uint64_t. It's the reciprocal of init_virt_addr64().
 * @param vaddr the virtual address structure to be translated to a 64-bit pattern
 * @return the 64-bit pattern corresponding to the physical address
 */
uint64_t virt_addr_t_to_uint64_t(const virt_addr_t * vaddr);

//=========================================================================
/**
 * @brief Extract virtual page number from virt_addr_t structure.
 * @param vaddr the virtual address structure
 * @return the virtual page number corresponding to the virtual address
 */
uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t * vaddr);

//=========================================================================
/**
 * @brief print a virtual address to stream
 * @param where the stream where to print to
 * @param vaddr the virtual address to be printed
 * @return number of printed characters
 */
int print_virtual_address(FILE* where, const virt_addr_t* vaddr);

//=========================================================================
/**
 * @brief print a physical address to stream
 * @param where the stream where to print to
 * @param paddr the physical address to be printed
 * @return number of printed characters
 */
int print_physical_address(FILE* where, const phy_addr_t* paddr);

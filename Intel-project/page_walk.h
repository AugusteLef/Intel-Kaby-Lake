#pragma once

/**
 * @file page_walk.h
 * @brief
 */

#include "addr.h"

/**
 * @brief Page walker: virtual address to physical address conversion.
 *
 * @param mem_space starting address of our simulated memory space
 * @param vaddr virtual address to be converted
 * @param paddr (SET) physical address
 * @return error code
 */
int page_walk(const void* mem_space, const virt_addr_t* vaddr, phy_addr_t* paddr);

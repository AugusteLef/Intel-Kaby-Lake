#include <stdint.h>
#include "addr_mng.h"
#include <inttypes.h>
#include "error.h"

#define  MASK_9bit_1  0x01FF // 0b1111111000000000
#define  MASK_12bit_1  0x0FFF // 0b1111000000000000

int init_virt_addr(virt_addr_t * vaddr,
                    uint16_t pgd_entry,
                    uint16_t pud_entry, uint16_t pmd_entry,
                    uint16_t pte_entry, uint16_t page_offset){


	// If one of the parameters doesn't have the good length : ERROR else none
	M_REQUIRE(((pgd_entry >> PGD_ENTRY) == 0x00) &&
	          ((pud_entry >> PUD_ENTRY) == 0x00) &&
	          ((pmd_entry >> PMD_ENTRY) == 0x00) &&
	          ((pte_entry >> PTE_ENTRY) == 0x00) &&
	          ((page_offset >> PAGE_OFFSET) == 0x00)
		, ERR_BAD_PARAMETER,"%s", ERR_MESSAGES[ERR_SIZE]);

	// Verify that vaddr is not null
	M_REQUIRE_NON_NULL(vaddr);

	// Affect the differents parameters to the vaddr
	vaddr->reserved = 0; // Always "nulle" as describe in the pdf file
	vaddr->pgd_entry = pgd_entry;
	vaddr->pud_entry = pud_entry;
	vaddr->pmd_entry = pmd_entry;
	vaddr->pte_entry = pte_entry;
	vaddr->page_offset = page_offset;

	// If no erro of parameters, and when all arguments have been affected to vaddr, return ERR_NONE (No error)
	return ERR_NONE;	

}

int init_virt_addr64(virt_addr_t * vaddr, uint64_t vaddr64){

	// Check that vaddr is non NULL	 
	M_REQUIRE_NON_NULL(vaddr);

	// here we shift to the left to have the bit we want to keep int the place of the LSB, and then apply the mask

	vaddr->page_offset = (uint16_t)vaddr64 & MASK_12bit_1;
	vaddr->pte_entry = (uint16_t)(vaddr64 >> PAGE_OFFSET) & MASK_9bit_1;
	vaddr->pmd_entry = (uint16_t)(vaddr64 >> (PAGE_OFFSET + PTE_ENTRY)) & MASK_9bit_1;
	vaddr->pud_entry = (uint16_t)(vaddr64 >> (PAGE_OFFSET + PTE_ENTRY + PMD_ENTRY)) & MASK_9bit_1;
	vaddr->pgd_entry = (uint16_t)(vaddr64 >> (PAGE_OFFSET + PTE_ENTRY + PMD_ENTRY + PUD_ENTRY)) & MASK_9bit_1;

	vaddr->reserved = 0x0000; // Always "nulle" as describe in the pdf file

	// If no problem, return erro none
	return ERR_NONE;

}

int init_phy_addr(phy_addr_t* paddr, uint32_t page_begin, uint32_t page_offset){

	// Check size of page begin and page offset
	M_REQUIRE(((page_offset >> PAGE_OFFSET) == 0), ERR_BAD_PARAMETER,"%s", ERR_MESSAGES[ERR_SIZE]);

	// Check that paddr is non null
	M_REQUIRE_NON_NULL(paddr);

	//Check that page_begin is a multiple of page_size
	M_REQUIRE(page_begin%PAGE_SIZE == 0, ERR_BAD_PARAMETER,"%s", ERR_MESSAGES[ERR_SIZE]);

	// fill the argument of paddr with the paramaters
	paddr->phy_page_num = (page_begin >> PAGE_OFFSET);
	paddr->page_offset = (uint16_t)(page_offset & MASK_12bit_1);

	// return err_none
	return ERR_NONE;

}

uint64_t virt_addr_t_to_uint64_t(const virt_addr_t * vaddr){
	// Verify that vaddr is not null
	M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, ERR_BAD_PARAMETER);

	// get x_entry by using virt_addr_t_to_virtual_page_number and then shift the result by 12 to the left and add with and OR bit to bit the page offset
	uint64_t vaddrint64 = virt_addr_t_to_virtual_page_number(vaddr);
	vaddrint64 = (vaddrint64 << PAGE_OFFSET) | vaddr->page_offset;
	return vaddrint64;
}


uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t * vaddr){
	// Verify that vaddr is not null
	M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, ERR_BAD_PARAMETER);

	// We add by an OR bit to bit and by shifting from the good number of bit 
	uint64_t virtu_numer = 0;
	virtu_numer |= vaddr->pgd_entry;
	virtu_numer = (virtu_numer << PUD_ENTRY) | vaddr->pud_entry;
	virtu_numer = (virtu_numer << PMD_ENTRY) | vaddr->pmd_entry;
	virtu_numer = (virtu_numer << PTE_ENTRY) | vaddr->pte_entry;
	return virtu_numer;
}



int print_virtual_address(FILE* where, const virt_addr_t* vaddr){
	// Verify that vaddr is not null and file too
	M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, ERR_BAD_PARAMETER);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(where, ERR_BAD_PARAMETER);

	// We assume that we receive a file open, and that we don't have to close it (the one who call this function take care of this)
	int number_of_char = fprintf(where, "PGD=0x%" PRIX16 "; PUD=0x%" PRIX16 "; PMD=0x%" PRIX16 "; PTE=0x%" PRIX16 "; offset=0x%" PRIX16,
		 vaddr->pgd_entry, vaddr->pud_entry, vaddr->pmd_entry, vaddr->pte_entry, vaddr->page_offset);

	return number_of_char;	
}


int print_physical_address(FILE* where, const phy_addr_t* paddr){
	// Verify that paddr is not null and file too
	M_REQUIRE_NON_NULL_CUSTOM_ERR(paddr, ERR_BAD_PARAMETER);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(where, ERR_BAD_PARAMETER);

	// We assume that we receive a file open, and that we don't have to close it (the one who call this function take care of this)
	int number_of_char = fprintf(where, "page num=0x%" PRIX32 "; offset=0x%" PRIX32, paddr->phy_page_num, paddr->page_offset);

	return number_of_char;

}

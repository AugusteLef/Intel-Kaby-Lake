/**
 * @file test-addr.c
 * @brief test code for virtual and physical address types and functions
 *
 * @author Jean-Cédric Chappelier and Atri Bhattacharyya
 * @date Jan 2019
 */

// ------------------------------------------------------------
// for thread-safe randomization
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

// ------------------------------------------------------------
#include <stdio.h> // for puts(). to be removed when no longer needed.

#include <check.h>
#include <inttypes.h>
#include <assert.h>

#include "tests.h"
#include "util.h"
#include "addr.h"
#include "addr_mng.h"

// ------------------------------------------------------------
// Preliminary stuff

#define random_value(TYPE)  (TYPE)(rand() % 4096)
#define REPEAT(N) for(unsigned int i_ = 1; i_ <= N; ++i_)

uint64_t generate_Nbit_random(int n)
{
    uint64_t val = 0;
    for (int i = 0; i < n; i++) {
        val <<= 1;
        val |= (unsigned)(rand() % 2);
    }
    return val;
}

virt_addr_t create_random_virt_addr() {


    virt_addr_t addr = {generate_Nbit_random(16), generate_Nbit_random(9), generate_Nbit_random(9), generate_Nbit_random(9), generate_Nbit_random(9), generate_Nbit_random(12)};
    return addr;

}

uint64_t generate_random_size_bit(int max) {

    int size = rand() % (max + 1);
    uint64_t bit =  generate_Nbit_random(size);
    return bit;
}

// ------------------------------------------------------------
// First basic tests, provided to students

START_TEST(addr_basic_test_1)
{
// ------------------------------------------------------------
    virt_addr_t vaddr;
    zero_init_var(vaddr);

    srand(time(NULL) ^ getpid() ^ pthread_self());
#pragma GCC diagnostic pop

    REPEAT(100) {
        uint16_t pgd_entry   = (uint16_t) generate_Nbit_random(PGD_ENTRY);
        uint16_t pud_entry   = (uint16_t) generate_Nbit_random(PUD_ENTRY);
        uint16_t pmd_entry   = (uint16_t) generate_Nbit_random(PMD_ENTRY);
        uint16_t pte_entry   = (uint16_t) generate_Nbit_random(PTE_ENTRY);
        uint16_t page_offset = (uint16_t) generate_Nbit_random(PAGE_OFFSET);

        (void)init_virt_addr(&vaddr, pgd_entry, pud_entry, pmd_entry, pte_entry, page_offset);

        ck_assert_int_eq(vaddr.pgd_entry, pgd_entry);
        ck_assert_int_eq(vaddr.pud_entry, pud_entry);
        ck_assert_int_eq(vaddr.pmd_entry, pmd_entry);
        ck_assert_int_eq(vaddr.pte_entry, pte_entry);
        ck_assert_int_eq(vaddr.page_offset, page_offset);
    }
}
END_TEST

START_TEST(ERR_NONE_CHECK) {

    virt_addr_t vaddr = {0};
    ck_assert_err_none(init_virt_addr(&vaddr, 0X1FF, 0X1FF, 0X1FF, 0X1FF, 0X1FF)); //0X1FF = 111111111

} END_TEST

START_TEST(test_init_virt_addr) {

    REPEAT(100) {

        virt_addr_t vaddr;
        uint64_t pgd = generate_Nbit_random(16);
        uint64_t pud = generate_Nbit_random(16);
        uint64_t pmd = generate_Nbit_random(16);
        uint64_t pte = generate_Nbit_random(16);
        uint64_t page_offset = generate_Nbit_random(16);

        if (pgd > 511 || pud > 511 || pmd > 511 || pte > 511 || page_offset > 4095) {
            ck_assert_bad_param(init_virt_addr(&vaddr, pgd, pud, pmd, pte, page_offset));
        } else {
            ck_assert_err_none(init_virt_addr(&vaddr, pgd, pud, pmd, pte, page_offset));
        }

    }


} END_TEST

START_TEST(virt_addr_t_to_virtual_page_number_test) {



    REPEAT(100) {

        uint16_t pgd = generate_Nbit_random(PGD_ENTRY);
        uint16_t pud = generate_Nbit_random(PUD_ENTRY);
        uint16_t pmd = generate_Nbit_random(PMD_ENTRY);
        uint16_t pte = generate_Nbit_random(PTE_ENTRY);

        virt_addr_t vaddr = {0, pgd, pud, pmd, pte, 0};

        uint64_t vaddr1 = virt_addr_t_to_virtual_page_number(&vaddr);

        virt_addr_t final;
        init_virt_addr64(&final, vaddr1 << PAGE_OFFSET);

        ck_assert_int_eq(final.pgd_entry , vaddr.pgd_entry);
        ck_assert_int_eq(final.pmd_entry , vaddr.pmd_entry);
        ck_assert_int_eq(final.pud_entry , vaddr.pud_entry);
        ck_assert_int_eq(final.pte_entry , vaddr.pte_entry);


    }

} END_TEST

START_TEST(virt_addr_to_uint64){


    REPEAT(100) {

        uint16_t pgd = generate_Nbit_random(PGD_ENTRY);
        uint16_t pud = generate_Nbit_random(PUD_ENTRY);
        uint16_t pmd = generate_Nbit_random(PMD_ENTRY);
        uint16_t pte = generate_Nbit_random(PTE_ENTRY);
        uint16_t off = generate_Nbit_random(PAGE_OFFSET);

        virt_addr_t vaddr = {0, pgd, pud, pmd, pte, off};

        uint64_t test = virt_addr_t_to_uint64_t(&vaddr);
        uint16_t mask9 = 0b111111111;
        uint16_t maskoff = 0b111111111111;
        


        ck_assert_int_eq(test & maskoff , vaddr.page_offset);
        ck_assert_int_eq( (test >> PAGE_OFFSET) & mask9, vaddr.pte_entry);
        ck_assert_int_eq((test >> (PAGE_OFFSET + PTE_ENTRY))& mask9 , vaddr.pmd_entry);
        ck_assert_int_eq( (test >> (PAGE_OFFSET + PTE_ENTRY + PMD_ENTRY))& mask9, vaddr.pud_entry);
        ck_assert_int_eq( (test >>  (PAGE_OFFSET + PTE_ENTRY + PMD_ENTRY + PUD_ENTRY))& mask9 , vaddr.pgd_entry);


    }

} END_TEST

START_TEST(init_phy_addr_test){


    REPEAT(100) {
        
        phy_addr_t phyad;
        zero_init_ptr(&phyad);

        uint32_t pagenum = generate_Nbit_random(32); // OFFSET + 20 bits
        uint32_t off = generate_Nbit_random(PAGE_OFFSET);
        
        (void) init_phy_addr(&phyad , pagenum, off);
                

        ck_assert_int_eq(off, phyad.page_offset );
        ck_assert_int_eq( (pagenum >> PAGE_OFFSET), phyad.phy_page_num);
    }

}END_TEST

// ======================================================================
Suite* addr_test_suite()
{
    Suite* s = suite_create("Virtual and Physical Address Tests");

    Add_Case(s, tc1, "basic tests");
    tcase_add_test(tc1, addr_basic_test_1);
    tcase_add_test(tc1, ERR_NONE_CHECK);
    tcase_add_test(tc1, test_init_virt_addr);
    tcase_add_test(tc1, virt_addr_t_to_virtual_page_number_test);
    tcase_add_test(tc1, virt_addr_to_uint64);
    tcase_add_test(tc1, init_phy_addr_test);


    return s;
}

TEST_SUITE(addr_test_suite)


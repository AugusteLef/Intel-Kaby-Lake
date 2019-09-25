#!/bin/bash

## Basic tests for week 5

source $(dirname ${BASH_SOURCE[0]})/test_env.sh

test=0

# ======================================================================
# tool function
check_output() {

    checkX "Test commands and programs emulation" "$1"

    testfile="tests/files/$2"
    [ -f "$testfile" ] || error "Expected test file \"$testfile\" not found."
    
    EXPECTED_OUTPUT="${3}"

    mytmp="$(new_tmp_file)"
    # gets stdout in case of success, stderr in case of error
    ACTUAL_OUTPUT="$("$1" "$testfile" 2>"$mytmp"|| cat "$mytmp")"

    diff -w <(echo "$ACTUAL_OUTPUT") <(echo -e "$EXPECTED_OUTPUT") \
        && echo "PASS" \
        || (echo "FAIL"; \
            echo -e "Expected:\n$EXPECTED_OUTPUT"; \
            echo -e "Actual:\n$ACTUAL_OUTPUT"; \
            exit 1)
}

# ======================================================================
# test test-commands on a few provided files
printf "Test %1d (test-command 1): " $((++test))
check_output test-commands commands01.txt \
"R I @0x0000000000000000 
R DW @0x0000000040200000 
R DB @0x0000000040200002 
W DB 0xAA @0x0000000040000005 
W DW 0x0000BEEF @0x0000000040000010"

printf "Test %1d (test-command 2): " $((++test))
check_output test-commands commands02.txt \
"R I @0x0000000000000000 
R I @0x0000000000000004 
R DW @0x0000000000200000 
R I @0x0000000000000008 
R DW @0x0000000040000000 
R I @0x000000000000000C 
R DW @0x0000000040200000 
R I @0x0000000000000010 
R DW @0x0000000040200004 
R I @0x0000000000000014 
R DW @0x0000000040200008 
R I @0x0000000000000018 
R DW @0x0000000000200004 
R I @0x000000000000001C 
R DW @0x0000000040000004 
R I @0x0000000000000020"

# ======================================================================
echo "SUCCESS"

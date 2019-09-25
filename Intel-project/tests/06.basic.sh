#!/bin/bash

## Basic tests for week 6

source $(dirname ${BASH_SOURCE[0]})/test_env.sh

test=0

# ======================================================================
# tool functions
check_output_with_file() {

    checkX "Test Memory" "$1"

    testbin="$1"
    type="$2"

    ref='tests/files'
    testfile="${ref}/$3"
    [ -f "$testfile" ] || error "Expected test file \"$testfile\" not found."

    refoutput="${ref}/$4"
    [ -f "$refoutput" ] || error "Expected output file \"$refoutput\" not found."
    
    mytmp="$(new_tmp_file)"
    shift 4
    # gets stdout in case of success, stderr in case of error
    ACTUAL_OUTPUT="$("$testbin" "$type" "$testfile" "$@" 2>"$mytmp" || cat "$mytmp")"

    diff -w <(echo "$ACTUAL_OUTPUT") <(cat "$refoutput") \
        && echo "PASS" \
        || (echo "FAIL"; \
            exit 1)
}

# ======================================================================
check_output() {

    checkX "Test memory" "$1"

    testbin="$1"
    type="$2"

    ref='tests/files'
    testfile="${ref}/$3"
    [ -f "$testfile" ] || error "Expected test file \"$testfile\" not found."
    
    EXPECTED_OUTPUT="${8}"

    mytmp="$(new_tmp_file)"
    # gets stdout in case of success, stderr in case of error
    if ACTUAL_OUTPUT="$("$testbin" "$type" "$testfile" "$4" "$5" "$6" 2>"$mytmp")"; then
        ACTUAL_OUTPUT="$(echo "$ACTUAL_OUTPUT" | head -n $7 | while read -d "$5" c; do printf "\x$c"; done)"
    else
        ACTUAL_OUTPUT="$(cat "$mytmp")"
    fi

    diff -w <(echo "$ACTUAL_OUTPUT") <(echo -e "$EXPECTED_OUTPUT") \
        && echo "PASS" \
        || (echo "FAIL"; \
            echo -e "Expected:\n$EXPECTED_OUTPUT"; \
            echo -e "Actual:\n$ACTUAL_OUTPUT"; \
            exit 1)
}


# ======================================================================
# test test-memory on a few provided files
sep=' '
addr=0x0

printf "Test %1d (test-memory on dump  #1 addr 0x0): " $((++test))
check_output_with_file test-memory dump memory-dump-01.mem output/memory-01-out.txt o "$sep" $addr

printf "Test %1d (test-memory on desc. #1 addr $addr): " $((++test))
check_output_with_file test-memory desc memory-desc-01.txt output/memory-01-out.txt o "$sep" $addr

printf "Test %1d (test-memory on desc. #2 addr $addr): " $((++test))
check_output_with_file test-memory desc memory-desc-02.txt output/memory-02-out.txt o "$sep" $addr

addr=0x8000000000
printf "Test %1d (test-memory on desc. #2 addr $addr): " $((++test))
check_output_with_file test-memory desc memory-desc-02.txt output/memory-02-B-out.txt o "$sep" $addr

printf "Test %1d (is that a hit?): " $((++test))
check_output test-memory desc memory-desc-02.txt n "$sep" 0x1000 20 \
"#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h> // for PRIx macro

#include \"addr_mng.h\"
#include \"memory.h\"
#include \"page_walk.h\"
#include \"error.h\"

#define DESCFILE \"tests/files/memory-desc-02.txt\"
#defi"

# ======================================================================
echo "SUCCESS"

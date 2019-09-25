#!/bin/bash

## Basic tests for weeks 8 and 9

source $(dirname ${BASH_SOURCE[0]})/test_env.sh

test=0

# ======================================================================
# tool function
check_output_with_file() {

    checkX "Test Cache hierarchy" "$1"

    ref='tests/files'
    memfile="${ref}/$3"
    [ -f "$memfile" ] || error "Expected mem dump file \"$memfile\" not found."

    cmdfile="${ref}/$4"
    [ -f "$cmdfile" ] || error "Expected command file \"$cmdfile\" not found."

    refoutput="${ref}/$5"
    [ -f "$refoutput" ] || error "Expected output file \"$refoutput\" not found."
    
    mytmp="$(new_tmp_file)"
    # gets stdout in case of success, stderr in case of error
    ACTUAL_OUTPUT="$("$1" "$2" "$memfile" "$cmdfile" 2>"$mytmp" || cat "$mytmp")"

    diff -w <(echo "$ACTUAL_OUTPUT") <(cat "$refoutput") \
        && echo "PASS" \
        || (echo "FAIL"; \
            exit 1)
}

# ======================================================================
# test test-tlb_simple on a few provided files
printf "Test %1d (test-cache 1): " $((++test))
check_output_with_file test-cache dump memory-dump-01.mem commands01.txt output/cache-01-out.txt

# ======================================================================
echo "SUCCESS"

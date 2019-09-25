#!/bin/bash

set -eu         # -e: cause the script to exit immediately when a command fails;
                # -u: treat unset variables as an error and exit immediately

set -o pipefail # sets the exit code of a pipeline to that of the
                # rightmost command to exit with a non-zero status, or
                # zero if all commands of the pipeline exit
                # successfully

# adds the origin directory of that very script to the PATH
RWD="$(dirname "$(readlink -f "$0")")"
RWD=${RWD%/*}
export PATH="$PATH:$RWD"

# ======================================================================
# reports messages to stderr and exists
error () {
    echo $@ 1>&2
    exit 1
}

# ======================================================================
# checks if $2 is executable. $1 is an English 1-2 word(s) summary of $2.
# Example usage: checkX server pps_launch_server
checkX () {
    [ -x "$2" ] || error "There's no point testing if I don't have any $1 code (cannot found $2)."
}

# ======================================================================
reset_everything() {
    set +e
    pkill 'pps-*'
    set -e
}

# ======================================================================
init() {
    reset_everything

    # add here everything needed for each new test run
    
    echo ">> running $(basename ${0})"
}

# ======================================================================
trap cleanup EXIT
cleanup() {
    local status="$?"
    clean_tmp_files
    exit $status
}

# ----------------------------------------------------------------------
# TMP_FILES contains the name of a file containing names of temporary files used
readonly TMP_FILES="$(mktemp)"
# ======================================================================
new_tmp_file() {
    local file="$(mktemp)"

    echo "${file}" | tee -a "$TMP_FILES"
}

# ======================================================================
clean_tmp_files() {
    while read f
    do
	rm "$f"
    done < "$TMP_FILES"

    rm "$TMP_FILES"
}

# ======================================================================
# waits for a process, the PID of which is passed as $1, to be killed
wait_for_kill() {
    wait "${@}" 2>/dev/null || [ $? -eq 143 ]
}

# ======================================================================
# suppresses SIGPIPE error in case of an unterminated command still writing to a closed pipe
# (which shall be the expected behaviour; e.g.: suppress_sigpipe foo | head
# where foo is a never-ending writing command). See examples below.
suppress_sigpipe() {
	"${@}" || [ $? -eq 141 ]
}

# ======================================================================
# generates a random stream of characters not containing newline
random_stream() {
	xxd -ps /dev/urandom | tr -d '\n'
}

# ======================================================================
# random sequence of non-zero non-newline chars. $1 is the size; 20 by default
# example usage: random_string 25
random_string() {
	local size=${1:-20}

	suppress_sigpipe random_stream | suppress_sigpipe tr -d '\0' | head -c $size
}

# ======================================================================
## random sequence of non-newline chars. $1 is the size; 20 by default
# example usage: random_value 25
random_value() {
	local size=${1:-20}

	supress_sigpipe random_stream | head -c $size
}


# ======================================================================
init

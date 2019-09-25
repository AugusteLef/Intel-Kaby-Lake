#include "error.h"
#include "commands.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "please provide command filename to read from\n");
        return 1;
    }

    program_t pgm;
    if (program_read(argv[1], &pgm) == ERR_NONE) {
        (void)program_print(stdout, &pgm);
    }
    (void)program_free(&pgm);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include "disass.h"

void usage() {
    puts("disass <input file>");
}

int main(int argc, char *argv[]) {
    int ret;

    /*if (argc < 2) {
        usage();
        exit(0);
    }*/

	argv[1] = "C:\\Users\\Matan\\Desktop\\Wheel of Fortune (USA).nes";

    ret = disassembler_init(argv[1]);

    if (ret) {
        switch (ret) {
            case -EBADF:
                perror("Couldn't open file.");
                goto err;
            case -EINVAL:
                perror("Invalid file supplied - file does not contain a valid iNES header.");
                goto err;
            default:
                goto err;
        }
    }

    dump_header();
	dump_code();

    return EXIT_SUCCESS;

err:
    return EXIT_FAILURE;
}
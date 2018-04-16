#include <stdio.h>
#include <stdlib.h>

void usage() {
    puts("disass <input file>");
}

int main(int argc, char *argv[]) {
    FILE *input_file;

    if (argc < 2) {
        usage();
        exit(0);
    }

    input_file = fopen(argv[1], "rb");

    if (!input_file) {
        perror("Could not open the input file");
        exit(1);
    }



    return 0;
}
#include "emu.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
	char *fpath;
	fpath = "donkey kong.nes";
	emu_init(fpath);
}

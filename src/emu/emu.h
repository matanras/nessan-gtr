/*
 * The emulator.
 */

#pragma once

/**
 * Initialize and run the emulator.
 * @param fpath The file path to the game ROM.
 * @return 0 if initialization was successful, nonzero otherwise.
 */
int emu_init(char *fpath);
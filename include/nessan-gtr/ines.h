#pragma once

#define INES_HEADER_MAGIC "NES\x1A" /* iNes header magic. */
#define INES_HEADER_MAGIC_SIZE 4 /* iNes header magic length. */
#define INES_HEADER_SIZE        16 /* iNes header size. */

/*
* Magic number which indicates NES2.0 header extension.
*/
#define INES_HEADER_V2_EXTENSION 2


/* Mirroring types. */
#define MIRRORING_HORIZONTAL 0
#define MIRRORING_VERTICAL   1

/* Supported TV systems. */
#define TVSYS_NTSC 0
#define TVSYS_PAL  1

/* iNes represents PRG ROM size in units of 16KB. */
#define PRG_ROM_UNIT 16384
/* iNes represents CHR ROM size in units of 8KB. */
#define CHR_ROM_UNIT 8192

/* Size of trainer in bytes. */
#define TRAINER_SIZE 512

/*
* iNes header, with NES 2.0 extension.
*/
#pragma pack(push, 1)
#include <stdint.h>

struct ines2_header {
	uint8_t magic[4];
	uint8_t prg_size;
	uint8_t chr_size;
	/* Flags 6 */
	uint8_t mirroring_type : 1;
	uint8_t is_persistent_memory_present : 1;
	uint8_t is_trainer_present : 1;
	uint8_t ignore_mirror_control : 1;
	uint8_t padding1 : 4;
	/* Flags 7 */
	uint8_t vs_unisystem : 1;
	uint8_t playchoice : 1;
	uint8_t is_nes2 : 2;
	uint8_t padding2 : 4;
	uint8_t padding3;
	/* flags 9 */
	uint8_t tv_system : 1;
	uint8_t padding4 : 7;
	uint8_t extra[6];
};
#pragma pack(pop)
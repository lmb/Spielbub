#ifndef __ROM_H__
#define __ROM_H__

#include <stdint.h>

typedef struct {
	uint8_t bip[4];
	uint8_t graphic[48];
	uint8_t name[15]; // may not be 0 terminated
	uint8_t type;
	uint8_t license[2];
	uint8_t sgb;
	uint8_t cart_type;
	uint8_t rom_banks;
	uint8_t ram_type;
	uint8_t region;
	uint8_t license_old;
	uint8_t mask_rom;
	uint8_t complement;
	uint8_t checksum[2];
} rom_meta;

uint8_t* rom_load(rom_meta* meta, char* filename);

#endif//__ROM_H__

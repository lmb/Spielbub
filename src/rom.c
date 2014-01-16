#include "rom.h"

#include "logging.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define ROM_MIN_LEN (0x14f)
#define ROM_META    (0x100)

/*
 * Loads ROM meta data into a rom_meta structure.
 * The returned buffer has to be free()'d by the caller.
 */
uint8_t* rom_load(rom_meta* meta, char* filename)
{
	FILE* rom = fopen(filename, "rb");
	unsigned int req_mem;
	uint8_t *buffer;

	if (!rom)
		return NULL;

	if (fseek(rom, ROM_MIN_LEN, SEEK_SET) != 0 || feof(rom))
	{
		// File is not large enough to be a valid
		// GB rom
		fclose(rom);
		return NULL;
	}

	fseek(rom, ROM_META, SEEK_SET);
	if (fread(meta, sizeof(rom_meta), 1, rom) != 1)
	{
		log_dbg("Could not read ROM meta-data");
		fclose(rom);
		return NULL;
	}

	meta->rom_banks = (uint8_t)pow(2, meta->rom_banks + 1);
	log_dbg("Loading '%s', %d banks...\n", meta->name, meta->rom_banks);
    
    // TODO: Limit number of banks?

	// Each rom bank is 16KB
	req_mem = meta->rom_banks * 0x4000;
	buffer = malloc(req_mem);
	log_dbg("Allocating %d bytes of ROM buffer: %p\n", req_mem, buffer);

	rewind(rom);
	if (fread(buffer, 1, req_mem, rom) != req_mem)
	{
	    log_dbg("Could not read from file");
		fclose(rom);
		free(buffer);
		return NULL;
	}

	fclose(rom);
	return buffer;
}

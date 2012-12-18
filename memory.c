#include "memory.h"
#include "rom.h"
#include "ioregs.h"
#include "joypad.h"

#include "logging.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// bank = addr / 0x2000, offset = addr % 0x2000
#define ADDR_TO_BANK_OFFSET(x) int bank = (x) / 0x2000; \
    int offset = (x) % 0x2000

// Type 1
void mbc1_init();
void mbc1(memory_t*, int addr, uint8_t value);

// List of supported memory controllers.
struct {
    mem_ctrl_init_f init;
    mem_ctrl_f handler;
} memory_controllers[] = {
    {0, 0},             // No memory controller
    {mbc1_init, &mbc1}  // Memory Bank Controller 1
};

// Initialization table for the IO registers,
// located at 0xFF00+ in memory.
uint8_t const ioregs_init[0x4C] = {
    //  x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 0x
    0x80, 0xBF, 0xF3,    0, 0xBF,    0, 0x3F,    0,    0, 0xBF, 0x7F, 0xFF, 0x9F,    0, 0xBF,    0, // 1x
    0xFF,    0,    0, 0xBF, 0x77, 0xF3, 0xF1,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 2x
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 3x
    0x91,    0,    0,    0,    0,    0,    0, 0xFC, 0xFF, 0xFF,    0,    0                          // 4x
};

void mem_init(memory_t *mem)
{
    // TODO: Does this even set all items to NULL?
	memset(mem->map, 0, sizeof(mem->map));
	memset(mem->internal_ram, 0, sizeof(mem->internal_ram));
    memset(mem->video_ram, 0, sizeof(mem->video_ram));

	mem->map[4] = mem->video_ram;
//  mem->map[5] = swappable memory bank
	mem->map[6] = mem->internal_ram;
	mem->map[7] = mem->internal_ram + 0x2000;
    
    // Initialize IO registers.
    mem_write_ioregs(mem, ioregs_init);
    // Disable all interrupts.
    mem_write(mem, R_IE, 0);
}

void mem_init_debug(memory_t *mem)
{
    mem_init(mem);
    mem->rom = malloc(5 * 0x2000);
    
    mem->map[0] = mem->rom;
	mem->map[1] = mem->rom + 0x2000;
	mem->map[2] = mem->rom + 0x4000;
	mem->map[3] = mem->rom + 0x6000;
    mem->map[5] = mem->rom + 0x8000;
}

void mem_destroy(memory_t *mem)
{
	if (mem->rom != NULL)
		free(mem->rom);

    if (mem->ram != NULL)
        free(mem->ram);
}

bool mem_load_rom(memory_t *mem, char *filename)
{
    // TODO: Correct size?
	memset(&(mem->meta), 0, sizeof(mem->meta));
    
	if (mem->rom != NULL)
		free(mem->rom);

	mem->rom = rom_load(&(mem->meta), filename);

	if (mem->rom == NULL)
	{
		log_dbg("Loading failed.\n");
		return false;
	}

    assert(mem->meta.cart_type < 0x02);
    if (memory_controllers[mem->meta.cart_type].init != NULL)
    {
        memory_controllers[mem->meta.cart_type].init(mem);
        mem->controller = memory_controllers[mem->meta.cart_type].handler;
    }
    else
    {
        printf("No suitable memory controller found!\n");
        return false;
    }
    
    // TODO: Check minimum number of banks?

	mem->map[0] = mem->rom;
	mem->map[1] = mem->rom + 0x2000;
	mem->map[2] = mem->rom + 0x4000;
	mem->map[3] = mem->rom + 0x6000;
    
    return true;
}

uint8_t mem_read(const memory_t* mem, register int addr)
{
	ADDR_TO_BANK_OFFSET(addr);
    
    assert(mem->map[bank] != NULL);
    
	return mem->map[bank][offset];
}

uint8_t* mem_address(const memory_t* mem , int addr)
{
	ADDR_TO_BANK_OFFSET(addr);
    
    assert(mem->map[bank] != NULL);
    
	return mem->map[bank] + offset;
}

void mem_write(memory_t *mem, int addr, uint8_t value)
{
    uint8_t *src, *dst;
    ADDR_TO_BANK_OFFSET(addr);

    if (addr < 0x8000)
    {
        // This is ROM, forward to MBC
        if (mem->controller != NULL)
            mem->controller(mem, addr, value);
        // else: ignored, since there are ROM only
        // carts that simply do nothing at this point.
        return;
    }

    // Take care of special behaviour and
    // certain read-only registers.
    switch (addr)
    {
        case R_DIV:
            // Writing to the Divider Register resets it to zero,
            // regardless of value.
            mem->map[bank][offset] = 0;
            return;

        case R_DMA:
            // Do DMA transfer into OAM
            src = mem_address(mem, value * 0x100);
            dst = mem_address(mem, OAM_START);
            memcpy((void*)dst, src, 0xA0);
            return;

        default:
            // Put value into memory
            mem->map[bank][offset] = value;
            break;
    }

	// Shadow RAM
	if ((bank == 6 || bank == 7) && (offset < 0x1E00))
	{
		bank = (bank == 6) ? 7 : 6;

		mem->map[bank][offset] = value;
	}
}

/*
 * Initialize IO registers, read 0x4c bytes from data.
 */
void mem_write_ioregs(memory_t *mem, const uint8_t* data)
{
    assert(mem->map != NULL);
    memcpy(mem->map[7] + 0x1F00, data, 0x4C);
}

// __ Memory bank controllers __________________

void mbc1_init(memory_t *mem)
{
    assert(mem->mbc_state.mode == 0);
    
    // TODO: Check malloc
    mem->ram = malloc(0x2000);
    mem->map[5] = mem->ram;
}

void mbc1(memory_t *mem, int addr, uint8_t value)
{
    // Different actions are executed depending
    // on the ROM address that the program tries
    // to write to.
    if (addr >= 0x6000)
    {
        // Change addressing mode
        if (mem->mbc_state.mode != (value & 1))
        {
            if (mem->mbc_state.mode == 0)
            {
                // Switching to mode 1
                mem->mbc_state.upper_rom_bits = 0;
                mem->ram = realloc(mem->ram, 0x8000);
            }
            else
            {
                // Switching to mode 0
                mem->ram = realloc(mem->ram, 0x2000);
            }
            
            mem->mbc_state.mode = value & 1;
            // TODO: Does this honor previous RAM bank settings?
            mem->map[5] = mem->ram;
        }
    }
    else if (addr >= 0x4000)
    {
        // RAM bank switching
        if (mem->mbc_state.mode == 0)
        {
            mem->mbc_state.upper_rom_bits = (value & 0x3) << 5;
        }
        else
        {
            int bank = value & 0x3;
            mem->map[5] = mem->ram + (bank * 0x2000);
        }
    }
    else if (addr >= 0x2000)
    {
        // ROM bank switching
        int bank = (value & 0x1F);
        if (bank == 0) bank = 1;
        bank |= mem->mbc_state.upper_rom_bits;
        
        mem->map[2] = mem->rom + (bank * 0x4000);
        mem->map[3] = mem->rom + (bank * 0x4000) + 0x2000;
    }
}

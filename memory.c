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

void mbc1_init(memory_t *mem)
{
    // TODO: Check malloc
    mem->ram = malloc(0x2000);
    mem->map[5] = mem->ram;
}

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

	mem->map[0] = mem->rom;
	mem->map[1] = mem->rom + 0x2000;
	mem->map[2] = mem->rom + 0x4000;
	mem->map[3] = mem->rom + 0x6000;
    
    return true;
}

inline uint8_t* mem_address(memory_t *mem, register int addr)
{
	ADDR_TO_BANK_OFFSET(addr);

    // TODO: Check for null bank pointers
    
	return mem->map[bank] + offset;
}

inline uint8_t mem_read(const memory_t *mem, register int addr)
{
	ADDR_TO_BANK_OFFSET(addr);

    // TODO: Check for null bank pointers
    
	return mem->map[bank][offset];
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
        // TODO: Handle else case!
        return;
    }

    // Take care of special behaviour and
    // certain read-only registers.
    switch (addr)
    {
        case R_DIV:
            mem->map[bank][offset] = 0;
            return;

        case R_DMA:
            // Do DMA transfer into OAM
            src = mem_address(mem, value * 0x100);
            dst = mem_address(mem, OAM_START);
            memcpy((void*)dst, src, 0xA0);
            return;

        /*case R_JOYPAD:
            // TODO: Re-enable joypad
            //mem->map[bank][offset] = joypad_get_state(context, value);*/

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

void mbc1(memory_t *mem, int addr, uint8_t value)
{
    // 0 = 16/8; 1 = 4/32
    static int mode = 0;

    // Different actions are executed depending
    // on the ROM address that the program tries
    // to write to.
    if (addr >= 0x6000)
    {
        // Change addressing mode
        if (mode != (value & 1))
        {
            mode = value & 1;
            mem->ram  = realloc(mem->ram, mode ? 0x8000 : 0x2000);
        }
    }
    else if (0x4000 <= addr)
    {
        // RAM bank switching
        if (mode == 0)
        {
            // TODO: is this correct?
            mem->map[5] = mem->ram;
        }
        else
        {
            int bank  = value & 3;
            mem->map[5] = mem->ram + (bank * 0x2000);
        }
    }
    else if (0x2000 <= addr)
    {
        // ROM bank switching
        int bank = value & 31;
        mem->map[2] = mem->rom + (bank * 0x4000);
        mem->map[3] = mem->rom + (bank * 0x4000) + 0x2000;
    }
}

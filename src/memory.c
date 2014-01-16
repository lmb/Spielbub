

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "context.h"

#include "ioregs.h"
#include "logging.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Type 1
void mbc1_init();
void mbc1(memory_t*, int addr, uint8_t value);

static void mem_write_ioregs(memory_t*, const uint8_t* data);

// List of supported memory controllers.
static const struct {
    void (*init)(memory_t*);
    mem_ctrl_f handler;
} memory_controllers[] = {
    {0, 0},             // No memory controller
    {mbc1_init, &mbc1}  // Memory Bank Controller 1
};

// Initialization table for the IO registers,
// located at 0xFF00+ in memory.
static const uint8_t ioregs_init[0x4C] = {
    //  x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 0x
    0x80, 0xBF, 0xF3,    0, 0xBF,    0, 0x3F,    0,    0, 0xBF, 0x7F, 0xFF, 0x9F,    0, 0xBF,    0, // 1x
    0xFF,    0,    0, 0xBF, 0x77, 0xF3, 0xF1,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 2x
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 3x
    0x91,    0,    0,    0,    0,    0,    0, 0xFC, 0xFF, 0xFF,    0,    0                          // 4x
};

static bool mem_map(memory_t *mem, int bank, uint8_t *src)
{
    const uint16_t addr = bank * 0x2000 + (bank == 4 ? 0x2000 : 0);

    if (bank < 0 || bank > 4 || src == NULL) {
        return false;
    }

    if (mem->banks[bank] != NULL) {
        if (mem->banks[bank] == src) {
            return true;
        }

        memcpy(mem->banks[bank], &mem->map[addr], 0x2000);
    }

    memcpy(&mem->map[addr], src, 0x2000);
    mem->banks[bank] = src;

    return true;
}

static bool mem_map_many(memory_t *mem, const int bank, const size_t num,
    uint8_t* src)
{
    int i, j;

    for (i = bank, j = 0; i < bank + num; i++, j++) {
        if (!mem_map(mem, i, src + (j * 0x2000))) {
            return false;
        }
    }

    return true;
}

void mem_init(memory_t *mem)
{
    // TODO: Does this even set all items to NULL?
    memset(mem->map, 0, sizeof(mem->map));

    // mem->map[4] = mem->video_ram;
    // mem->map[5] = swappable memory bank
    // mem->map[6] = mem->internal_ram;
    // mem->map[7] = mem->internal_ram + 0x2000;
    
    // Initialize IO registers.
    mem_write_ioregs(mem, ioregs_init);
    // Disable all interrupts.
    mem->map[R_IE] = 0;
}

void mem_init_debug(memory_t *mem)
{
    mem_init(mem);
    mem->rom = malloc(5 * 0x2000);

    mem_map_many(mem, 0, 5, mem->rom);
}

void mem_destroy(memory_t *mem)
{
    if (mem->rom != NULL) {
        free(mem->rom);
    }
}

bool mem_load_rom(memory_t *mem, const char *filename)
{
    // TODO: Correct size?
    memset(&(mem->meta), 0, sizeof(mem->meta));
    
    if (mem->rom != NULL) {
        free(mem->rom);
    }

    mem->rom = rom_load(&(mem->meta), filename);

    if (mem->rom == NULL) {
        return false;
    }

    assert(mem->meta.cart_type < 0x02);

    if (memory_controllers[mem->meta.cart_type].init != NULL) {
        memory_controllers[mem->meta.cart_type].init(mem);
        mem->controller = memory_controllers[mem->meta.cart_type].handler;
    } else {
        return false;
    }
    
    // TODO: Check minimum number of banks?

    mem_map_many(mem, 0, 4, mem->rom);

    return true;
}

uint16_t mem_read16(const memory_t *mem, uint16_t addr)
{
    return mem->map[addr] | (mem->map[addr+1] << 8);
}

void mem_write16(memory_t *mem, uint16_t addr, uint16_t value)
{
    mem->map[addr]   = value & 0xff;
    mem->map[addr+1] = value >> 8;
}

void mem_write(memory_t *mem, const uint16_t addr, uint8_t value)
{
    if (addr < 0x8000) {
        // This is ROM, forward to MBC
        if (mem->controller != NULL)
            mem->controller(mem, addr, value);
        // else: ignored, since there are ROM only
        // carts that simply do nothing at this point.
        return;
    }

    // Take care of special behaviour and
    // certain read-only registers.
    switch (addr) {
        case R_DIV:
            // Writing to the Divider Register resets it to zero,
            // regardless of value.
            mem->map[addr] = 0;
            return;

        case R_DMA:
            // Do DMA transfer into OAM
            memcpy(&mem->map[OAM_START], &mem->map[value * 0x100], 0xA0);
            return;
    }

    // Put value into memory
    mem->map[addr] = value;

    // Shadow 0xC000-0xDDFF to 0xE000-0xFDFF
    if (0xC000 <= addr && addr <= 0xDDFF) {
        mem->map[addr + 0x2000] = value;
    } else if (0xE000 <= addr && addr <= 0xFDFF) {
        mem->map[addr - 0x2000] = value;
    }
}

/*
 * Initialize IO registers, read 0x4c bytes from data.
 */
void mem_write_ioregs(memory_t *mem, const uint8_t* data)
{
    assert(mem->map != NULL);
    memcpy(&mem->map[7] + 0x1F00, data, 0x4C);
}

// __ Memory bank controllers __________________

void mbc1_init(memory_t *mem)
{
    assert(mem->mbc.type1.mode == 0);
    mem_map(mem, 4, mem->mbc.type1.ram);
}

void mbc1(memory_t *mem, int addr, uint8_t value)
{
    // Different actions are executed depending
    // on the ROM address that the program tries
    // to write to.
    if (addr >= 0x6000)
    {
        // Change addressing mode
        if (mem->mbc.type1.mode != (value & 1))
        {
            mem->mbc.type1.upper_rom_bits = 0;
            mem->mbc.type1.mode = value & 1;

            mem_map(mem, 4, mem->mbc.type1.ram);
        }
    }
    else if (addr >= 0x4000)
    {
        if (mem->mbc.type1.mode == 0)
        {
            mem->mbc.type1.upper_rom_bits = (value & 0x3) << 5;
        }
        else
        {
            // RAM bank switching
            int bank = value & 0x3;
            mem_map(mem, 4, mem->mbc.type1.ram + (bank * 0x2000));
        }
    }
    else if (addr >= 0x2000)
    {
        // ROM bank switching
        int bank = (value & 0x1F);
        bank = MIN(bank, 1) | mem->mbc.type1.upper_rom_bits;

        mem_map_many(mem, 2, 2, mem->rom + (bank * 0x4000));
    }
}

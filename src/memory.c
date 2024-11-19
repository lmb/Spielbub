#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "context.h"

#include "ioregs.h"
#include "logging.h"

#define R_DIV (0xFF04)
#define R_LY  (0xFF44)
#define R_DMA (0xFF46)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Type 1
void mbc1_init(memory_t *mem);
void mbc1_init_battery(memory_t *mem);
void mbc1(memory_t*, int addr, uint8_t value);

// List of supported memory controllers.
static const struct {
    void (*init)(memory_t*);
    mem_ctrl_f handler;
} memory_controllers[] = {
    {0, 0},             // No memory controller
    {mbc1_init, &mbc1}, // Memory Bank Controller 1
    {mbc1_init, &mbc1}, // Memory Bank Controller 1 (RAM)
    {mbc1_init_battery, &mbc1}, // Memory Bank Controller 1 (Battery-backed RAM)
};

// Initialization table for the IO registers, located at 0xFF00+ in memory.
static const uint8_t ioregs_init[0x4C] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0xBF, 0xF3, 0x00, 0xBF, 0x00, 0x3F, 0x00, 0x00, 0xBF, 0x7F, 0xFF, 0x9F, 0x00, 0xBF, 0x00,
    0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0x00, 0x00
};

static bool change_mapping(memory_t* mem, size_t bank, void* src)
{
    const uint16_t addr = bank * 0x2000 + (bank == 4 ? 0x2000 : 0);

    if (bank > 4 || src == NULL) {
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

static bool change_mappings(memory_t *mem, size_t bank, size_t num, void* src)
{
    size_t i, j;

    for (i = bank, j = 0; i < bank + num; i++, j++) {
        if (!change_mapping(mem, i, (uint8_t*)src + (j * 0x2000))) {
            return false;
        }
    }

    return true;
}

void mem_init(memory_t *mem)
{
    memset(mem->map, 0, sizeof(mem->map));
    memset(mem->banks, 0, sizeof(mem->banks));

    // mem->map[4] = mem->video_ram;
    // mem->map[5] = swappable memory bank
    // mem->map[6] = mem->internal_ram;
    // mem->map[7] = mem->internal_ram + 0x2000;
    
    // Initialize IO registers.
    memcpy(&mem->map[7] + 0x1F00, ioregs_init, 0x4C);

    // Disable all interrupts.
    mem->io.IE = 0;
}

void mem_init_debug(memory_t *mem)
{
    mem_init(mem);
    mem->rom = malloc(5 * 0x2000);

    change_mappings(mem, 0, 4, mem->rom);
}

void mem_destroy(memory_t *mem)
{
    if (mem->rom != NULL) {
        free(mem->rom);
    }
}

bool mem_load_rom(memory_t *mem, const char *filename)
{
    memset(&(mem->meta), 0, sizeof(mem->meta));
    
    if (mem->rom != NULL) {
        free(mem->rom);
    }

    mem->rom = rom_load(&(mem->meta), filename);

    if (mem->rom == NULL) {
        return false;
    }

    assert(mem->meta.cart_type < sizeof(memory_controllers) / sizeof(memory_controllers[0]));

    if (memory_controllers[mem->meta.cart_type].init != NULL) {
        memory_controllers[mem->meta.cart_type].init(mem);
        mem->controller = memory_controllers[mem->meta.cart_type].handler;
    } else {
        return false;
    }
    
    // TODO: Check minimum number of banks?

    change_mappings(mem, 0, 4, mem->rom);

    return true;
}

uint16_t mem_read16(const memory_t *mem, uint16_t addr)
{
    return mem->map[addr] | (mem->map[addr+1] << 8);
}

void mem_write16(memory_t *mem, uint16_t addr, uint16_t value)
{
    mem_write(mem, addr, value & 0xff);
    mem_write(mem, addr+1, value >> 8);
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
        case R_LY:
            mem->io.LY = 0;
            return;

        case R_DIV:
            // Writing to the Divider Register resets it to zero,
            // regardless of value.
            mem->map[addr] = 0;
            return;

        case R_DMA:
            // Do DMA transfer into OAM
            memcpy(&mem->gfx.oam, &mem->map[value * 0x100], 0xA0);
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

// __ Memory bank controllers __________________

void mbc1_init(memory_t *mem)
{
    assert(mem->mbc.type1.mode == 0);
    change_mapping(mem, 4, mem->mbc.type1.ram);
}

void mbc1_init_battery(memory_t *mem)
{
    // TODO: What does the battery backing imply?
    mbc1_init(mem);
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

            change_mapping(mem, 4, mem->mbc.type1.ram);
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
            change_mapping(mem, 4, mem->mbc.type1.ram + (bank * 0x2000));
        }
    }
    else if (addr >= 0x2000)
    {
        // ROM bank switching
        int bank = (value & 0x1F);
        bank = MAX(bank, 1) | mem->mbc.type1.upper_rom_bits;

        change_mappings(mem, 2, 2, mem->rom + (bank * 0x4000));
    }
}

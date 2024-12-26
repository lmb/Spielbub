#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

#include "spielbub.h"
#include "bitfield.h"
#include "rom.h"
#include "sound.h"
#include "util.h"

#define SPRITE_SIZE (4)

typedef struct context context_t;
typedef struct memory memory_t;

typedef struct mbc {
    union {
        struct {
            // 0 = 16 MBit ROM/8 KByte RAM; 1 = 4 MBit ROM/32 KByte RAM
            int mode;
            
            // Used in mode 0
            int upper_rom_bits;

            uint8_t ram[0x8000];
        } type1;
    };
} mbc_t;

typedef struct memory_tile {
    uint8_t lines[TILE_HEIGHT][2];
} memory_tile_t;

typedef struct memory_tile_map {
    uint8_t data[MAP_ROWS][MAP_COLUMNS];
} memory_tile_map_t;

typedef struct memory_tile_data {
    memory_tile_t data[MAX_TILES];
} memory_tile_data_t;

typedef struct memory_oam {
    uint8_t data[SPRITE_SIZE];
} memory_oam_t;

typedef void (*mem_ctrl_f)(memory_t*, int, uint8_t);

typedef struct memory_io {
    uint8_t __pad0[0xFF00];
    uint8_t JOYPAD; // 0xFF00
    uint8_t __pad1[0x03];
    uint8_t DIV;    // 0xFF04
    uint8_t TIMA;
    uint8_t TMA;
    uint8_t TAC;
    uint8_t __pad2[0x07];
    uint8_t IF;     // 0xFF0F
    uint8_t __pad3[0x30];
    uint8_t LCDC;   // 0xFF40
    uint8_t STAT;
    uint8_t SCY;
    uint8_t SCX;
    uint8_t LY;
    uint8_t LYC;
    uint8_t DMA;
    uint8_t BGP;
    uint8_t SPP_LOW;
    uint8_t SPP_HIGH;
    uint8_t WY;
    uint8_t WX;
    uint8_t __pad4[0xB3];
    uint8_t IE;    // 0xFFFF
} memory_io_t;

typedef struct memory_gfx {
    uint8_t __pad0[0x8000];
    memory_tile_data_t tiles;
    memory_tile_map_t  tile_maps[2];
    uint8_t __pad1[0x5E00];
    memory_oam_t oam[MAX_SPRITES];
} memory_gfx_t;

typedef struct memory_sound {
    uint8_t __pad0[0xff10];
    
    union {
        uint8_t regs[23];
        struct {
            uint8_t NR10, NR11, NR12, NR13, NR14;
            uint8_t NR20, NR21, NR22, NR23, NR24;
            uint8_t NR30, NR31, NR32, NR33, NR34;
            uint8_t NR40, NR41, NR42, NR43, NR44;
            uint8_t NR50, NR51, NR52;
        };
        struct {
            sound_square_params_t square1;
            sound_square_params_t square2;

            struct {
                uint8_t BITFIELD(dac_on:1, :7);
                uint8_t length_load;
                uint8_t BITFIELD(:1, volume_code:2, :5);
                uint8_t period_lsb;
                uint8_t BITFIELD(trigger:1, length_enable:1, :3, period_msb:3);
            } wave;
            
            struct {
                uint8_t __pad0;
                uint8_t BITFIELD(:2, length_load:6);
                uint8_t BITFIELD(volume:4, envelope_mode:1, period:3);
                uint8_t BITFIELD(clock_shift:4, lfsr_width:1, divisor_code:3);
                uint8_t BITFIELD(trigger:1, length_enable:1, :6);
            } noise;
            
            uint8_t BITFIELD(vin_l_enable:1, volume_left:3, vin_r_enable:1, volume_right:3);
            uint8_t BITFIELD(left_enables:4, right_enables:4);
            uint8_t BITFIELD(power:1, :3, channel_statuses:4);
        };
    };
    uint8_t unused[0xff2f - 0xff27 + 1];
    uint8_t wave_table[0xff3f - 0xff30 + 1];
} memory_sound_t;

_Static_assert(sizeof((memory_sound_t*)0)->regs == 0xff27 - 0xff10, "Raw registers must have the correct size");
_Static_assert(offsetof(memory_sound_t, wave) == 0xff1a, "Wave registers must be at correct offset");
_Static_assert(offsetof(memory_sound_t, noise) == 0xff1f, "Noise registers must be at correct offset");
_Static_assert(offsetof(memory_sound_t, wave_table) == 0xff30, "Wave table must be at correct offset");
_Static_assert(offsetofend(memory_sound_t, wave_table) == 0xff40, "Wave table must end at correct offset");

_Static_assert(offsetof(memory_sound_t, NR10) == 0xff10, "NR10 must have correct offset");
_Static_assert(offsetof(memory_sound_t, NR52) == 0xff26, "NR52 must have correct offset");

struct memory {
    union {
        uint8_t map[8 * 0x2000];
        memory_io_t io;
        memory_gfx_t gfx;
        memory_sound_t sound;
    };

    // Current memory controller
    mem_ctrl_f controller;

    void* banks[5];
    uint8_t* rom;
    
    rom_meta meta;
    mbc_t mbc;
};

void mem_init(memory_t*);
void mem_init_debug(memory_t *mem);
void mem_destroy(memory_t*);

bool mem_load_rom(memory_t*, const char *filename);
uint8_t mem_read(const context_t *ctx, uint16_t addr);
uint16_t mem_read16(const context_t *ctx, uint16_t addr);
void mem_write16(context_t *ctx, uint16_t addr, uint16_t value);
void mem_write(context_t *ctx, uint16_t addr, uint8_t value);

#endif//__MEMORY_H__

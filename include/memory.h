#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

#include "spielbub.h"
#include "rom.h"

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

typedef void (*mem_ctrl_f)(memory_t*, int, uint8_t);

struct memory {
    // Current memory controller
    mem_ctrl_f controller;

    union {
        uint8_t  map[8 * 0x2000];
        struct {
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
        } io;
    };

    uint8_t* banks[5];
    uint8_t* rom;
    
    rom_meta meta;
    mbc_t mbc;
};

void mem_init(memory_t*);
void mem_init_debug(memory_t *mem);
void mem_destroy(memory_t*);

bool mem_load_rom(memory_t*, const char *filename);
uint16_t mem_read16(const memory_t *mem, uint16_t addr);
void mem_write16(memory_t *mem, uint16_t addr, uint16_t value);
void mem_write(memory_t*, uint16_t addr, uint8_t value);

#endif//__MEMORY_H__

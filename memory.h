#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

#include "context.h"
#include "rom.h"

/*
 * Memory Bank Controllers
 * The gameboy uses several different memory controllers.
 */
typedef void (*mem_ctrl_init_f)(memory_t*);          // Initializer
typedef void (*mem_ctrl_f)(memory_t*, int, uint8_t); // Emulator

struct memory_opaque_t {
    // Current memory controller
    mem_ctrl_f controller;
    
    // This is a memory map ranging from 0x0000-0xFFFF.
    // Each index in it occupies 0x2000 bytes.
    uint8_t* map[8];
    uint8_t  internal_ram[0x4000];
    uint8_t  video_ram[0x2000];
    uint8_t* ram;
    uint8_t* rom;
    
    rom_meta meta;
};

void mem_init(memory_t*);
void mem_destroy(memory_t*);

bool mem_load_rom(memory_t*, char *filename);
uint8_t mem_read(const memory_t* mem, register int addr);
uint8_t* mem_address(const memory_t* mem , int addr);
void mem_write(memory_t*, int addr, uint8_t value);
void mem_write_ioregs(memory_t*, const uint8_t* data);

#endif//__MEMORY_H__

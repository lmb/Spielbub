//
//  tests.c
//  spielbub
//
//  Created by Lorenz on 07.12.12.
//  Copyright (c) 2012 Lorenz. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <assert.h>

#include "context.h"
#include "cpu_ops.h"
#include "probability_list.h"

// Fixtures
context_t ctx;

void setup_cpu(void)
{
    context_init_minimal(&ctx);
}

/* -------------------------------------------------------------------------- */
// CPU

START_TEST (test_cpu_init)
{
    fail_unless(ctx.cpu.AF == 0x01B0, "AF not initialized to 0x01B0");
}
END_TEST

START_TEST (test_cpu_registers)
{
    cpu_t cpu;

    cpu.AF = 0xAAFF;
    cpu.HL = 0xBEEF;

    fail_unless(cpu.A == 0xAA);
    fail_unless(cpu.F == 0xFF);
    fail_unless(cpu.H == 0xBE);
    fail_unless(cpu.L == 0xEF);
}
END_TEST

START_TEST (test_cpu_stack)
{
    uint16_t old_sp = ctx.cpu.SP;

    _push(&ctx, 0x1234);

    fail_unless(ctx.cpu.SP == old_sp - 2);
    fail_unless(_pop(&ctx) == 0x1234);
}
END_TEST

START_TEST (test_cpu_add)
{
    cpu_t cpu;

    cpu.A = 0;
    cpu_set_n(&cpu, true);
    cpu_set_z(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _add(&cpu, 0xD, false);

    fail_unless(cpu.A == 0xD, "_add failed, A = %d", cpu.A);
    fail_unless(!cpu_get_z(&cpu), "_add set Z flag");
    fail_unless(!cpu_get_n(&cpu), "_add failed to reset N flag");
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _add(&cpu, 0x3, false);

    fail_unless(cpu.A == 0x10);
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(cpu_get_h(&cpu));

    _add(&cpu, 0xEF, false);

    fail_unless(cpu.A == 0xFF);
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _add(&cpu, 1, false);

    fail_unless(cpu.A == 0);
    fail_unless(cpu_get_c(&cpu));
    fail_unless(cpu_get_h(&cpu));
}
END_TEST

START_TEST (test_cpu_add16)
{
    cpu_t cpu;

    cpu.HL = 0;
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _add16(&cpu, 0x1001);

    fail_unless(cpu.HL == 0x1001);
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _add16(&cpu, 0xFF);

    fail_unless(cpu.HL == 0x1100);
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(cpu_get_h(&cpu));

    _add16(&cpu, 0xef00);

    fail_unless(cpu.HL == 0);
    fail_unless(cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));
}
END_TEST

START_TEST (test_cpu_sub)
{
    cpu_t cpu;

    cpu.A = 0xFF;
    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, false);
    cpu_set_c(&cpu, true);

    _sub(&cpu, 0xF, false);

    fail_unless(cpu.A == 0xF0, "_sub failed, A = %d", cpu.A);
    fail_unless(!cpu_get_z(&cpu), "_sub set Z flag");
    fail_unless(cpu_get_n(&cpu), "_sub failed to set N flag");
    fail_unless(!cpu_get_c(&cpu), "_sub set C flag");
    fail_unless(!cpu_get_h(&cpu));

    _sub(&cpu, 0xF, false);

    fail_unless(cpu.A == 0xE1);
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(cpu_get_h(&cpu));

    _sub(&cpu, 0xE1, false);

    fail_unless(cpu.A == 0, "_sub failed, A = %d", cpu.A);
    fail_unless(cpu_get_z(&cpu), "_sub failed to set Z flag");

    _sub(&cpu, 1, false);

    fail_unless(cpu.A == 0xFF, "_sub with underflow failed, A = %d", cpu.A);
    fail_unless(cpu_get_c(&cpu), "_sub failed to set C flag");
}
END_TEST

START_TEST (test_cpu_inc)
{
    cpu_t cpu;
    uint8_t reg = 0;

    cpu_set_n(&cpu, true);
    cpu_set_z(&cpu, true);
    cpu_set_h(&cpu, true);

    _inc(&cpu, &reg);

    fail_unless(reg == 1);
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    reg = 0x0F;
    _inc(&cpu, &reg);

    fail_unless(reg == 0x10);
    fail_unless(cpu_get_h(&cpu));

    reg = 0xFF;
    _inc(&cpu, &reg);

    fail_unless(reg == 0);
    fail_unless(cpu_get_z(&cpu));
}
END_TEST

START_TEST (test_cpu_dec)
{
    cpu_t cpu;
    uint8_t reg = 0xFF;

    cpu_set_n(&cpu, false);
    cpu_set_z(&cpu, true);
    cpu_set_h(&cpu, true);

    _dec(&cpu, &reg);

    fail_unless(reg == 0xFE);
    fail_unless(cpu_get_n(&cpu));
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    reg = 0;
    _dec(&cpu, &reg);

    fail_unless(reg == 0xFF);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(cpu_get_h(&cpu));
}
END_TEST

START_TEST (test_cpu_and)
{
    cpu_t cpu;
    cpu.A = 0x55;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, false);

    _and(&cpu, 0xF0);

    fail_unless(cpu.A == 0x50);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(cpu_get_h(&cpu));

    _and(&cpu, 0x00);

    fail_unless(cpu.A == 0);
    fail_unless(cpu_get_z(&cpu));
}
END_TEST

START_TEST (test_cpu_or)
{
    cpu_t cpu;
    cpu.A = 0x50;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _or(&cpu, 0x05);

    fail_unless(cpu.A == 0x55);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    cpu.A = 0;
    _or(&cpu, 0x00);

    fail_unless(cpu.A == 0);
    fail_unless(cpu_get_z(&cpu));
}
END_TEST

START_TEST (test_cpu_xor)
{
    cpu_t cpu;
    cpu.A = 0x55;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _xor(&cpu, 0xAA);

    fail_unless(cpu.A == 0xFF);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _xor(&cpu, 0xFF);

    fail_unless(cpu.A == 0);
    fail_unless(cpu_get_z(&cpu));
}
END_TEST

START_TEST (test_cpu_cp)
{
    cpu_t cpu;
    cpu.A = 0x22;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, false);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _cp(&cpu, 0x02);

    fail_unless(cpu.A == 0x22);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _cp(&cpu, 0x22);

    fail_unless(cpu_get_z(&cpu));
    fail_unless(!cpu_get_c(&cpu));

    _cp(&cpu, 0x23);

    fail_unless(!cpu_get_z(&cpu));
    fail_unless(cpu_get_c(&cpu));
}
END_TEST

START_TEST (test_cpu_swap)
{
    cpu_t cpu;
    uint8_t reg = 0x45;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _swap(&cpu, &reg);

    fail_unless(reg == 0x54);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    reg = 0;
    _swap(&cpu, &reg);

    fail_unless(cpu_get_z(&cpu));
}
END_TEST

START_TEST (test_cpu_rotate)
{
    cpu_t cpu;
    uint8_t reg;

    // Rotate left
    reg = 0xAA;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, false);
    cpu_set_h(&cpu, true);

    _rotate_l(&cpu, &reg);

    fail_unless(reg == 0x55);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    cpu_set_c(&cpu, false);
    reg = 0xAA;

    _rotate_l_carry(&cpu, &reg);

    fail_unless(reg == 0x54);
    fail_unless(cpu_get_c(&cpu));

    // Rotate right
    reg = 0x55;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, false);
    cpu_set_h(&cpu, true);

    _rotate_r(&cpu, &reg);

    fail_unless(reg == 0xAA);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    cpu_set_c(&cpu, false);
    reg = 0x55;

    _rotate_r_carry(&cpu, &reg);

    fail_unless(reg == 0x2A);
    fail_unless(cpu_get_c(&cpu));
}
END_TEST

START_TEST (test_cpu_shift)
{
    cpu_t cpu;
    uint8_t reg = 0xAA;

    // Shift left
    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, false);
    cpu_set_h(&cpu, true);

    _shift_l(&cpu, &reg);

    fail_unless(reg == 0x54);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    reg = 0;
    _shift_l(&cpu, &reg);

    fail_unless(cpu_get_z(&cpu));

    // Shift right (logical)
    reg = 0x55;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, false);
    cpu_set_h(&cpu, true);

    _shift_r_logic(&cpu, &reg);

    fail_unless(reg == 0x2A);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    reg = 0;
    _shift_r_logic(&cpu, &reg);

    fail_unless(cpu_get_z(&cpu));

    // Shift right (arithmetical)
    reg = 0xAA;

    cpu_set_z(&cpu, true);
    cpu_set_n(&cpu, true);
    cpu_set_c(&cpu, true);
    cpu_set_h(&cpu, true);

    _shift_r_arithm(&cpu, &reg);

    fail_unless(reg == 0xD5);
    fail_unless(!cpu_get_z(&cpu));
    fail_unless(!cpu_get_n(&cpu));
    fail_unless(!cpu_get_c(&cpu));
    fail_unless(!cpu_get_h(&cpu));

    _shift_r_arithm(&cpu, &reg);

    fail_unless(reg == 0xEA);
    fail_unless(cpu_get_c(&cpu));

    reg = 0;
    _shift_r_arithm(&cpu, &reg);

    fail_unless(reg == 0);
    fail_unless(cpu_get_z(&cpu));
}
END_TEST

void cpu_test_store(context_t *ctx, uint8_t opcode, uint8_t value);

START_TEST (test_cpu_ld)
{
    uint8_t opcode = _i;
    
    if (0x76 == opcode)
        return;
    
    // Run tests for LD (HL), <X> only with values > 0x80 and < 0xFF
    if (opcode < 0x70 || opcode >= 0x78) {
        cpu_test_store(&ctx, opcode, 0x00);
        cpu_test_store(&ctx, opcode, 0xFF);
        cpu_test_store(&ctx, opcode, 0x55);
    }

    cpu_test_store(&ctx, opcode, 0xAA);
    cpu_test_store(&ctx, opcode, 0xFE);
    cpu_test_store(&ctx, opcode, 0x81);
}
END_TEST

#undef CPU_RUN_TEST

/* -------------------------------------------------------------------------- */
// Graphics

// START_TEST (test_gfx_sprite_t)
// {
//     sprite_t sprite = { .raw = 0 };
    
//     // These tests currently fail on big-endian.
    
//     fail_unless(sizeof(sprite_t) == 4,
//         "The compiler seems to pad sprite_t to != 4 bytes");
    
//     sprite.y = 0xFF;
//     sprite.x = 0xEE;
//     sprite.tile_id = 0xDD;
//     sprite.flags = 0xCC;
    
//     const uint8_t check[] = { 0xFF, 0xEE, 0xDD, 0xCC };
    
//     fail_unless(memcmp((void*)&sprite, (void*)check, 4) == 0,
//         "sprite_t packing does not seem to work");
// }
// END_TEST

// START_TEST (test_gfx_sprite_table)
// {
//     sprite_table_t table;
//     table.length = 0;
//     memset(&(table.data), 0, sizeof(table.data));
    
//     const sprite_t to_sort[] = {
//         { .b = { .x = 0xA, .flags = 0xA}},
//         { .b = { .x = 0x9, .flags = 0x9}},
//         { .b = { .x = 0x8, .flags = 0x8}},
//         { .b = { .x = 0x7, .flags = 0x7}},
//         { .b = { .x = 0x6, .flags = 0x6}},
//         { .b = { .x = 0x5, .flags = 0x5}},
//         { .b = { .x = 0x5, .flags = 0x4}},
//         { .b = { .x = 0x4, .flags = 0x3}},
//         { .b = { .x = 0x3, .flags = 0x2}},
//         { .b = { .x = 0x2, .flags = 0x1}},
//         { .b = { .x = 0xF, .flags = 0xB}},
//         { .b = { .x = 0x8, .flags = 0xC}},
//         { .raw = 0 }
//     };
    
//     int i;
//     for (i = 0; to_sort[i].raw != 0; i++) {
//         _add_sprite_to_table(&table, to_sort[i]);
//     }
    
//     const uint8_t sorted[SPRITES_PER_LINE][4] = {
//         {0x00, 0x02, 0x00, 0x01},
//         {0x00, 0x03, 0x00, 0x02},
//         {0x00, 0x04, 0x00, 0x03},
//         {0x00, 0x05, 0x00, 0x05},
//         {0x00, 0x05, 0x00, 0x04},
//         {0x00, 0x06, 0x00, 0x06},
//         {0x00, 0x07, 0x00, 0x07},
//         {0x00, 0x08, 0x00, 0x08},
//         {0x00, 0x08, 0x00, 0x0C},
//         {0x00, 0x09, 0x00, 0x09},
//     };
    
//     fail_unless(sizeof(table.data) == sizeof(sorted),
//         "sprite_table_t is not of size %d but %d", sizeof(sorted),
//         sizeof(table.data));

//     fail_unless(memcmp(sorted, table.data, sizeof(sorted)) == 0,
//         "sprite_table_t is not sorted properly");
// }
// END_TEST

/* -------------------------------------------------------------------------- */
// Memory

START_TEST (test_mem_locations)
{
    fail_unless(offsetof(memory_t, io) == 0);
    fail_unless(offsetof(memory_io_t, JOYPAD) == 0xFF00);
    fail_unless(offsetof(memory_io_t, DIV) == 0xFF04);
    fail_unless(offsetof(memory_io_t, IF) == 0xFF0F);
    fail_unless(offsetof(memory_io_t, LCDC) == 0xFF40);
    fail_unless(offsetof(memory_io_t, IE) == 0xFFFF);

    fail_unless(offsetof(memory_t, gfx) == 0);
    fail_unless(offsetof(memory_gfx_t, tiles) == 0x8000);
    fail_unless(offsetof(memory_gfx_t, map_low) == 0x9800);
    fail_unless(offsetof(memory_gfx_t, map_high) == 0x9C00);
    fail_unless(offsetof(memory_gfx_t, oam) == 0xFE00);
}
END_TEST

/* -------------------------------------------------------------------------- */
// Probability list

START_TEST (test_prob_list)
{
    prob_list_t pl;
    
    pl_init(&pl);
    
    uint16_t const values[] = {
        0x0392, 0xAF78, 0x8923, 0xFEAA, 0x2939, 0xFFFF, 0
    };
    
    int i = 0;
    
    do {
        fail_unless(pl_add(&pl, values[i]), "Could not add 0x%X to pl, value number %d", values[i], i+1);
        i++;
    } while (values[i] != 0);
    
    for (int j = 0; j < PL_MAX_VALUES - i; j++) {
        fail_unless(pl_add(&pl, j), "Could not add 0x%X to pl, value number %d", j, j+i+1);
    }
    
    fail_unless(pl.length == PL_MAX_VALUES, "Pl is not at max capacity");
    
    fail_unless(pl_add(&pl, 0x0) == false, "Added entry after pl was supposedly full");
    
    i = 0;
    do {
        fail_unless(pl_check(&pl, values[i]), "Pl failed to recognize added value 0x%X", values[i]);
        i++;
    } while (values[i] != 0);
    
    fail_unless(pl_check(&pl, 0xDEAD) == false, "Pl recognized a value that was not added");
}
END_TEST

/* -------------------------------------------------------------------------- */

Suite * spielbub_suite(void)
{
    Suite *s = suite_create("Spielbub");
    
    // CPU test cases
    TCase *tc_cpu = tcase_create("CPU");
    tcase_add_checked_fixture(tc_cpu, setup_cpu, NULL);
    tcase_add_test(tc_cpu, test_cpu_init);
    tcase_add_test(tc_cpu, test_cpu_registers);
    tcase_add_test(tc_cpu, test_cpu_stack);
    tcase_add_test(tc_cpu, test_cpu_add);
    tcase_add_test(tc_cpu, test_cpu_add16);
    tcase_add_test(tc_cpu, test_cpu_sub);
    tcase_add_test(tc_cpu, test_cpu_inc);
    tcase_add_test(tc_cpu, test_cpu_dec);
    tcase_add_test(tc_cpu, test_cpu_and);
    tcase_add_test(tc_cpu, test_cpu_or);
    tcase_add_test(tc_cpu, test_cpu_xor);
    tcase_add_test(tc_cpu, test_cpu_cp);
    tcase_add_test(tc_cpu, test_cpu_swap);
    tcase_add_test(tc_cpu, test_cpu_rotate);
    tcase_add_test(tc_cpu, test_cpu_shift);
    tcase_add_loop_test(tc_cpu, test_cpu_ld, 0x40, 0x80);
    suite_add_tcase(s, tc_cpu);
    
    // Graphics
    // TCase *tc_graphics = tcase_create("Graphics");
    // tcase_add_test(tc_graphics, test_gfx_sprite_t);
    // tcase_add_test(tc_graphics, test_gfx_sprite_table);
    // suite_add_tcase(s, tc_graphics);

    // Memory
    TCase *tc_memory = tcase_create("Memory");
    tcase_add_test(tc_memory, test_mem_locations);
    suite_add_tcase(s, tc_memory);
    
    // Probability list
    TCase *tc_pl = tcase_create("Probability list");
    tcase_add_test(tc_pl, test_prob_list);
    suite_add_tcase(s, tc_pl);
    
    return s;
}

int main(void)
{
    int number_failed = 0;
    Suite *s = spielbub_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* -------------------------------------------------------------------------- */

static uint8_t* op_get_src(context_t *ctx, uint8_t opcode) {
    cpu_t *cpu = &ctx->cpu;
    
    assert(0x40 <= opcode && opcode < 0xC0);
    
    opcode %= 0x8;
    
    switch (opcode)
    {
        case 0x0: // B
            return &cpu->B;
        case 0x1: // C
            return &cpu->C;
        case 0x2: // D
            return &cpu->D;
        case 0x3: // E
            return &cpu->E;
        case 0x4: // H
            return &cpu->H;
        case 0x5: // L
            return &cpu->L;
        case 0x6: // addr in HL
            return &ctx->mem.map[cpu->HL];
        case 0x7: // A
            return &cpu->A;
            
        default:
            return NULL;
    }
}

static uint8_t* op_get_dst(context_t *ctx, uint8_t opcode) {
    cpu_t *cpu = &ctx->cpu;
    
    assert(0x40 <= opcode && opcode < 0xA0);
    
    opcode /= 0x8;
    opcode -= 0x8;
    
    switch (opcode) {
        case 0x0: // B
            return &cpu->B;
        case 0x1: // C
            return &cpu->C;
        case 0x2: // D
            return &cpu->D;
        case 0x3: // E
            return &cpu->E;
        case 0x4: // H
            return &cpu->H;
        case 0x5: // L
            return &cpu->L;
        case 0x6: // addr in HL
            return &ctx->mem.map[cpu->HL];
        case 0x7: // A
            return &cpu->A;
            
        default:
            return NULL;
    }
}

void cpu_setup_test(context_t *ctx, uint8_t opcode)
{
    // This resets all registers.
    cpu_init(&ctx->cpu);
    
    ctx->mem.map[ctx->cpu.PC] = opcode;
    
    // Point HL into "safe" ram:
    // < 0x8000 goes to memory controller;
    // >= 0xFF00 writes to various ioregs,
    // which will cause problems.
    ctx->cpu.HL = 0x8001;
}

void cpu_test_store(context_t *ctx, uint8_t opcode, uint8_t value)
{
    cpu_setup_test(ctx, opcode);
    
    uint8_t *src = op_get_src(ctx, opcode), *dst;
    
    *src = value;
    dst = op_get_dst(ctx, opcode);

    cpu_run(ctx);

    fail_unless(*dst == *src, "Opcode 0x%02X: failed with src = %p, dst = %p",
        opcode, *src, *dst);
}


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
#include "cpu.h"
#include "cpu_ops.h"
#include "graphics.h"
#include "hardware.h"
#include "memory.h"
#include "probability_list.h"

// Fixtures
context_t context;
context_t *ctx;

void setup_cpu(void)
{
    ctx = &context;
    context_create(ctx);
    cpu_init(ctx->cpu);
    mem_init_debug(ctx->mem);
}

void teardown_cpu(void)
{
    context_destroy(ctx);
    ctx = NULL;
}

// CONTEXT
START_TEST (test_context_create)
{
    context_t ctx;
    fail_unless(context_create(&ctx), "context_create failed");
    context_destroy(&ctx);
}
END_TEST

START_TEST (test_context_init)
{
    context_t ctx;
    fail_unless(context_init(&ctx), "context_init failed");
    context_destroy(&ctx);
}
END_TEST

// CPU
START_TEST (test_cpu_init)
{
    cpu_t *cpu = ctx->cpu;
    
    fail_unless(cpu->AF.W == 0x01B0, "AF not initialized to 0x01B0");
}
END_TEST

START_TEST (test_cpu_add)
{
    _A = 0;
    _add(ctx, 13, false);
    
    fail_unless(_A == 13, "_add failed, _A = %d", _A);
    fail_unless(GET_Z() == 0, "_add set Z flag");
    fail_unless(GET_N() == 0, "_add failed to reset N flag");
    // TODO: H set if carry from bit 3
    // TODO: C set if carry from bit 7
    
    _add(ctx, 244, false);
    fail_unless(_A == 1, "_add with overflow failed, _A = %d", _A);
    fail_unless(GET_C() == 1, "_add failed to set C flag");
    // TODO: GET_H()
}
END_TEST

START_TEST (test_cpu_sub)
{
    _A = 100;
    _sub(ctx, 10, false);
    
    fail_unless(_A == 90, "_sub failed, _A = %d", _A);
    fail_unless(GET_Z() == 0, "_sub set Z flag");
    fail_unless(GET_N() == 1, "_sub failed to set N flag");
    fail_unless(GET_C() == 0, "_sub set C flag");
    
    _sub(ctx, 90, false);
    fail_unless(_A == 0, "_sub failed, _A = %d", _A);
    fail_unless(GET_Z() == 1, "_sub failed to set Z flag");
    
    _sub(ctx, 1, false);
    fail_unless(_A == 255, "_sub with underflow failed, _A = %d", _A);
    fail_unless(GET_C() == 1, "_sub failed to set C flag");
    
//    fail_unless(_A == 13, "_add failed, _A = %d", _A);
//    fail_unless(GET_Z() == 0, "_add set Z flag");
//    fail_unless(GET_N() == 0, "_add failed to reset N flag");
//    // TODO: H set if carry from bit 3
//    // TODO: C set if carry from bit 7
//    
//    _add(ctx, 244, false);
//    fail_unless(_A == 1, "_add with overflow failed, _A = %d", _A);
//    fail_unless(GET_C() == 1, "_add failed to set C flag");
//    // TODO: GET_H()
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
        cpu_test_store(ctx, opcode, 0x00);
        cpu_test_store(ctx, opcode, 0xFF);
        cpu_test_store(ctx, opcode, 0x55);
    }

    cpu_test_store(ctx, opcode, 0xAA);
    cpu_test_store(ctx, opcode, 0xFE);
    cpu_test_store(ctx, opcode, 0x81);
}
END_TEST

#undef CPU_RUN_TEST

// Graphics
START_TEST (test_gfx_sprite_t)
{
    sprite_t sprite = { .raw = 0};
    
    // These tests currently fail on big-endian.
    
    fail_unless(sizeof(sprite_t) == 4, "The compiler seems to pad sprite_t to != 4 bytes");
    
    sprite.y = 0xFF;
    sprite.x = 0xEE;
    sprite.tile_id = 0xDD;
    sprite.flags = 0xCC;
    
    const uint8_t check[] = { 0xFF, 0xEE, 0xDD, 0xCC };
    
    fail_unless(memcmp((void*)&sprite, (void*)check, 4) == 0, "sprite_t packing does not seem to work");
}
END_TEST


extern void _add_sprite_to_table(sprite_table_t *table, sprite_t sprite);
START_TEST (test_gfx_sprite_table)
{
    sprite_table_t table;
    table.length = 0;
    memset(&(table.data), 0, sizeof(table.data));
    
    sprite_t const to_sort[] = {
        { .b = { .x = 0xA, .flags = 0xA}},
        { .b = { .x = 0x9, .flags = 0x9}},
        { .b = { .x = 0x8, .flags = 0x8}},
        { .b = { .x = 0x7, .flags = 0x7}},
        { .b = { .x = 0x6, .flags = 0x6}},
        { .b = { .x = 0x5, .flags = 0x5}},
        { .b = { .x = 0x5, .flags = 0x4}},
        { .b = { .x = 0x4, .flags = 0x3}},
        { .b = { .x = 0x3, .flags = 0x2}},
        { .b = { .x = 0x2, .flags = 0x1}},
        { .b = { .x = 0xF, .flags = 0xB}},
        { .b = { .x = 0x8, .flags = 0xC}},
        { .raw = 0 }
    };
    
    int i;
    for (i = 0; to_sort[i].raw != 0; i++) {
        _add_sprite_to_table(&table, to_sort[i]);
    }
    
    const uint8_t sorted[SPRITES_PER_LINE][4] = {
        {0x00, 0x02, 0x00, 0x01},
        {0x00, 0x03, 0x00, 0x02},
        {0x00, 0x04, 0x00, 0x03},
        {0x00, 0x05, 0x00, 0x05},
        {0x00, 0x05, 0x00, 0x04},
        {0x00, 0x06, 0x00, 0x06},
        {0x00, 0x07, 0x00, 0x07},
        {0x00, 0x08, 0x00, 0x08},
        {0x00, 0x08, 0x00, 0x0C},
        {0x00, 0x09, 0x00, 0x09},
    };
    
    fail_unless(sizeof(table.data) == sizeof(sorted), "sprite_table_t is not of size %d but %d", sizeof(sorted), sizeof(table.data));
    fail_unless(memcmp(sorted, table.data, sizeof(sorted)) == 0, "sprite_table_t is not sorted properly");
}
END_TEST

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

Suite * spielbub_suite(void)
{
    Suite *s = suite_create("Spielbub");
    
    // Context test cases
    TCase *tc_context = tcase_create("Context");
    tcase_add_test(tc_context, test_context_create);
    //tcase_add_test(tc_context, test_context_init);
    suite_add_tcase(s, tc_context);
    
    // CPU test cases
    TCase *tc_cpu = tcase_create("CPU");
    tcase_add_checked_fixture(tc_cpu, setup_cpu, teardown_cpu);
    tcase_add_test(tc_cpu, test_cpu_init);
    tcase_add_test(tc_cpu, test_cpu_add);
    tcase_add_test(tc_cpu, test_cpu_sub);
    tcase_add_loop_test(tc_cpu, test_cpu_ld, 0x40, 0x80);
    suite_add_tcase(s, tc_cpu);
    
    // Graphics
    TCase *tc_graphics = tcase_create("Graphics");
    tcase_add_test(tc_graphics, test_gfx_sprite_t);
    tcase_add_test(tc_graphics, test_gfx_sprite_table);
    suite_add_tcase(s, tc_graphics);
    
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

uint8_t* op_get_src(context_t *ctx, uint8_t opcode) {
    cpu_t *cpu = ctx->cpu;
    
    assert(0x40 <= opcode && opcode < 0xC0);
    
    opcode %= 0x8;
    
    switch (opcode)
    {
        case 0x0: // B
            return &(cpu->BC.B.h);
        case 0x1: // C
            return &(cpu->BC.B.l);
        case 0x2: // D
            return &(cpu->DE.B.h);
        case 0x3: // E
            return &(cpu->DE.B.l);
        case 0x4: // H
            return &(cpu->HL.B.h);
        case 0x5: // L
            return &(cpu->HL.B.l);
        case 0x6: // addr in HL
            return mem_address(ctx->mem, cpu->HL.W);
        case 0x7: // A
            return &(cpu->AF.B.h);
            
        default:
            return NULL;
    }
}

uint8_t* op_get_dst(context_t *ctx, uint8_t opcode) {
    cpu_t *cpu = ctx->cpu;
    
    assert(0x40 <= opcode && opcode < 0xA0);
    
    opcode /= 0x8;
    opcode -= 0x8;
    
    switch (opcode) {
        case 0x0: // B
            return &(cpu->BC.B.h);
        case 0x1: // C
            return &(cpu->BC.B.l);
        case 0x2: // D
            return &(cpu->DE.B.h);
        case 0x3: // E
            return &(cpu->DE.B.l);
        case 0x4: // H
            return &(cpu->HL.B.h);
        case 0x5: // L
            return &(cpu->HL.B.l);
        case 0x6: // addr in HL
            return mem_address(ctx->mem, cpu->HL.W);
        case 0x7: // A
            return &(cpu->AF.B.h);
            
        default:
            return NULL;
    }
}

void cpu_setup_test(context_t *ctx, uint8_t opcode)
{
    // This resets all registers.
    cpu_init(ctx->cpu);
    
    ctx->mem->map[0][ctx->cpu->PC.W] = opcode;
    
    // Point HL into "safe" ram:
    // < 0x8000 goes to memory controller;
    // >= 0xFF00 writes to various ioregs,
    // which will cause problems.
    ctx->cpu->HL.W = 0x8001;

}

void cpu_test_store(context_t *ctx, uint8_t opcode, uint8_t value)
{
    cpu_setup_test(ctx, opcode);
    
    uint8_t *src = op_get_src(ctx, opcode), *dst;
    
    *src = value;
    dst = op_get_dst(ctx, opcode);
    
    cpu_run(ctx);
    
    fail_unless(*dst == *src, "Opcode 0x%X: failed with src = 0x%X, dst = 0x%X", opcode, *src, *dst);
}


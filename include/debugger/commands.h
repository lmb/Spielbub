#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "debugger/debug.h"

bool execute_command(const char* command, context_t* ctx, debug_t* dbg);

#endif//__COMMANDS_H__

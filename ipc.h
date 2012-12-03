//
//  ipc.h
//  spielbub
//
//  Created by Lorenz on 03.12.12.
//  Copyright (c) 2012 Lorenz. All rights reserved.
//

#ifndef __IPC_H__
#define __IPC_H__

#include <stdbool.h>

#include "context.h"

#define IPC_VER_MAJOR 0
#define IPC_VER_MINOR 1

struct ipc_opaque_t
{
    void *context;
    void *service;
};

bool ipc_init(ipc_t *ipc);
void ipc_destroy(ipc_t *ipc);
void ipc_update(ipc_t *ipc);

#endif//__IPC_H__

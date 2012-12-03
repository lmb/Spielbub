//
//  ipc.c
//  spielbub
//
//  Created by Lorenz on 03.12.12.
//  Copyright (c) 2012 Lorenz. All rights reserved.
//

#include "ipc.h"
#include "debug.pb-c.h"

#define ENDPOINT "tcp://127.0.0.1:19479"

#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

bool ipc_init(ipc_t *ipc)
{
    ipc->context = zmq_ctx_new();
    
    if (ipc->context == NULL)
    {
        free(ipc);
        return false;
    }
    
    ipc->service = zmq_socket(ipc->context, ZMQ_REP);
    
    if (ipc->service == NULL)
    {
        free(ipc);
        return false;
    }
    
    if (zmq_bind(ipc->service, ENDPOINT) < 0)
    {
        free(ipc);
        return false;
    }
    
    printf("Listening on %s\n", ENDPOINT);
    return true;
}

void ipc_update(ipc_t *ipc)
{
    zmq_msg_t msg;
    
    assert(zmq_msg_init(&msg) == 0);
    
    int size = zmq_msg_recv(&msg, ipc->service, ZMQ_NOBLOCK);
    if (size < 0)
    {
        if (errno == EAGAIN) {
            // We're dandy, there's just nothing on the wire.
            zmq_msg_close(&msg);
            return;
        }
        
        // This means trouble. Do something about it.
        printf("zmq_recv() failed: %s\n", zmq_strerror(errno));
        zmq_msg_close(&msg);
        return;
    }
    
    // We've got a message!
    uint8_t *buf = malloc(size);
    assert(buf != NULL);
    
    memcpy(buf, zmq_msg_data(&msg), size);
    
    D__Request *request;
    request = d__request__unpack(NULL, size, buf);
    
    if (request == NULL) {
        printf("ipc_update(): received invalid packet\n");
        // TODO: Send error response?
    } else {
        D__Response response = D__RESPONSE__INIT;
        
        switch (request->type) {
            case D__TYPE__VERSION:
                ;; // I don't get it
                D__Response__Version version = D__RESPONSE__VERSION__INIT;
                
                version.major = IPC_VER_MAJOR;
                version.minor = IPC_VER_MINOR;
                response.version = &version;
                response.type = version.type;
                
                size = d__response__get_packed_size(&response);
                zmq_msg_init_size(&msg, size);
                
                d__response__pack(&response, zmq_msg_data(&msg));
                // TODO: Make this non-blocking?
                zmq_msg_send(&msg, ipc->service, 0);
                break;
                
            default:
                // TODO: Respond with error
                break;
        }
        
        d__request__free_unpacked(request, NULL);
    }
    
    free(buf);
    zmq_msg_close(&msg);
}

void ipc_destroy(ipc_t *ipc)
{
    if (ipc)
    {
        if (ipc->service)
        {
            zmq_close(ipc->service);
        }
        
        if (ipc->context)
        {
            zmq_ctx_destroy(ipc->context);
        }
    }
}

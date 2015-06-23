//
//  hook_kern_api.c
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//
#include <kern/kern_types.h>
#include <mach/mach_types.h>
#include <sys/errno.h>
#include <libkern/libkern.h>
#include <sys/lock.h>

#include "hook_kern_api.h"
#include "kernelInfo.h"

errno_t replace_osmalloc_func(mach_vm_address_t func_addr,
                              OSMalloc_handler* my_os_malloc, OSMalloc_handler *old_os_malloc)
{
    LOG(LOG_ERROR, "in");
    if(func_addr<(vm_offset_t)0xFFFFFF8000000000UL)
        return EINVAL;

    CloseInterupt();

    OSMalloc_handler *osmalloc = (OSMalloc_handler*)func_addr;
    //save old osmalloc
    bcopy(osmalloc, old_os_malloc, sizeof(OSMalloc_handler));
    LOG(LOG_ERROR, "old_os_malloc=%p", old_os_malloc);
    //write new osmalloc
    LOG(LOG_ERROR, "my_os_malloc=%p", my_os_malloc);
    bcopy(my_os_malloc, (OSMalloc_handler*)func_addr, sizeof(OSMalloc_handler));

    //__asm__("int3");

    RecorverInterupt();

    return KERN_SUCCESS;
}
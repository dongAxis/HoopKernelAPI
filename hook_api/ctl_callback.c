//
//  ctl_callback.c
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//
#include <sys/types.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <sys/lock.h>
#include <mach/vm_param.h>
#include <libkern/OSMalloc.h>
#include <sys/malloc.h>

#include "ctl_callback.h"
#include "hook_kern_api.h"
#include "debugInfo.h"
//#include "hook_api.h"

extern  lck_grp_t    *lck_group_lock;

static uint32_t     hook_enable;
extern  lck_rw_t     *hook_enable_rw_lock;

static OSMalloc_handler *older_osmalloc_handler=NULL;
extern lck_mtx_t *osmalloc_lock;

errno_t enable_api_hook()
{
    lck_rw_lock_exclusive(hook_enable_rw_lock);
    hook_enable=1;
    lck_rw_unlock_exclusive(hook_enable_rw_lock);

    return KERN_SUCCESS;
}

errno_t disable_api_hook()
{
    lck_rw_lock_exclusive(hook_enable_rw_lock);
    hook_enable=0;
    lck_rw_unlock_exclusive(hook_enable_rw_lock);
    return KERN_SUCCESS;
}

static void* my_osmalloc(uint32_t size, OSMallocTag tag)
{
    LOG(LOG_ERROR, "in my_osmalloc");
    return (*older_osmalloc_handler)(size, tag);
}

errno_t hook_os_malloc(mach_vm_address_t func_addr)
{
    lck_mtx_lock(osmalloc_lock);

    older_osmalloc_handler = (OSMalloc_handler*)_MALLOC(sizeof(OSMalloc_handler), M_TEMP, M_WAITOK);
    replace_osmalloc_func(func_addr, my_osmalloc, older_osmalloc_handler);

    lck_mtx_unlock(osmalloc_lock);
    return KERN_SUCCESS;
}

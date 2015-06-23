//
//  hook_api.c
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#include <mach/mach_types.h>

#include "kernelInfo.h"
#include "configure.h"
#include "control.h"
#include "hook_kern_api.h"
#include "hook_api.h"

kern_return_t hook_api_start(kmod_info_t * ki, void *d);
kern_return_t hook_api_stop(kmod_info_t *ki, void *d);

extern OSMalloc_handler *older_osmalloc_handler;

errno_t alloc_locks()
{
    lck_group_lock = lck_grp_alloc_init(TM_COMPONENT_NAME, LCK_GRP_ATTR_NULL);
    if(lck_group_lock==NULL)
    {
#if DEBUG_MODE
        LOG(LOG_ERROR, "alloc lock group failed");
#endif
        return ENOMEM;
    }

    hook_enable_rw_lock = lck_rw_alloc_init(lck_group_lock, LCK_ATTR_NULL);
    if(hook_enable_rw_lock==NULL)
    {
        lck_grp_free(lck_group_lock);
#if DEBUG_MODE
        LOG(LOG_ERROR, "alloc rw-lock failed");
#endif
        return ENOMEM;
    }

    osmalloc_lock = lck_mtx_alloc_init(lck_group_lock, LCK_ATTR_NULL);
    if(osmalloc_lock==NULL)
    {
        lck_grp_free(lck_group_lock);
#if DEBUG_MODE
        LOG(LOG_ERROR, "alloc rw-lock failed");
#endif
        return ENOMEM;
    }
    return KERN_SUCCESS;
}

kern_return_t destroy_locks()
{
    if(osmalloc_lock)
    {
        lck_mtx_free(osmalloc_lock, lck_group_lock);
    }

    if(hook_enable_rw_lock)
    {
        lck_rw_free(hook_enable_rw_lock, lck_group_lock);
    }

    if(lck_group_lock)
    {
        lck_grp_free(lck_group_lock);
    }

    return KERN_SUCCESS;
}

kern_return_t hook_api_start(kmod_info_t * ki, void *d)
{
    kernel_addr = getKernelHeader();

    mach_vm_address_t os_malloc_addr = get_osmalloc_addr(kernel_addr);

    LOG(LOG_ERROR, "$$$$$$os_malloc_addr=%llu", os_malloc_addr);

    errno_t error_code = alloc_locks();
    if(error_code!=KERN_SUCCESS)
        goto failed;

    error_code = init_rhinos_components();
    if(error_code!=KERN_SUCCESS)
    {
#if DEBUG_MODE
        LOG(LOG_ERROR, "init device files failed");
#endif
        return KERN_FAILURE;
    }

    return KERN_SUCCESS;
failed:
    destroy_locks();
    return KERN_FAILURE;
}

kern_return_t hook_api_stop(kmod_info_t *ki, void *d)
{
    errno_t error_code = destroy_rhinos_component();
    if(error_code!=KERN_SUCCESS)
    {
#if DEBUG_MODE
        LOG(LOG_ERROR, "destroy device files failed");
#endif
        return KERN_FAILURE;
    }

    destroy_locks();

    return KERN_SUCCESS;
}

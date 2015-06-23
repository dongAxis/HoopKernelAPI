//
//  hook_api.h
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#ifndef hook_api_hook_api_h
#define hook_api_hook_api_h
#include <mach/mach_types.h>
#include <sys/lock.h>

lck_grp_t    *lck_group_lock=NULL;
lck_rw_t     *hook_enable_rw_lock=NULL;
lck_mtx_t *osmalloc_lock;
mach_vm_address_t kernel_addr;

#endif

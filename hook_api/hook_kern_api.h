//
//  hook_kern_api.h
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#ifndef __hook_api__hook_kern_api__
#define __hook_api__hook_kern_api__
#include <mach/vm_param.h>
#include <libkern/OSMalloc.h>

typedef void* (OSMalloc_handler)(uint32_t size, OSMallocTag	tag);

errno_t replace_osmalloc_func(mach_vm_address_t func_addr,
                              OSMalloc_handler my_os_malloc, OSMalloc_handler *old_os_malloc);

#endif /* defined(__hook_api__hook_kern_api__) */

//
//  ctl_callback.h
//  hook_api
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#ifndef __hook_api__ctl_callback__
#define __hook_api__ctl_callback__

errno_t enable_api_hook();
errno_t disable_api_hook();
errno_t hook_os_malloc(mach_vm_address_t func_addr);

#endif /* defined(__hook_api__ctl_callback__) */

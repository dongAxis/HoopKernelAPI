//
//  configure.h
//  Troy
//
//  Created by Axis on 15/5/6.
//  Copyright (c) 2015年 Axis. All rights reserved.
//

#ifndef Troy_configure_h
#define Troy_configure_h

#include <sys/ioctl.h>
#include <sys/queue.h>
#include <mach-o/loader.h>

#pragma pack(8)
typedef struct _log_obj
{
    time_t time;
    mach_vm_address_t addr;
    mach_vm_size_t size;
}log_obj;
#pragma pack()

#define TM_COMPONENT_NAME "com.trendmicro.hook_api"
//TAILQ_HEAD(troy_array_entry, _troy_hide_object);    //this is the array

#pragma mark - DEVICE CMD
#define MAGIC_NUM 'a'
#define RHINOS_ENABLE_HOOK      _IOWR(MAGIC_NUM, 1, int)
#define RHINOS_DISABLE_HOOK     _IOWR(MAGIC_NUM, 2, int)
#define RHINOS_GET_LOG          _IOWR(MAGIC_NUM, 3, log_obj)
#define RHINOS_SET_FUNC_ADDR    _IOWR(MAGIC_NUM, 4, mach_vm_address_t)
#define RHINOS_GET_KERNEL_ADDR  _IOWR(MAGIC_NUM, 5, mach_vm_address_t)

#pragma mark - ERROR CODE
#define TROY_SUCCESS 1
#define TROY_ERROR_BASIC 0
#define TROY_ERROR_NOT_MATCH                    TROY_ERROR_BASIC-1
#define TROY_ERROR_INVALID_PARAMETER            TROY_ERROR_BASIC-2
#define TROY_ERROR_NOMEM                        TROY_ERROR_BASIC-3
#define TROY_ERROR_DATA_LENGTH_INVALID          TROY_ERROR_BASIC-4
#define TROY_ERROR_HIDE_ARRAY_IS_EMPRY          TROY_ERROR_BASIC-5

#pragma mark - character device name & absolute path
#define TROY_DEV_NAME "rhinos_dev"
#define TROY_ABSOLUTE_DEV_PATH "/dev/"TROY_DEV_NAME

#pragma mark - CONFIGURE
#define DEBUG_MODE 1

#pragma mark - align-level

#pragma mark - 
#define SAFE_FREE(ptr)     \
        do {               \
            _FREE((ptr), M_TEMP);    \
            (ptr)=NULL;     \
        }while(0)

#endif

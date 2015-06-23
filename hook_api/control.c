//
//  Control.c
//  Troy
//
//  Created by Axis on 5/7/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#include <string.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <miscfs/devfs/devfs.h>

#include "control.h"
#include "configure.h"
#include "debugInfo.h"
#include "ctl_callback.h"
#include "hook_kern_api.h"

extern mach_vm_address_t kernel_addr;

errno_t init_rhinos_components();
int rhinos_open_fn(dev_t dev, int flags, int devtype, struct proc *p);
int rhinos_close_fn(dev_t dev, int flags, int devtype, struct proc *p);
int rhinos_ioctl_fn(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p);

static int g_dev_major=-1;
static void *g_fs_mac_dev=NULL;

struct cdevsw rhinos_fops= {
    .d_open=rhinos_open_fn,		// open_close_fcn_t
    .d_close=rhinos_close_fn,		// open_close_fcn_t
    .d_read=nullread,           // read_write_fcn_t
    .d_write=nullwrite,         // read_write_fcn_t
    .d_ioctl=rhinos_ioctl_fn,		// ioctl_fcn_t
    .d_stop=nullstop,           // stop_fcn_t
    .d_reset=nullreset,         // reset_fcn_t
    .d_ttys=0,                  // struct tty
    .d_select=eno_select,		// select_fcn_t
    .d_mmap=eno_mmap,           // mmap_fcn_t
    .d_strategy=eno_strat,		// strategy_fcn_t
    .d_reserved_1=eno_getc,		// getc_fcn_t
    .d_reserved_2=eno_putc,		// putc_fcn_t
    .d_type=D_TTY,              // int
};

int rhinos_open_fn(dev_t dev, int flags, int devtype, struct proc *p)
{
    LOG(LOG_ERROR, "start _fn");
    return 0;
}

int rhinos_close_fn(dev_t dev, int flags, int devtype, struct proc *p)
{
    LOG(LOG_ERROR, "stop _fn");
    return 0;
}

int  rhinos_ioctl_fn(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
    switch (cmd) {
        case RHINOS_ENABLE_HOOK:
        {
            enable_api_hook();
        }break;
        case RHINOS_DISABLE_HOOK:
        {
            disable_api_hook();
        }break;
        case RHINOS_GET_LOG:
        {

        }break;
        case RHINOS_SET_FUNC_ADDR:
        {
            mach_vm_address_t addr = (mach_vm_address_t)data;
            LOG(LOG_ERROR, "[hahaha]addr=%llx", addr);
            hook_os_malloc(addr);
        }break;
        case RHINOS_GET_KERNEL_ADDR:
        {
            mach_vm_address_t* kernel = (mach_vm_address_t*)data;
            *kernel = kernel_addr;
            LOG(LOG_ERROR, "kernel_addr=%llx", kernel_addr);
        }
        default:
        {

        }
    }
    return 0;
}

kern_return_t init_rhinos_components()
{
    LOG(LOG_DEBUG, "Enter");
    g_dev_major = cdevsw_add(-1, &rhinos_fops);   //add file operation to the device file
    if(g_dev_major==-1)
    {
        LOG(LOG_ERROR, "create device file falied, g_dev_major=%d", g_dev_major);
        return KERN_FAILURE;
    }

    g_fs_mac_dev=devfs_make_node(makedev(g_dev_major, 0), DEVFS_CHAR, UID_ROOT,
                                 GID_KMEM, 0640, TROY_DEV_NAME);
    if(g_fs_mac_dev==NULL)
    {
        LOG(LOG_ERROR, "Failed to call devfs_make_node");
        return KERN_FAILURE;
    }
    LOG(LOG_DEBUG, "Leave");

    return KERN_SUCCESS;
}

kern_return_t destroy_rhinos_component()
{
    LOG(LOG_DEBUG, "Enter");
    int return_code = KERN_SUCCESS;

    if(g_fs_mac_dev!=NULL)
    {
        devfs_remove(g_fs_mac_dev);
        g_fs_mac_dev=NULL;
    }

    if(g_dev_major!=-1)
    {
        return_code=cdevsw_remove(g_dev_major, &rhinos_fops);
        if(return_code!=g_dev_major)
        {
            LOG(LOG_ERROR, "remove device failed, return code is %d", return_code);
            return_code = KERN_FAILURE;
        }
        else
        {
            return_code = KERN_SUCCESS;
        }
    }
    return return_code;
}
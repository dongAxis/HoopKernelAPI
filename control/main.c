//
//  main.c
//  control
//
//  Created by Axis on 6/16/15.
//  Copyright (c) 2015 Axis. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "debugInfo.h"
#include "configure.h"

mach_vm_address_t get_kernel_vm_addr()
{
    uint8_t *head_buf = (uint8_t*)malloc((sizeof(struct mach_header_64)+1)*sizeof(uint8_t));
    uint8_t *seg_buf = (uint8_t*)malloc((sizeof(struct segment_command_64)+1)*sizeof(uint8_t));
    bzero(head_buf, sizeof(struct mach_header_64)+1);
    bzero(seg_buf, (sizeof(struct segment_command_64)+1)*sizeof(uint8_t));

    int fd = open("/System/Library/Kernels/kernel", O_RDONLY);
    if(fd<0)
    {
        free(head_buf);
        free(seg_buf);
        printf("open file error");
        return 0;
    }

    ssize_t nreadbytes = read(fd, head_buf, sizeof(struct mach_header_64));
    if(nreadbytes!=sizeof(struct mach_header_64))
    {
        free(head_buf);
        free(seg_buf);
        close(fd);
        return 0;
    }
    struct mach_header_64 *header_64 = (struct mach_header_64*)head_buf;
    if(header_64->magic!=MH_MAGIC_64)
    {
        free(head_buf);
        free(seg_buf);
        close(fd);
        return 0;
    }

    nreadbytes = read(fd, seg_buf, sizeof(struct segment_command_64));
    if(nreadbytes!=sizeof(struct segment_command_64))
    {
        free(head_buf);
        free(seg_buf);
        close(fd);
        return 0;
    }

    struct segment_command_64 *seg_64 = (struct segment_command_64*)seg_buf;
    mach_vm_address_t kernel_vm_addr = seg_64->vmaddr-seg_64->fileoff;

    free(head_buf);
    free(seg_buf);
    close(fd);
    return kernel_vm_addr;
}

int connect_kext()
{
    int fd = open(TROY_ABSOLUTE_DEV_PATH, O_RDWR, 0666);
    if(fd<0)
    {
        LOG(LOG_ERROR, "connect failed");
        return 0;
    }
    return fd;
}

void disconnect_kext(int fd)
{
    close(fd);
}

mach_vm_address_t step_macho_get_symbol_seg(void* macho_addr)
{
    struct mach_header_64 *header_64=NULL;
    header_64=(struct mach_header_64*)macho_addr;
    printf("header_64->magic=%x", header_64->magic);
    if(header_64->magic!=MH_MAGIC_64)
    {
        printf("magic dose not right");
        return 0;
    }

    mach_vm_address_t *addr = (mach_vm_address_t*)((uint8_t*)header_64+sizeof(struct mach_header_64));
    struct symtab_command *symtab_seg=NULL;
    struct dysymtab_command *dysymtab_seg=NULL;
    struct segment_command_64 *linkedit_seg=NULL;

    for(int i=0; i<header_64->ncmds; i++)
    {
        struct segment_command_64 *seg_64=(struct segment_command_64*)addr;
        if(seg_64->cmd==LC_SYMTAB)
        {
            symtab_seg=(struct symtab_command *)addr;
        }
        else if(seg_64->cmd==LC_DYSYMTAB)
        {
            dysymtab_seg=(struct dysymtab_command *)addr;
        }
        else if(seg_64->cmd==LC_SEGMENT_64 && strcmp(seg_64->segname, SEG_LINKEDIT)==0)
        {
            linkedit_seg=(struct segment_command_64*)addr;
        }
        addr=(mach_vm_address_t*)((uint8_t*)addr+seg_64->cmdsize);
    }

    if(!symtab_seg || !dysymtab_seg || !linkedit_seg) return 0;

    mach_vm_address_t *base = (mach_vm_address_t*)((uint8_t*)header_64+linkedit_seg->fileoff);
    mach_vm_address_t* linkedit_base = (mach_vm_address_t*)((uint8_t*)base-linkedit_seg->fileoff);
    struct nlist_64 *symtab = (struct nlist_64*)((uint8_t*)linkedit_base+symtab_seg->symoff);
    uint32_t nsyms = symtab_seg->nsyms;
    char *strtab = (char*)((uint8_t*)linkedit_base+symtab_seg->stroff);

    int is_found=0;
    uint32_t i;
    for(i=0; i<nsyms; i++)
    {
        char* func_str = (char*)strtab+symtab[i].n_un.n_strx;
        if(strcmp(&func_str[1], "OSMalloc")==0)
        {
            is_found=1;
            break;
        }
    }

    if(is_found==0) return 0;

    struct nlist_64 osmalloc_sym_tab=symtab[i];
    printf("%s\n", strtab+osmalloc_sym_tab.n_un.n_strx);
    printf("%llx\n", osmalloc_sym_tab.n_value);

    return osmalloc_sym_tab.n_value;
}

mach_vm_address_t analysis_kernel_debug()
{
    int fd = open("./kernel.debug", O_RDWR, 777);
    if(fd<0)
    {
        printf("open ./kernel.debug error");
        return 0;
    }

    struct stat stat_buf;
    if(stat("./kernel.debug", &stat_buf)<0)
    {
        close(fd);
        printf("get stat of ./kernel.debug failed");
        return 0;
    }

    void *map_addr;
    map_addr = (void*)mmap(NULL, stat_buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(map_addr==MAP_FAILED)
    {
        printf("111");
        close(fd);
        return 0;
    }
    close(fd);

    mach_vm_address_t symbol_seg_addr = step_macho_get_symbol_seg(map_addr);

    if(map_addr)
        munmap(map_addr, stat_buf.st_size);

    return symbol_seg_addr;
}

int main(int argc, const char * argv[]) {

    mach_vm_address_t vm_addr_macho = get_kernel_vm_addr();

    //todo: connect kext
//    int fd = connect_kext();
//
//    mach_vm_address_t kernel_addr;
//    if(ioctl(fd, RHINOS_GET_KERNEL_ADDR, &kernel_addr)<0)
//    {
//        LOG(LOG_ERROR, "failed");
//        disconnect_kext(fd);
//        return -1;
//    }
//
//    LOG(LOG_ERROR, "kernel address in memory: %llx", kernel_addr);
//
//    mach_vm_address_t aslr = kernel_addr-vm_addr_macho;
    mach_vm_address_t os_malloc_vm_addr = analysis_kernel_debug();
//    mach_vm_address_t os_malloc_in_memory=os_malloc_vm_addr+aslr;
//
//    LOG(LOG_ERROR, "os_malloc_in_memory=%llx", os_malloc_in_memory);
//    //set the OSMalloc address to kernel
//    if(ioctl(fd, RHINOS_SET_FUNC_ADDR, &os_malloc_in_memory)<0)
//    {
//        LOG(LOG_ERROR, "RHINOS_SET_FUNC_ADDR failed");
//        disconnect_kext(fd);
//        return -1;
//    }
//    disconnect_kext(fd);

    return 0;
}

//
//  KernelInfo.c
//  Troy
//
//  Created by Axis on 15/5/6.
//  Copyright (c) 2015年 Axis. All rights reserved.
//
//#include <sys/malloc.h>
#include <mach/mach_types.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/types.h>
#include <sys/kernel_types.h>
#include <sys/systm.h>
#include <sys/unistd.h>

#include "kernelInfo.h"
#include "debugInfo.h"

static mach_vm_address_t GetInteruptTableAddr()
{
    LOG(LOG_DEBUG, "enter");
    uint8_t idtr[10];
    bzero(idtr, sizeof(uint8_t)*10);
    __asm__ __volatile__("sidt %0":"=m"(idtr));
    
    LOG(LOG_DEBUG, "leave");
    return *(mach_vm_address_t*)(&idtr[2]);
}

static uint16_t GetInteruptTableSize()
{
    uint8_t idtr[10];
    bzero(idtr, sizeof(uint8_t)*10);
    __asm__ __volatile__("side %0":"=m"(idtr));
    
    return *(uint16_t*)(&idtr[0]);
}

static mach_vm_address_t GetInt80Address()
{
    LOG(LOG_DEBUG, "enter");
    mach_vm_address_t idtr_addr = GetInteruptTableAddr();   //get the interput table's start address
    mach_vm_address_t idtr_80_addr = idtr_addr+sizeof(struct idt_descriptor)*0x80;  //get the int 80 start address
    struct idt_descriptor *idtr_int80_ptr = (struct idt_descriptor*)idtr_80_addr;
    
    uint64_t hight_addr = (uint64_t)idtr_int80_ptr->hight<<32;
    uint32_t middle_addr = (uint32_t)idtr_int80_ptr->middle<<16;
    uint64_t int80_vm_addr = hight_addr + (uint64_t)middle_addr + (uint64_t)idtr_int80_ptr->low;
    
    LOG(LOG_DEBUG, "leave");
    return (mach_vm_address_t)int80_vm_addr;
}

mach_vm_address_t getKernelHeader()
{
    mach_vm_address_t kernel_tmp_address = GetInt80Address();
    
    while(kernel_tmp_address>0)
    {
        struct mach_header_64 *header = (struct mach_header_64*)kernel_tmp_address;
        if(header->magic==MH_MAGIC_64)
        {
            struct segment_command_64 *text_segment = (struct segment_command_64 *)++header;
            if(strcmp(text_segment->segname, SEG_TEXT)==0)
            {
                return kernel_tmp_address;
            }
        }
        kernel_tmp_address--;
    }
    
    return 0;
}

mach_vm_address_t get_osmalloc_addr(mach_vm_address_t macho_addr)
{
    struct mach_header_64 *header_64=NULL;
    header_64=(struct mach_header_64*)macho_addr;
    printf("star address is %llx, header_64->magic=%x", macho_addr, header_64->magic);
    if(header_64->magic!=MH_MAGIC_64)
    {
        LOG(LOG_DEBUG, "magic dose not right");
        return 0;
    }
    LOG(LOG_DEBUG,"right");

    mach_vm_address_t *addr = (mach_vm_address_t*)((uint8_t*)header_64+sizeof(struct mach_header_64));
    struct symtab_command *symtab_seg=NULL;
    struct segment_command_64 *linkedit_seg=NULL;

    for(int i=0; i<header_64->ncmds; i++)
    {
        struct segment_command_64 *seg_64=(struct segment_command_64*)addr;
        if(seg_64->cmd==LC_SYMTAB)
        {
            symtab_seg=(struct symtab_command *)addr;
        }
        else if(seg_64->cmd==LC_SEGMENT_64 && strcmp(seg_64->segname, SEG_LINKEDIT)==0)
        {
            linkedit_seg=(struct segment_command_64*)addr;
        }
        addr=(mach_vm_address_t*)((uint8_t*)addr+seg_64->cmdsize);
    }

    if(!symtab_seg || !linkedit_seg) return 0;

    mach_vm_address_t* linkedit_base = (mach_vm_address_t*)(linkedit_seg->vmaddr-linkedit_seg->fileoff);
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
    printf("@@%d\n", i);
    printf("%s\n", strtab+osmalloc_sym_tab.n_un.n_strx);
    printf("%llx\n", osmalloc_sym_tab.n_value);

    return osmalloc_sym_tab.n_value;
}

mach_vm_address_t GetTextEntryAddr()
{
    mach_vm_address_t kernel_address = getKernelHeader();
    if(kernel_address==0) return 0;
    struct mach_header_64 *header = (struct mach_header_64*)kernel_address;
    uint32_t ncmds = header->ncmds;
    mach_vm_address_t segment_start_address=(mach_vm_address_t)++header;    //skip the header, and get the segment data
    struct segment_command_64 *segment_64=NULL;

    for(int i=0; i<ncmds; i++)
    {
        segment_64 =(struct segment_command_64*)(segment_start_address);
        if(strcmp(segment_64->segname,SEG_TEXT)==0)
        {
            LOG(LOG_ERROR, "cmd size %d", segment_64->cmdsize);
            LOG(LOG_ERROR, "start=%llu, vmsize=%llu", segment_64->vmaddr, segment_64->vmsize);
            LOG(LOG_ERROR, "offset=%llu, size=%llu", segment_64->fileoff, segment_64->filesize);

            mach_vm_address_t *sec_addr=
                    (mach_vm_address_t*)((uint8_t*)segment_64+sizeof(struct segment_command_64));
            for(int j=0; j<segment_64->nsects; j++)
            {
                struct section_64 *sec_64=(struct section_64*)sec_addr;
                if(strcmp(sec_64->sectname, SECT_TEXT)==0)
                {
                    printf("section name is %s, vm=%llx, size=%lld", sec_64->sectname, sec_64->addr, sec_64->size);
                }
                sec_addr=(mach_vm_address_t*)((uint8_t*)sec_addr+sec_64->size);
            }

            return (mach_vm_address_t)segment_64;
        }
        segment_start_address+=segment_64->cmdsize;
    }
    
    return 0;
}

/*uint64_t GetSystemCallAddress(uint64_t *start_address, uint64_t *vm_size)
{
    mach_vm_address_t system_call_address = GetSystmEntryAddr();
    if(system_call_address==0) return -1;

    struct segment_command_64 *data_segment = (struct segment_command_64*)system_call_address;
//    LOG(LOG_ERROR, "segname  is %d", data_segment->cmd);
//    LOG(LOG_ERROR, "segname is %s", data_segment->segname);
    *start_address=data_segment->vmaddr;
    *vm_size = data_segment->vmsize;
//    LOG(LOG_ERROR, "cmd size %d", data_segment->cmdsize);
//    LOG(LOG_ERROR, "start=%llu, vmsize=%llu", data_segment->vmaddr, data_segment->vmsize);
//    LOG(LOG_ERROR, "offset=%llu, size=%llu", data_segment->fileoff, data_segment->filesize);
    return 0;
}*/

void CloseInterupt()
{
    __asm__ __volatile__ ("cli");
}

void RecorverInterupt()
{
    __asm__ __volatile__ ("sti");
}
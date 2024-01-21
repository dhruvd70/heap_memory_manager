#include <stdio.h>
#include <memory.h>
#include <unistd.h>     //to get page size
#include <sys/mman.h>   //to use mmap()

#include "mem_manager.h"

static vm_page_for_families_t *first_vm_page_for_families = NULL;
static size_t SYSTEM_PAGE_SIZE = 0;

void m_mamp_init(void)
{
    SYSTEM_PAGE_SIZE = getpagesize();
}

/*Funtion to request VM Page from kernel*/
static void* m_map_get_vm_page_from_kernel(int units)
{
    char *vm_page = mmap(0, (units * SYSTEM_PAGE_SIZE), 
                             PROT_READ|PROT_WRITE|PROT_EXEC,
                             MAP_ANONYMOUS|MAP_PRIVATE,
                             0, 0);
    
    if(vm_page == MAP_FAILED) {
        printf("ERROR: VM Page Allocation Failed!\n");
        return NULL;
    }
    memset(vm_page, 0, (units * SYSTEM_PAGE_SIZE));
    printf("VM Page Map successful\n");
    return (void *)vm_page;
} 

/*Funtion to Return VM Page to Kernel*/
static void m_map_return_vm_page_to_kernel(void *vm_page, int units)
{
    if(munmap(vm_page, (units * SYSTEM_PAGE_SIZE))) {
        printf("ERROR: Could Not Unmap VM Page to Kernel\n");
    }   
    else{
        printf("VM Page Unmap Successful\n");
    }
}

void m_map_instansiate_new_page_family(char *struct_name, uint32_t struct_size)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *new_vm_page_for_familes = NULL;

    if(struct_size > SYSTEM_PAGE_SIZE) {
        printf("ERROR: %s() Structure %s Size exceeds System Page Size", 
                                                __FUNCTION__, struct_name);
        return;
    }

    if(!first_vm_page_for_families) {
        first_vm_page_for_families = 
                    (vm_page_for_families_t *)m_map_get_vm_page_from_kernel(1);
        first_vm_page_for_families->pNext = NULL;
        strncpy(first_vm_page_for_families->vm_page_family[0].struct_name, 
                struct_name, MAX_STRUCT_NAME_SIZE);
            
        first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
        return;
    }

    uint32_t count = 0;
    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
        if(strncmp(vm_page_family_curr->struct_name, struct_name, MAX_STRUCT_NAME_SIZE != 0)) {
            count ++;
            continue;
        }
        assert(0);
    }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);

    if(count == MAX_FAMILIES_PER_VM_PAGE) {
        new_vm_page_for_familes = 
                    (vm_page_for_families_t *)m_map_get_vm_page_from_kernel(1);
        new_vm_page_for_familes->pNext = first_vm_page_for_families;
        first_vm_page_for_families = new_vm_page_for_familes;
        vm_page_family_curr = &first_vm_page_for_families->vm_page_family[0];
    }

    strncpy(vm_page_family_curr->struct_name, struct_name, MAX_STRUCT_NAME_SIZE);
    vm_page_family_curr->struct_size = struct_size;
    // vm_page_family_curr->
}
void mm_print_registered_page_families(void)
{
    vm_page_family_t *vm_page_family_curr = NULL;

    if(!first_vm_page_for_families) {
        printf("ERROR: No Page Families Registered!\n");
        return;
    }

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
       printf("Page Family: %s, Size: %d\n",vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);
    }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);
}
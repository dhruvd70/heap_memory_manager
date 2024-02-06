#include <stdio.h>
#include <memory.h>
#include <unistd.h>     //to get page size
#include <sys/mman.h>   //to use mmap()

#include "mem_manager.h"

static vm_page_for_families_t *first_vm_page_for_families = NULL;
static size_t SYSTEM_PAGE_SIZE = 0;

void m_map_init(void)
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

static void m_map_merge_free_blocks(block_meta_data_t *first, block_meta_data_t *second)
{
    assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);

    first->block_size += sizeof(block_meta_data_t) + second->block_size;

    if(second->p_next_block) {
        second->p_next_block->p_prev_block = first;
    }
}

static inline uint32_t m_map_max_page_allocatable_memory(int units)
{
    return (uint32_t)((SYSTEM_PAGE_SIZE * units) - OFFSET_OF(vm_page_t, page_mem));
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
        strncpy(first_vm_page_for_families->vm_page_family[0].struct_name,struct_name, MAX_STRUCT_NAME_SIZE);

        first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
        first_vm_page_for_families->vm_page_family[0].first_page = NULL;
        init_glthread(&first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head);
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
    vm_page_family_curr->first_page = NULL;
    init_glthread(&first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head);
}

void m_map_print_registered_page_families(void)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    if(!first_vm_page_for_families) {
        printf("ERROR: No Page Families Registered!\n");
        return;
    }
    for(vm_page_for_families_curr = first_vm_page_for_families;
        vm_page_for_families_curr;
        vm_page_for_families_curr = vm_page_for_families_curr->pNext){
            ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
            printf("Page Family: %s, Size: %d\n",vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);
            }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);
    }
}

vm_page_family_t* lookup_page_family_by_name(char *struct_name)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    if(!first_vm_page_for_families) {
        printf("ERROR: No Page Families Registered!\n");
        return NULL;
    }
    for(vm_page_for_families_curr = first_vm_page_for_families;
        vm_page_for_families_curr;
        vm_page_for_families_curr = vm_page_for_families_curr->pNext){
            ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
                if(strncmp(vm_page_family_curr->struct_name, struct_name, MAX_STRUCT_NAME_SIZE) == 0) {
                    return vm_page_family_curr;
                }
            }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);
    }
    printf("ERROR: No Page Families called %s exist.\n",struct_name);
    return NULL;
}

vm_bool_e m_map_is_vm_page_empty(vm_page_t *vm_page)
{
    if( vm_page->block_meta_data.p_next_block == NULL &&
        vm_page->block_meta_data.p_prev_block == NULL &&
        vm_page->block_meta_data.is_free == MM_TRUE) {

        return MM_TRUE;
    }
    return MM_FALSE;
}

vm_page_t* allocate_vm_page(vm_page_family_t *vm_page_family)
{
    vm_page_t *vm_page = m_map_get_vm_page_from_kernel(1);

    MARK_VM_PAGE_EMPTY(vm_page);
    vm_page->block_meta_data.block_size = m_map_max_page_allocatable_memory(1);
    vm_page->block_meta_data.offset = OFFSET_OF(vm_page_t, block_meta_data);
    init_glthread(&vm_page->block_meta_data.priority_list_node);

    vm_page->p_next_page = NULL;
    vm_page->p_prev_page = NULL;

    /*setting the back pointer to page family*/
    vm_page->p_page_family = vm_page_family;

    /*inserting page if no other pages exist*/
    if(!vm_page_family->first_page) {
        vm_page_family->first_page = vm_page;
        return vm_page;
    }

    /*if other pages exist, new page is added at head*/
    vm_page->p_next_page = vm_page_family->first_page;
    vm_page_family->first_page->p_prev_page = vm_page;
    vm_page_family->first_page = vm_page;

    return vm_page;
}

void deallocate_vm_page(vm_page_t *vm_page)
{
    vm_page_family_t *vm_page_family = vm_page->p_page_family;

    /*page is Head of DLL*/
    if(vm_page_family->first_page == vm_page) {
        vm_page_family->first_page = vm_page->p_next_page;
        if(vm_page->p_next_page) {
            vm_page->p_next_page->p_prev_page = NULL;
        }
        vm_page->p_next_page = NULL;
        vm_page->p_prev_page = NULL;

        m_map_return_vm_page_to_kernel((void *)vm_page, 1);
        return;
    }

    if(vm_page->p_next_page) {
        vm_page->p_next_page->p_prev_page = vm_page->p_prev_page;
    }
    vm_page->p_prev_page->p_next_page = vm_page->p_next_page;
    m_map_return_vm_page_to_kernel((void *)vm_page, 1);
}

static int compare_free_block_size(void *_block_meta_data_1, void *_block_meta_data_2)
{
    block_meta_data_t *block_meta_data_1 = (block_meta_data_t *)_block_meta_data_1;
    block_meta_data_t *block_meta_data_2 = (block_meta_data_t *)_block_meta_data_2;

    if(block_meta_data_1->block_size > block_meta_data_2->block_size) {
        return -1;
    }
    else if(block_meta_data_1->block_size < block_meta_data_2->block_size) {
        return 1;
    }
    return 0;
}

static void m_map_add_free_block_meta_data_to_block_list
                (vm_page_family_t *vm_page_family, block_meta_data_t *free_block)
{
    assert(free_block->is_free == MM_TRUE);
    glthread_priority_insert(&vm_page_family->free_block_priority_list_head,
                             &free_block->priority_list_node,
                             compare_free_block_size,
                             OFFSET_OF(block_meta_data_t, priority_list_node));
}

static vm_page_t* m_map_add_new_page_to_family(vm_page_family_t *vm_page_family)
{
    vm_page_t *vm_page = allocate_vm_page(vm_page_family);

    if(!vm_page) {
        return NULL;
    }

    m_map_add_free_block_meta_data_to_block_list(vm_page_family, &vm_page->block_meta_data);
    
    return vm_page;
}

static vm_bool_e m_map_split_free_data_block(vm_page_family_t *vm_page_family, 
                                             block_meta_data_t *block_meta_data,
                                             uint32_t size)
{
    block_meta_data_t *next_block_meta_data = NULL;
    assert(block_meta_data->is_free == MM_TRUE);

    if(block_meta_data->block_size < size) {
        return MM_FALSE;
    }

    uint32_t remaining_size = block_meta_data->block_size - size;

    block_meta_data->is_free = MM_FALSE;
    block_meta_data->block_size = size;
    remove_glthread(&block_meta_data->priority_list_node);

    /*No Splitting Required*/
    if(!remaining_size) {
        return MM_TRUE;
    }

    /*Partial Splitting Required (Fragmentation)*/
    else if(sizeof(block_meta_data) < remaining_size && 
            remaining_size < (sizeof(block_meta_data) + vm_page_family->struct_size)) {
        next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);
        next_block_meta_data->is_free = MM_TRUE;
        next_block_meta_data->block_size = remaining_size - sizeof(block_meta_data);
        next_block_meta_data->offset = block_meta_data->offset +
                                       block_meta_data->block_size +
                                       sizeof(block_meta_data_t);
        init_glthread(&next_block_meta_data->priority_list_node);
        m_map_add_free_block_meta_data_to_block_list(vm_page_family, next_block_meta_data);
        m_map_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
    }

    else if(remaining_size < sizeof(block_meta_data_t)) {
        /*do nothing*/
    }

    /*Full Split, New Meta Block is created*/
    else {
        next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);
        next_block_meta_data->is_free = MM_TRUE;
        next_block_meta_data->block_size = remaining_size - sizeof(block_meta_data);
        next_block_meta_data->offset = block_meta_data->offset +
                                       block_meta_data->block_size +
                                       sizeof(block_meta_data_t);
        init_glthread(&next_block_meta_data->priority_list_node);
        m_map_add_free_block_meta_data_to_block_list(vm_page_family, next_block_meta_data);
        m_map_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
    }
    return MM_TRUE;
}

block_meta_data_t* m_map_allocate_free_data_block(vm_page_family_t *vm_page_family, uint32_t req_size)
{
    vm_bool_e status = MM_FALSE;
    vm_page_t *vm_page = NULL;
    block_meta_data_t *block_meta_data = NULL;

    block_meta_data_t *largest_block_meta_data = m_map_get_largest_free_block(vm_page_family);

    if(!largest_block_meta_data || largest_block_meta_data->block_size < req_size) {
        /*add new vm page to satisfy the request*/
        printf("NEW PAGE ALLOCATED\n");
        vm_page = m_map_add_new_page_to_family(vm_page_family);
        status = m_map_split_free_data_block(vm_page_family, &vm_page->block_meta_data, req_size);

        if(status) {
            return &vm_page->block_meta_data;
        }
        return NULL;
    }

    if(largest_block_meta_data) {
        status = m_map_split_free_data_block(vm_page_family, largest_block_meta_data, req_size);
    }

    if(status) {
        return largest_block_meta_data;
    }
    return NULL;
}

void *my_calloc(char *struct_name, int units)
{
    // printf("XCALLOC: memory requested for struct -> %s\n",struct_name);
    vm_page_family_t *page_family = lookup_page_family_by_name(struct_name);

    if(!page_family) {
        printf("ERROR: Structure %s is not registered with Memory Manager\n", struct_name);
        return NULL;
    }

    if((units * page_family->struct_size) > m_map_max_page_allocatable_memory(1)) {
        printf("ERROR: Memory requested exceeds Page Size\n");
        return NULL;
    }

    block_meta_data_t *free_block_meta_data = NULL;
    free_block_meta_data = m_map_allocate_free_data_block(page_family, (units * page_family->struct_size));
    
    if(free_block_meta_data) {
        memset((char *)free_block_meta_data+1, 0,free_block_meta_data->block_size);
        return (void *)free_block_meta_data+1;
    }
    return NULL;
}
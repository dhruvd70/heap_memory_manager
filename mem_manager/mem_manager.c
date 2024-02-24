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
        printf(ANSI_COLOR_RED"ERROR: VM Page Allocation Failed!\n"ANSI_COLOR_RESET);
        return NULL;
    }
    memset(vm_page, 0, (units * SYSTEM_PAGE_SIZE));
    // printf("VM Page Map successful\n");
    return (void *)vm_page;
}

/*Funtion to Return VM Page to Kernel*/
static void m_map_return_vm_page_to_kernel(void *vm_page)
{
    if(munmap(vm_page, SYSTEM_PAGE_SIZE)) {
        printf(ANSI_COLOR_RED"ERROR: Could Not Unmap VM Page to Kernel\n"ANSI_COLOR_RESET);
    }
    else{
        printf("VM Page Unmap Successful\n");
    }
}

static void m_map_merge_free_blocks(block_meta_data_t *first, block_meta_data_t *second)
{
    assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);

    first->block_size += sizeof(block_meta_data_t) + second->block_size;

    first->p_next_block = second->p_next_block;
    if(second->p_next_block) {
        second->p_next_block->p_prev_block = first;
    }
}

static inline uint32_t m_map_max_page_allocatable_memory(int units)
{
    return (uint32_t)((SYSTEM_PAGE_SIZE * units) - OFFSET_OF(vm_page_t, page_mem));
}

vm_page_family_t* lookup_page_family_by_name(char *struct_name)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    if(!first_vm_page_for_families) {
        printf(ANSI_COLOR_RED"ERROR: No Page Families Registered!\n"ANSI_COLOR_RESET);
        return NULL;
    }
    for(vm_page_for_families_curr = first_vm_page_for_families;
        vm_page_for_families_curr;
        vm_page_for_families_curr = vm_page_for_families_curr->pNext){
            ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr, vm_page_family_curr) {
                if(strncmp(vm_page_family_curr->struct_name, struct_name, MAX_STRUCT_NAME_SIZE) == 0) {
                    return vm_page_family_curr;
                }
            }ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr);
    }
    // printf(ANSI_COLOR_RED"ERROR: No Page Families called %s exist.\n"ANSI_COLOR_RESET,struct_name);
    return NULL;
}

void m_map_instansiate_new_page_family(char *struct_name, uint32_t struct_size)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *new_vm_page_for_familes = NULL;

    if(struct_size > SYSTEM_PAGE_SIZE) {
        printf(ANSI_COLOR_RED"ERROR: %s() Structure %s Size exceeds System Page Size"ANSI_COLOR_RESET,
                                                __FUNCTION__, struct_name);
        return;
    }

    if(!first_vm_page_for_families) {
        first_vm_page_for_families = (vm_page_for_families_t *)m_map_get_vm_page_from_kernel(1);
        first_vm_page_for_families->pNext = NULL;
        strncpy(first_vm_page_for_families->vm_page_family[0].struct_name,struct_name, MAX_STRUCT_NAME_SIZE);

        first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
        first_vm_page_for_families->vm_page_family[0].first_page = NULL;
        init_glthread(&first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head);
        return;
    }

    vm_page_family_curr = lookup_page_family_by_name(struct_name);

    if(vm_page_family_curr) {
        assert(0);
    }

    uint32_t count = 0;
    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {

        count ++;

    }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);

    if(count == MAX_FAMILIES_PER_VM_PAGE) {
        new_vm_page_for_familes = (vm_page_for_families_t *)m_map_get_vm_page_from_kernel(1);
        new_vm_page_for_familes->pNext = first_vm_page_for_families;
        first_vm_page_for_families = new_vm_page_for_familes;
    }

    strncpy(vm_page_family_curr->struct_name, struct_name, MAX_STRUCT_NAME_SIZE);
    vm_page_family_curr->struct_size = struct_size;
    vm_page_family_curr->first_page = NULL;
    init_glthread(&vm_page_family_curr->free_block_priority_list_head);
}

void m_map_print_registered_page_families(void)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    if(!first_vm_page_for_families) {
        printf(ANSI_COLOR_RED"ERROR: No Page Families Registered!\n"ANSI_COLOR_RESET);
        return;
    }
    for(vm_page_for_families_curr = first_vm_page_for_families;
        vm_page_for_families_curr;
        vm_page_for_families_curr = vm_page_for_families_curr->pNext){
            ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr, vm_page_family_curr) {
            printf("Page Family: %s, Size: %d\n",vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);
            }ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr);
    }
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

        m_map_return_vm_page_to_kernel((void *)vm_page);
        return;
    }

    if(vm_page->p_next_page) {
        vm_page->p_next_page->p_prev_page = vm_page->p_prev_page;
    }
    vm_page->p_prev_page->p_next_page = vm_page->p_next_page;
    m_map_return_vm_page_to_kernel((void *)vm_page);
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

static void m_map_add_free_block_meta_data_to_block_list(vm_page_family_t *vm_page_family, block_meta_data_t *free_block)
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
    else if(sizeof(block_meta_data_t) < remaining_size && 
            remaining_size < (sizeof(block_meta_data_t) + vm_page_family->struct_size)) {
        next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);
        next_block_meta_data->is_free = MM_TRUE;
        next_block_meta_data->block_size = remaining_size - sizeof(block_meta_data_t);
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
        next_block_meta_data->block_size = remaining_size - sizeof(block_meta_data_t);
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
        // printf("NEW PAGE ALLOCATED\n");
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
        printf(ANSI_COLOR_RED"ERROR: Structure %s is not registered with Memory Manager\n"ANSI_COLOR_RESET, struct_name);
        return NULL;
    }

    if((units * page_family->struct_size) > m_map_max_page_allocatable_memory(1)) {
        printf(ANSI_COLOR_RED"ERROR: Memory requested exceeds Page Size\n"ANSI_COLOR_RESET);
        return NULL;
    }

    block_meta_data_t *free_block_meta_data = NULL;
    free_block_meta_data = m_map_allocate_free_data_block(page_family, (units * page_family->struct_size));
    
    if(free_block_meta_data) {
        memset((char *)(free_block_meta_data + 1), 0,free_block_meta_data->block_size);
        return (void *)(free_block_meta_data + 1);
    }
    return NULL;
}

static int m_map_get_hard_internal_frag_mem_size(block_meta_data_t *first, block_meta_data_t *second)
{
    block_meta_data_t *next = NEXT_META_BLOCK_BY_SIZE(first);

    return (int)(unsigned long)second - (unsigned long)(next);
}

static block_meta_data_t *m_map_free_block(block_meta_data_t *block_to_free)
{
    block_meta_data_t *block_to_return = NULL;

    assert(block_to_free->is_free == MM_FALSE);
    vm_page_t *host_page = GET_PAGE_FROM_META_BLOCK(block_to_free);

    vm_page_family_t *vm_page_family = host_page->p_page_family;

    block_to_return = block_to_free;
    block_to_free->is_free = MM_TRUE;

    block_meta_data_t *next_block = NEXT_META_BLOCK(block_to_free);

    if(next_block) {
        // block to be freed is not the last block in current page i.e next_block != NULL
        block_to_free->block_size += m_map_get_hard_internal_frag_mem_size(block_to_free, next_block);
    }
    else{
        //block to be freed is the last block in current page i.e next_block == NULL
        char *end_addr_of_page = (char *)((char *)host_page + SYSTEM_PAGE_SIZE);
        char *end_addr_of_free_data_block = (char *)(block_to_free + 1) + block_to_free->block_size;

        int fragmented_mem = (int) (unsigned long)end_addr_of_page - (unsigned long)end_addr_of_free_data_block;

        block_to_free->block_size += fragmented_mem;
    }

    //perform merging of free blocks if possible
    if(next_block && next_block->is_free == MM_TRUE) {
        m_map_merge_free_blocks(block_to_free, next_block);
        block_to_return = block_to_free;
    }

    // check if previous block is free
    block_meta_data_t *prev_block = PREV_META_BLOCK(block_to_free);
    if(prev_block && prev_block->is_free == MM_TRUE) {
        m_map_merge_free_blocks(prev_block, block_to_free);
        block_to_return = prev_block;
    }

    if (m_map_is_vm_page_empty(host_page)) {
        deallocate_vm_page(host_page);
        return NULL;
    }
    
    m_map_add_free_block_meta_data_to_block_list(host_page->p_page_family, block_to_return);

    return block_to_return;
}

void my_free(void *app_data)
{
    // block_meta_data_t *block_meta_data = (block_meta_data_t *)((char *)app_data - sizeof(block_meta_data_t));
    block_meta_data_t *block_meta_data = (block_meta_data_t *)((char *)app_data - sizeof(block_meta_data_t));

    assert(block_meta_data->is_free == MM_FALSE);
    m_map_free_block(block_meta_data);
}

void m_map_print_vm_page_details(vm_page_t *vm_page)
{
    printf("\tNext = %p, Prev = %p\n",vm_page->p_next_page, vm_page->p_prev_page);
    printf(ANSI_COLOR_MAGENTA"\tPage Family = %s\n"ANSI_COLOR_RESET, vm_page->p_page_family->struct_name);

    uint32_t i = 1;
    block_meta_data_t *curr;

    ITERATE_BLOCKS_IN_VM_PAGE_BEGIN(vm_page, curr) {
        printf("\t\t%-14p | Block %-3u| %s | Block Size = %-6u | Offset = %-6u | Prev = %-14p | Next = %-14p\n",
                                                            curr, i++, curr->is_free ? " F R E E " : "ALLOCATED",
                                                            curr->block_size, curr->offset,
                                                            curr->p_prev_block, curr->p_next_block);
    }ITERATE_BLOCKS_IN_VM_PAGE_END(vm_page, curr);
}

void m_map_print_mem_usage(char *struct_name)
{   
    uint32_t i = 0;
    vm_page_t *vm_page = NULL;
    vm_page_family_t *vm_page_family_curr;
    uint32_t num_struct_families = 0;
    uint32_t num_vm_pages_from_kernel = 0;

    printf(ANSI_COLOR_CYAN"PAGE SIZE = %ld Bytes\n"ANSI_COLOR_RESET,SYSTEM_PAGE_SIZE);

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {

        if(struct_name) {
            if(strncmp(struct_name, vm_page_family_curr->struct_name, MAX_STRUCT_NAME_SIZE) == 0) {
                continue;
            }
        }
        num_struct_families++;
        printf(ANSI_COLOR_YELLOW"vm_page_family : %s, size : %d\n"ANSI_COLOR_RESET,vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);

        i = 0;

        ITERATE_VM_PAGE_BEGIN(vm_page_family_curr, vm_page) {
            
            num_vm_pages_from_kernel++;
            m_map_print_vm_page_details(vm_page);

        }ITERATE_VM_PAGE_END(vm_page_family_curr, vm_page);
        printf("\n");
    }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);

    printf(ANSI_COLOR_RED"No. of VM Pages in Use : %u (Total Memory Used : %lu Bytes)\n"ANSI_COLOR_RESET,num_vm_pages_from_kernel, SYSTEM_PAGE_SIZE * num_vm_pages_from_kernel);
}

void m_map_print_block_usage(void)
{
    vm_page_t *vm_page_curr;
    vm_page_family_t * vm_page_family_curr;
    block_meta_data_t *block_meta_data_curr;

    uint32_t total_block_count, free_block_count, occupied_block_count;

    uint32_t app_mem_usage;
    printf("\n");
    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
        total_block_count = 0;
        free_block_count  = 0;
        occupied_block_count = 0;
        app_mem_usage = 0;

        ITERATE_VM_PAGE_BEGIN(vm_page_family_curr, vm_page_curr) {

            ITERATE_BLOCKS_IN_VM_PAGE_BEGIN(vm_page_curr, block_meta_data_curr) {
                
                total_block_count++;
                if(block_meta_data_curr->is_free == MM_TRUE) {
                    free_block_count ++;
                }
                else {
                    app_mem_usage += block_meta_data_curr->block_size + sizeof(block_meta_data_t);
                    occupied_block_count++;
                }
            }ITERATE_BLOCKS_IN_VM_PAGE_END(vm_page_curr, block_meta_data_curr)
        }ITERATE_VM_PAGE_END(vm_page_family_curr, vm_page_curr)

        printf("%-20s TBC = %-4u    FBC = %-4u    OBC = %-4u    App_Mem_Usage = %u\n", vm_page_family_curr->struct_name,
                                                                                        total_block_count,
                                                                                        free_block_count,
                                                                                        occupied_block_count,
                                                                                        app_mem_usage);
    }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr)
}
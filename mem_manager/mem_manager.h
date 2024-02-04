#ifndef __MEM_MANAGER__
#define __MEM_MANAGER__

#include <stdint.h>
#include <assert.h>

#include "../gluethread/glthread.h"

#define MAX_STRUCT_NAME_SIZE        32

typedef enum{
    MM_FALSE,
    MM_TRUE
}vm_bool_e;

typedef struct block_meta_data_{
    vm_bool_e is_free;
    uint32_t block_size;
    uint32_t offset;
	glthread_t priority_list_node;
    struct block_meta_data_ *p_prev_block;
    struct block_meta_data_ *p_next_block;
}block_meta_data_t;

GLTHREAD_TO_STRUCT(glthread_to_block_meta_data, block_meta_data_t, 
					priority_list_node, glthread_ptr);

struct vm_page_family_;//forward declaration

typedef struct vm_page_{
	struct vm_page_ *p_next_page;
	struct vm_page_ *p_prev_page;
	struct vm_page_family_ *p_page_family;
	block_meta_data_t block_meta_data;
	char page_mem[0]; /*First Data block in VM Page*/
}vm_page_t;

typedef struct vm_page_family_{
    char struct_name[MAX_STRUCT_NAME_SIZE];
    uint32_t struct_size;
	vm_page_t *first_page;
	glthread_t free_block_priority_list_head;
}vm_page_family_t;

typedef struct vm_page_for_families_{
    struct vm_page_for_families_ *pNext;
    vm_page_family_t vm_page_family[0];
}vm_page_for_families_t;

#define OFFSET_OF(container_struct, field_name)         \
        ((size_t) & (((container_struct *) 0)->field_name))

#define GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)   \
        ((void *)((char *)block_meta_data_ptr - block_meta_data_ptr->offset))

#define NEXT_META_BLOCK(block_meta_data_ptr)            \
        (block_meta_data_ptr->p_next_block)

#define PREV_META_BLOCK(block_meta_data_ptr)            \
        (block_meta_data_ptr->p_prev_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)    			\
        (block_meta_data_t *) ((char *)(block_meta_data_ptr + 1)	\
        + block_meta_data_ptr->block_size)

#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)    	\
        free_meta_block->p_prev_block = allocated_meta_block;                       \
        free_meta_block->p_next_block = allocated_meta_block->next_block;           \
        allocated_meta_block->p_next_block = free_meta_block;                       \
        if (free_meta_block->p_next_block)                                          \
        free_meta_block->p_next_block->p_prev_block = free_meta_block

#define MARK_VM_PAGE_EMPTY(vm_page_t_ptr)					\
		vm_page_t_ptr->block_meta_data.p_next_block = NULL;	\
		vm_page_t_ptr->block_meta_data.p_prev_block = NULL;	\
		vm_page_t_ptr->block_meta_data.is_free = MM_TRUE;	\

#define MAX_FAMILIES_PER_VM_PAGE    							\
        (SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t))/    \
        sizeof(vm_page_family_t)

/*Iterative Macro to traverse Page Families*/
#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)         \
        {                                                                   \
        uint32_t count = 0;                                                 \
        for(curr = (vm_page_family_t*)&vm_page_for_families_ptr->vm_page_family[0];    \
        curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;              \
        curr++, count++) {

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) }}

/*Iterative Macro to traverse VM Pages of same Families*/
#define ITERATE_VM_PAGE_BEGIN(vm_page_family_ptr, curr)						\
		{																	\
		curr = vm_page_family_ptr->first_page;								\
		vm_page_t *pNext = NULL;											\
		for(; curr; curr=pNext) {											\
			pNext = curr->p_next_page;

#define ITERATE_VM_PAGE_END(vm_page_family_ptr, curr)	}}

/*Iterative Macro to traverse Mem Blocks in one VM Page*/
#define ITERATE_BLOCKS_IN_VM_PAGE_BEGIN(vm_page_ptr, curr)					\
		{																	\
		curr = &vm_page_ptr->block_meta_data;								\
		block_meta_data_t *pNext = NULL;									\
		for(; curr; curr = pNext) {											\
			pNext = NEXT_META_BLOCK(curr);

#define ITERATE_BLOCKS_IN_VM_PAGE_END(vm_page_ptr, curr)	}}

vm_page_family_t* lookup_page_family_by_name(char *struct_name);

vm_bool_e m_map_is_vm_page_empty(vm_page_t *vm_page);

vm_page_t* allocate_vm_page(vm_page_family_t *vm_page_family);

void deallocate_vm_page(vm_page_t *vm_page);

static inline block_meta_data_t* m_map_get_largest_free_block(vm_page_family_t *vm_page_family)
{
	glthread_t *largest_free_block = vm_page_family->free_block_priority_list_head.right;

	if(largest_free_block) {
		return glthread_to_block_meta_data(largest_free_block);
	}
	return NULL;
}

#endif
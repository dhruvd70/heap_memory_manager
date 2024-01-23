
#include <stdint.h>
#include <assert.h>

#define MAX_STRUCT_NAME_SIZE        32

typedef enum{
    MM_FALSE,
    MM_TRUE
}vm_bool_e;
typedef struct vm_page_family_{
    char struct_name[MAX_STRUCT_NAME_SIZE];
    uint32_t struct_size;
}vm_page_family_t;

typedef struct vm_page_for_families_{
    struct vm_page_for_families_ *pNext;
    vm_page_family_t vm_page_family[0];
}vm_page_for_families_t;

typedef struct block_meta_data_{
    vm_bool_e is_free;
    uint32_t block_size;
    uint32_t offset;
    struct block_meta_data_ *p_prev_block;
    struct block_meta_data_ *p_next_block;
}block_meta_data_t;

#define OFFSET_OF(container_struct, field_name)         \
        ((size_t) & (((container_struct *) 0)->field_name))

#define GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)   \
        ((void *)((char *)block_meta_data_ptr - block_meta_data_ptr->offset))

#define NEXT_META_BLOCK(block_meta_data_ptr)            \
        (block_meta_data_ptr->p_next_block)

#define PREV_META_BLOCK(block_meta_data_ptr)            \
        (block_meta_data_ptr->p_prev_block)
#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)    \
        (block_meta_data_t *) ((char *)(block_meta_data_ptr + 1)\
        + block_meta_data_ptr->block_size)

#define MAX_FAMILIES_PER_VM_PAGE    \
        (SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t))/    \
        sizeof(vm_page_family_t)

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)         \
    {                                                                       \
        uint32_t count = 0;                                                 \
        for(curr = (vm_page_family_t*)&vm_page_for_families_ptr->vm_page_family[0];    \
        curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;              \
        curr++, count++) {

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) }}

vm_page_family_t* lookup_page_family_by_name(char *struct_name);


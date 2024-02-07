#ifndef __CUSTOM_LMM__
#define __CUSTOM_LMM__

#include <stdint.h>

void m_map_init(void);

void m_map_instansiate_new_page_family(char *struct_name, uint32_t struct_size);

void m_map_print_registered_page_families(void);

void* my_calloc(char *struct_name, int units);

void m_map_print_mem_usage(char *struct_name);

#define MY_CALLOC(struct_name, units)       \
        my_calloc(#struct_name, units)

#define M_MAP_REG_STRUCT(struct_name)       \
        (m_map_instansiate_new_page_family(#struct_name, sizeof(struct_name)))


#endif
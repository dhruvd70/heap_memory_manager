#ifndef __CUSTOM_LMM__
#define __CUSTOM_LMM__

#include <stdint.h>

void m_mamp_init(void);

void m_map_instansiate_new_page_family(char *struct_name, uint32_t struct_size);

void mm_print_registered_page_families(void);

#define M_MAP_REG_STRUCT(struct_name)       \
        (m_map_instansiate_new_page_family(#struct_name, sizeof(struct_name)))


#endif
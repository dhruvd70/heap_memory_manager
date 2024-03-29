#include <stdio.h>
#include <string.h>

#include "custom_lmm.h"
typedef struct emp_
{
    char name[32];
    uint32_t emp_id;
}emp_t;

typedef struct test_{
    char arr[56];
}test_t;

int main()
{
    int wait;
    m_map_init();
    
    M_MAP_REG_STRUCT(emp_t);
    M_MAP_REG_STRUCT(test_t);

    m_map_print_registered_page_families();

    test_t *test1 = MY_MALLOC(test_t);
    test_t *test2 = MY_MALLOC(test_t);    

    emp_t *emp1 = MY_CALLOC(emp_t, 1);
    emp_t *emp2 = MY_CALLOC(emp_t, 1);
    emp_t *emp3 = MY_CALLOC(emp_t, 1);

    m_map_print_mem_usage(0);
    m_map_print_block_usage();

    scanf("%d", &wait);

    printf("/********SCENARIO 1********/\n");
    MY_FREE(emp1);
    MY_FREE(test2);

    m_map_print_mem_usage(0);
    m_map_print_block_usage();

    scanf("%d", &wait);

    printf("/********SCENARIO 2********/\n");
    MY_FREE(emp2);

    m_map_print_mem_usage(0);
    m_map_print_block_usage();

    scanf("%d", &wait);

    printf("/********SCENARIO 3********/\n");
    MY_FREE(emp3);
    MY_FREE(test1);

    m_map_print_mem_usage(0);
    m_map_print_block_usage();
    return 0;
}
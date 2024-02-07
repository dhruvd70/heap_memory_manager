#include <stdio.h>
#include <string.h>

#include "custom_lmm.h"
typedef struct emp_
{
    char name[32];
    uint32_t emp_id;
}emp_t;

typedef struct test_{
    int var_1;
    int var_2;
    int var_3;
    char arr[20];
}test_t;

int main()
{
    m_map_init();
    M_MAP_REG_STRUCT(emp_t);
    M_MAP_REG_STRUCT(test_t);
    m_map_print_registered_page_families();

    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);
    MY_CALLOC(test_t, 1);

    // MY_CALLOC(emp_t, 1);
    // MY_CALLOC(emp_t, 1);
    // MY_CALLOC(emp_t, 1);
    // MY_CALLOC(emp_t, 1);
    // MY_CALLOC(emp_t, 1);
    // MY_CALLOC(emp_t, 1);

    m_map_print_mem_usage(0);
    return 0;
}
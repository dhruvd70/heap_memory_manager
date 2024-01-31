#include "custom_lmm.h"

typedef struct emp_
{
    char name[32];
    uint32_t emp_id;
}emp_t;

typedef struct student_
{
    char name[32];
    uint32_t roll_no;
    uint32_t marks_phy;
    uint32_t marks_chem;
    uint32_t marks_maths;
    uint32_t marks_eng;
}student_t;

typedef struct test_
{
    int *a;
    int *b;
}test_t;


int main()
{
    m_mamp_init();

    M_MAP_REG_STRUCT(emp_t);
    M_MAP_REG_STRUCT(student_t);

    M_MAP_REG_STRUCT(test_t);

    m_map_print_registered_page_families();
    return 0;
}
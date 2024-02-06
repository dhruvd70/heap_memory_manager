#include <stdio.h>
#include <string.h>

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

int main()
{
    m_map_init();
    M_MAP_REG_STRUCT(emp_t);
    M_MAP_REG_STRUCT(student_t);

    m_map_print_registered_page_families();
    printf("\n\n");
    for(int i=0;i<50;i++) {
        printf("%d\n",i);
        MY_CALLOC(emp_t,1);
        MY_CALLOC(student_t,1);
    }
    return 0;
}
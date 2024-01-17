#include <stdio.h>
#include <memory.h>
#include <unistd.h>     //to get page size
#include <sys/mman.h>   //to use mmap()

static size_t SYSTEM_PAGE_SIZE = 0;

void m_mamp_init(void)
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

int main(int argc, char **argv)
{
    m_mamp_init();
    printf("SYSTEM PAGE SIZE == %ld\n",SYSTEM_PAGE_SIZE);
    
    void *page1 = m_map_get_vm_page_from_kernel(1);
    void *page2 = m_map_get_vm_page_from_kernel(1);
    printf("====> %ld\n",page1 - page2);
    m_map_return_vm_page_to_kernel(page1, 1);
    return 0;
}
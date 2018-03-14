#define  __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <module.h>

/*
 *  SylixOS call module_init() and module_exit() automatically.
 */
int module_init (void)
{
    printk("hello_module init!\n");

    return 0;
}

void module_exit (void)
{
    printk("hello_module exit!\n");
}

/*
 *  module export symbols
 */
LW_SYMBOL_EXPORT void hello_module_func (void)
{
    printk("hello_module_func() run!\n");
}

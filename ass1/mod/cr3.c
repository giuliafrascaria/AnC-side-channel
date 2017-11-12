// #include <linux/module.h>
// #include <linux/kernel.h>
// #include <linux/init.h>
// #include <asm/current.h>
// #include <asm/uaccess.h>
// #include <linux/sched.h>
// #include <linux/fs_struct.h>
//
//
// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Giulia");
// MODULE_DESCRIPTION("cr3 module");
//
// unsigned long get_cr3(char __user *buffer, unsigned long len)
// {
//   pgd_t * cr3;
//   unsigned long buf_len;
//
//   //this should not be the case but you never know
//   cr3 = current->mm->pgd;
//   if(cr3 == NULL)
//   {
//     return -1;
//   }
//
//   //returns 0 on success
//   buf_len = copy_to_user(buffer, (const void *) cr3, len);
//   printk("cr3 = %p\n", cr3);
//
//   return buf_len;
// }

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>

MODULE_LICENSE("GPL");

void* get_cr3(char __user *buffer, unsigned long len)
{
  pgd_t * cr3;
  unsigned long buf_len;

  //this should not be the case but you never know
  cr3 = current->mm->pgd;
  if(cr3 == NULL)
  {
    return -1;
  }
  return cr3;
}

int __init module_load(void){

	printk( KERN_ALERT "LOADing \n your cr3 address is %p\n", get_cr3);
	return -1; // Means that kernel module always fails during installation, Kernel module won't be in a loaded modules list
}

void __exit module_cleanup(void){
	return;
}

module_init(module_load);
module_exit(module_cleanup);

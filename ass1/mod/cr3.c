#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giulia");
MODULE_DESCRIPTION("cr3 module");

unsigned long get_cr3(char __user *buffer, unsigned long len)
{
  pgd_t * cr3;
  unsigned long buf_len;

  //this should not be the case but you never know
  cr3 = current->mm->pgd;
  if(cr3 == NULL)
  {
    return -1;
  }

  //returns 0 on success
  buf_len = copy_to_user(buffer, (const void *) cr3, len);
  printk("cr3 = %p\n", cr3);

  return buf_len;
}

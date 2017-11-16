#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");

static struct proc_dir_entry* cr3_file;

unsigned long get_cr3(void)
{

      unsigned long cr3;
      asm volatile(

          "mov %%cr3, %%rax\n\t"
          "mov %%rax, %0\n\t"
      :  "=m" (cr3)
      : /* no input */
      : "%rax"
      );



      printk("cr3 = 0x%lx\n", cr3);


      pgd_t * k_cr3;
      unsigned long phys_cr3;

      k_cr3 = current->mm->pgd;
      phys_cr3= virt_to_phys((void *) k_cr3);
      printk("cr3 hi %lx\n", phys_cr3);


      return cr3;



  //phys_cr3 = __pa_symbol((void *) cr3);

  // return phys_cr3;


}


static int cr3_show(struct seq_file *m, void *v)
{
    printk("cr3 lala %lu\n", get_cr3());
    seq_printf(m, "%lu\n", get_cr3());
    return 0;
}

static int cr3_open(struct inode *inode, struct file *file)
{
    return single_open(file, cr3_show, NULL);
}

static const struct file_operations cr3_file_operations = {
    .owner	= THIS_MODULE,
    .open	= cr3_open,
    .read	= seq_read,
    .llseek	= seq_lseek,
    .release	= single_release,
};

static int cr3_init(void)
{
    cr3_file = proc_create_data("cr3", 0, NULL, &cr3_file_operations, NULL);

    if(!cr3_file)
    {
		return -1;
    }

    return 0;
}

static void cr3_exit(void)
{
    remove_proc_entry("cr3", NULL);
}

module_init(cr3_init);
module_exit(cr3_exit);

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>		
#include <linux/proc_fs.h>	
#include <linux/seq_file.h>	

MODULE_LICENSE("GPL");

static struct proc_dir_entry* cr3_file;

void* get_cr3(void)
{
  pgd_t * cr3;

  //this should not be the case but you never know
  cr3 = current->mm->pgd;
  if(cr3 == NULL)
  {
    return NULL;
  }
  return cr3;
}


 static int cr3_show(struct seq_file *m, void *v)
 {
     seq_printf(m, "%p\n", (void*) get_cr3());
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

 /*    if (cr3_file) {
     	cr3_file->proc_fops = &cr3_file_operations;
     }
	
     cr3_file = proc_create("cr3", 0, NULL, &cr3_file_operations);
*/

     if(!cr3_file){
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


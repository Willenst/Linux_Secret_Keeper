#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratochka Vyacheslav");
MODULE_DESCRIPTION("A simple procfs storage module.");
MODULE_VERSION("0.01");

#define SECRET_SIZE 128
#define PROCFS_NAME "secret_stash"

static struct proc_dir_entry *storage_filename;
static char storage[SECRET_SIZE];
static unsigned long newsecret_size = 0;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
    if (*offset >= SECRET_SIZE||copy_to_user(buffer, storage, SECRET_SIZE)) { 
        pr_info("fail!");
        return 0;
    }
    else{
        *offset += SECRET_SIZE;
    }
    return SECRET_SIZE;
}


static ssize_t procfile_write(struct file *file, const char __user *buff, size_t size, loff_t *off)
{
    newsecret_size = size;
    if (newsecret_size > SECRET_SIZE){
        newsecret_size = SECRET_SIZE;
    }
    if (copy_from_user(storage, buff, newsecret_size)){
        return -EFAULT; 
    }
    storage[newsecret_size & (SECRET_SIZE - 1)] = '\0';
    pr_info("procfile write %s\n", storage);
    return newsecret_size;
}



static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};


static int __init procfs2_init(void)
{
    storage_filename = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (NULL == storage_filename) {
    proc_remove(storage_filename);
    pr_alert("Error:Could not initialize /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
    }
    pr_info("/proc/%s created\n", PROCFS_NAME);
    printk(KERN_INFO "Hello, World!\n");
    return 0;
}

static void __exit procfs2_exit(void)
{
    proc_remove(storage_filename);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
    printk(KERN_INFO "Goodbye, World!\n");
}



module_init(procfs2_init);
module_exit(procfs2_exit);

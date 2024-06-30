#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratochka Vyacheslav");
MODULE_DESCRIPTION("A simple procfs storage module.");
MODULE_VERSION("0.07");


#define MAX_SECRET_SIZE 138
#define MAX_SECRETS 10
#define PROCFS_NAME "secret_stash"

typedef struct secret{
    int secret_id;
    char secret_data[MAX_SECRET_SIZE];
    struct list_head list_node; //переделка списка под макрос
} secret_t;

LIST_HEAD(secrets); //переделка списка под макрос

static struct proc_dir_entry *storage_filename;
static struct secret storage[MAX_SECRETS];
static unsigned long newsecret_size = 0;
static char secret_buffer[MAX_SECRET_SIZE];
static int next_id=0;
static int read_index=-1;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
    int i;
    char output_buffer[MAX_SECRET_SIZE*(next_id+1)];
    char temp_buffer[MAX_SECRET_SIZE];
    if (read_index == -1){
            for (i = 0; i < next_id; i++) {
                sprintf(temp_buffer, "%d. %s\n",storage[i].secret_id,storage[i].secret_data);
                strcat(output_buffer,temp_buffer);
            }
            if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, output_buffer, MAX_SECRET_SIZE)) { 
                pr_info("fail!");
                return 0;
            }
            else{
                *offset += MAX_SECRET_SIZE;
            }

        return MAX_SECRET_SIZE;
            }
    else{
        sprintf(temp_buffer, "%d. %s\n",storage[read_index].secret_id,storage[read_index].secret_data);
        if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, temp_buffer, MAX_SECRET_SIZE)) { 
            pr_info("fail!");
            return 0;
        }
        else{
            *offset += MAX_SECRET_SIZE;
        }
        return MAX_SECRET_SIZE;
        }
    return MAX_SECRET_SIZE;
}



static ssize_t procfile_write(struct file *file, const char __user *buff, size_t size, loff_t *off)
{
    int id;
    int i;
    char command;
    char secret_data[MAX_SECRET_SIZE];

    newsecret_size = size;

    if (newsecret_size > MAX_SECRET_SIZE){
        newsecret_size = MAX_SECRET_SIZE;
    }

    if (copy_from_user(secret_buffer, buff, newsecret_size)){
        return -EFAULT; 
    }

    if (sscanf(secret_buffer, "%c %d %s", &command, &id, secret_data) !=3 ){
        printk(KERN_ERR "Failed to parse input\n");
        return -EFAULT;
    }
    if (id<-1){
        return -EINVAL;
    }
    switch (command) {
        case 'W':
            if (next_id >= MAX_SECRETS||id > next_id)
                return -ENOMEM;
            secret_t* new_secret = (secret_t*)kmalloc(sizeof(secret_t), GFP_KERNEL);
            new_secret->secret_id = next_id;
            strscpy(new_secret->secret_data, secret_data, MAX_SECRET_SIZE);        
            list_add_tail(&new_secret->list_node, &secrets);

            struct  list_head *pos;
            int counter = 0;

            list_for_each(pos, &secrets) {
                secret_t* p = NULL;
                p = list_entry(pos, secret_t, list_node);
                pr_info("Secret %d {%d, %s}\n", counter, p->secret_id, p->secret_data);
                counter++;
            }
            next_id++;
            return newsecret_size;
        case 'R':
            if (id > next_id)
                return -ENOMEM;
            read_index = id;
            pr_info("current index %s\n", id);
            return newsecret_size;
        case 'D':
            if (id<0||id > next_id)
                return -EINVAL;
            for (int i = id; i < next_id - 1; ++i) {
                storage[i] = storage[i + 1];
            }
            next_id--;
        default:
            return -EINVAL;
    }
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

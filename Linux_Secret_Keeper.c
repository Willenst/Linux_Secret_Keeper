#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratochka Vyacheslav");
MODULE_DESCRIPTION("A simple procfs storage module.");
MODULE_VERSION("0.08");


#define MAX_SECRET_SIZE 138
#define MAX_SECRETS 10
#define PROCFS_NAME "secret_stash"
#define MAX_ID 30000
#define MIN_ID 0

typedef struct secret{
    int secret_id;
    char secret_data[MAX_SECRET_SIZE];
    struct list_head list_node;
} secret_t;

LIST_HEAD(secrets);

static struct proc_dir_entry *storage_filename;
static unsigned long newsecret_size = 0;
static char secret_buffer[MAX_SECRET_SIZE];
static int next_id=0;
static int read_index=-1;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
    struct list_head *pos;
    secret_t* p = NULL;
    char output_buffer[MAX_SECRET_SIZE*MAX_SECRETS];
    char temp_buffer[MAX_SECRET_SIZE];
    if (read_index == -1){
            list_for_each(pos, &secrets) {
                p = list_entry(pos, secret_t, list_node);
                sprintf(temp_buffer, "%d. %s\n", p->secret_id, p->secret_data);
                strcat(output_buffer,temp_buffer);
            }
            if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, output_buffer, MAX_SECRET_SIZE)) { 
                return 0;
            }
            else{
                *offset += MAX_SECRET_SIZE;
            }

        return MAX_SECRET_SIZE;
            }
    else{
        list_for_each(pos, &secrets){
            p = list_entry(pos, secret_t, list_node);
        if (p->secret_id == read_index) {
            sprintf(temp_buffer, "%d. %s\n", p->secret_id, p->secret_data);
            if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, temp_buffer, MAX_SECRET_SIZE)) { 
                return 0;
            }
            else{
                *offset += MAX_SECRET_SIZE;
            }
            return MAX_SECRET_SIZE;
        }
        }
    }
    return MAX_SECRET_SIZE;
}

static bool secret_finder(int id, struct list_head *secrets) {
    struct list_head *pos;

    list_for_each(pos, secrets) {
        secret_t *p = list_entry(pos, secret_t, list_node);
        if (p->secret_id == id) {
            return true;
        }
    }
    return false;
}



static ssize_t procfile_write(struct file *file, const char __user *buff, size_t size, loff_t *off)
{
    int id;
    char command;
    char secret_data[MAX_SECRET_SIZE];
    struct list_head *pos;
    struct list_head* tmp;
    bool deleted = false;
    newsecret_size = size;

    if (newsecret_size > MAX_SECRET_SIZE){
        return -EINVAL;
    }

    if (copy_from_user(secret_buffer, buff, newsecret_size)){
        return -EFAULT; 
    }

    if (sscanf(secret_buffer, "%c %d %s", &command, &id, secret_data)<2){
        printk(KERN_ERR "Failed to parse input\n");
        return -EFAULT;
    }
    if (id<-1||id>MAX_ID||id<MIN_ID){
        return -EINVAL;
    }
    switch (command) {
        case 'W':
            if (next_id >= MAX_SECRETS)
                return -ENOMEM;
            if (secret_finder(id, &secrets))
                return -EINVAL;
            secret_t* new_secret = (secret_t*)kmalloc(sizeof(secret_t), GFP_KERNEL);
            new_secret->secret_id = id;
            strscpy(new_secret->secret_data, secret_data, MAX_SECRET_SIZE);        
            list_add_tail(&new_secret->list_node, &secrets);
            next_id++;
            return newsecret_size;
        case 'R':
            if (id == -1)
            {
                read_index = id;
                return newsecret_size;
            }
            if (secret_finder(id, &secrets)){
                read_index = id;
                return newsecret_size;
            }
            return -EINVAL;
        case 'D':
            if (next_id<1)
                return -EINVAL;
            list_for_each_safe(pos, tmp, &secrets) {
                secret_t* p = list_entry(pos, secret_t, list_node);
                if (p->secret_id == id) {
                    list_del(pos);
                    kfree(p);
                    deleted=true;
                }
            }
            if (deleted==true){
            next_id--;
            return newsecret_size;
            }
            return -EINVAL;
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
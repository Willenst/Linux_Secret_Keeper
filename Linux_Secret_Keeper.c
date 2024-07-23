#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratochka Vyacheslav");
MODULE_DESCRIPTION("A simple procfs storage module.");
MODULE_VERSION("1.01");

//задание максимамального размера секрета, количества секретов, папки в procfs и лимитов на индексы
//specifying the maximum secret size, number of secrets, folder in procfs and index limits
#define MAX_SECRET_SIZE 138
#define MAX_SECRETS 10
#define PROCFS_NAME "secret_stash"
#define MAX_ID 30000
#define MIN_ID -1
//структура секрета - массив специально созданный под работу смакросом списков
//secret structure - an array of lists specially created to work with lists macro
typedef struct secret{
    int secret_id;
    char *secret_data;
    struct list_head list_node;
} secret_t;

LIST_HEAD(secrets); //инициализация макроса //macro initialization

static struct proc_dir_entry *storage_filename;
static unsigned long newsecret_size = 0;
static int next_id=0;
static int read_index=-1;

//функция чтения, принимает системный вызов read, вызывать через cat /proc/secret_stash, он сам отдает все параметры
//read function, takes system call read, call via cat /proc/secret_stash, it gives all parameters itself
static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
    struct list_head *pos;
    secret_t* p = NULL;
    char *output_buffer = kmalloc(MAX_SECRET_SIZE*next_id, GFP_KERNEL);;
    char *temp_buffer = kmalloc(MAX_SECRET_SIZE, GFP_KERNEL);
    if (!output_buffer||!temp_buffer) {
        pr_crit("Memory error, probably out of memory!");
        return -ENOMEM;
    }
    if (read_index == -1){ //итератор для чтения всех элементов //iterator to read all elements
        list_for_each(pos, &secrets) {
            p = list_entry(pos, secret_t, list_node);
            sprintf(temp_buffer, "%d. %s\n", p->secret_id, p->secret_data);
            strcat(output_buffer,temp_buffer);
        }
        kfree(temp_buffer);
        if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, output_buffer, MAX_SECRET_SIZE)) { 
            kfree(output_buffer);
            return 0;
        } else {
            *offset += MAX_SECRET_SIZE;
        }
        return MAX_SECRET_SIZE;
    } else {
        list_for_each(pos, &secrets){ //итератор для одного элемента //iterator to read one element
            p = list_entry(pos, secret_t, list_node);
            if (p->secret_id == read_index) {
                sprintf(temp_buffer, "%d. %s\n", p->secret_id, p->secret_data);
                if (*offset >= MAX_SECRET_SIZE||copy_to_user(buffer, temp_buffer, MAX_SECRET_SIZE)) {
                    kfree(temp_buffer);
                    kfree(output_buffer);
                    return 0;
                } else {
                    *offset += MAX_SECRET_SIZE;
                }
                return MAX_SECRET_SIZE;
            }
        }
    }
    return MAX_SECRET_SIZE;
}

static bool secret_finder(int id, struct list_head *secrets) { //функция нахождения секрета в списке //function to find secret in list
    struct list_head *pos;

    list_for_each(pos, secrets) {
        secret_t *p = list_entry(pos, secret_t, list_node);
        if (p->secret_id == id) {
            return true;
        }
    }
    return false;
}


//функция записи, вызывается через связку echo, tee
//The write function, called by combination of echo, tee
static ssize_t procfile_write(struct file *file, const char __user *buff, size_t size, loff_t *off) 
{
    int id;
    char command;
    char *secret_data_input = kmalloc(MAX_SECRET_SIZE, GFP_KERNEL);
    char *secret_buffer = kmalloc(MAX_SECRET_SIZE, GFP_KERNEL);
    if (!secret_data_input||!secret_buffer) {
        pr_crit("Memory error, probably out of memory!");
        kfree(secret_data_input);
        kfree(secret_buffer);
        return -ENOMEM;
    }
    struct list_head *pos;
    struct list_head* tmp;
    bool deleted = false;
    newsecret_size = size;

    // Check if the size of incoming data exceeds the maximum allowed size
    // Проверка, не превышает ли входная информация максимальный предел 
    if (newsecret_size > MAX_SECRET_SIZE){
        kfree(secret_data_input);
        kfree(secret_buffer);
        pr_err("secret size = %lu, secret size must be lower than %i bytes!",newsecret_size,MAX_SECRET_SIZE);
        return -ENOMEM;
    }

    // Copy data from user space to kernel space //копирование пользовательского ввода из userspace
    if (copy_from_user(secret_buffer, buff, newsecret_size)){
        printk(KERN_ERR "Failed to copy user input\n");
        return -EFAULT; 
    }
    //data parser //парсер :)
    if (sscanf(secret_buffer, "%c %d %s", &command, &id, secret_data_input)<2){
        printk(KERN_ERR "Failed to parse input\n");
        return -EFAULT;
    }
    kfree(secret_buffer);
    if (id<MIN_ID||id>MAX_ID){
        pr_err("id = %i, id is limited between %i and %i!",id, MAX_ID,MIN_ID);
        return -EINVAL;
    }
    switch (command) {
        //режим чтения, после пары проверок выделяет память под массив и заполняем его входными данными
        //read mode, after a couple of checks allocates memory for the array and fills it with input data
        case 'W':
            if (next_id >= MAX_SECRETS){
                pr_err("maximum number of secrets exceeded, delete some secrets!");
                return -ENOMEM;
                }
            if (secret_finder(id, &secrets)||(strlen(secret_data_input)<1)){
                pr_err("secret with id = %i already exists!",id);
                return -EINVAL;
                }
            if (id==-1)
            {   
                    for (int z = 0; z < MAX_ID; z++) {
                        if (!secret_finder(z, &secrets)){
                            id=z;
                            break;
                    }
                }
                if (id==-1){
                    pr_err("id can't be created!");
                    return -EINVAL;
                }
                }
            secret_t* new_secret = (secret_t*)kmalloc(sizeof(secret_t), GFP_KERNEL);
            new_secret->secret_id = id;
            new_secret->secret_data = kmalloc(MAX_SECRET_SIZE, GFP_KERNEL);
            if (!new_secret) {
                pr_crit("Memory error, probably out of memory!");
                return -ENOMEM;
            }
            strscpy(new_secret->secret_data, secret_data_input, MAX_SECRET_SIZE);        
            list_add_tail(&new_secret->list_node, &secrets);
            next_id++;
            kfree(secret_data_input);
            return newsecret_size;
        //режим чтения, выставляет режим функции чтения, либо какая то кокретная запись, либо все (-1)
        //read mode, sets the mode of the read function, either a specific record or all (-1)
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
            pr_err("no secret with id = %i",id);
            return -EINVAL;
        //режим удаления, работает через систему макросов и чистит память удаленного элемента
        //delete mode, works through the macro system and cleans the memory of the deleted item
        case 'D':
            if (next_id<1){
                pr_err("secret id must be positive");
                return -EINVAL;
                }
            list_for_each_safe(pos, tmp, &secrets) {
                secret_t* p = list_entry(pos, secret_t, list_node);
                if (p->secret_id == id||id==-1) {
                    list_del(pos);
                    kfree(p->secret_data);
                    kfree(p);
                    deleted=true;
                }
            }
            if (deleted==true){
            next_id--;
            return newsecret_size;
            }
            pr_err("no secret with id = %i",id);
            return -EINVAL;
        default:
            pr_err("no operation was set or unknown operation");
            return -EINVAL;
    }
}

//system calls handler //обработчик системных вызовов
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

//функция инициализации, банально создает файл в procfs
//initialization function, trivially creates a file in procfs
static int __init procfs2_init(void)
{
    storage_filename = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (NULL == storage_filename) {
    proc_remove(storage_filename);
    pr_crit("Error:Could not initialize /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
    }
    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

//функция завершения работы - удаляет файл из procfs
//shutdown function - deletes file from procfs
static void __exit procfs2_exit(void)
{
    proc_remove(storage_filename);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
}



module_init(procfs2_init);
module_exit(procfs2_exit);
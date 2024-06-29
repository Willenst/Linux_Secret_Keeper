# Журнал Работ

### Техническое задание

1. Написать модуль ядра Linux для хранения секретов. Этот драйвер должен создавать специальный файл в procfs. С помощью этого интерфейса пользователь должен иметь возможность сохранить секрет в ядре (некоторые произвольные данные), прочитать секрет, удалить секрет. Для этого каждой порции секретных данных должен назначаться идентификатор.
2. Добить userspace-программу для тестирования драйвера
3. (По возможности) Собать ядро Linux и свой модуль с поддержкой KASAN, запустить userspace-тест и попробовать обнаружить ошибки доступа к памяти в коде. Если таких нет, то намеренно добавить ошибку доступа к памяти в свой модуль ядра и поймать ее с помощью KASAN.
4. (По возможности) Перевести созданный мной материал на английский язык для донесения до международного опенсорс комьюнити

### Предисловие

Опыта использования что Си, Что Си подобных языков у меня никогда не было, данный проект модуля ядра будет не просто первым моим модулем ядра, а еще и в целом первым моим проектом на Си. Наверное весьма уникальный случай - начать изучать язык с не с базовых задачек а сразу с программирования модулей ядра. Так, что, возможно на создание кода уйдет немного больше времени, чем нужно и будет он не самого высокого качества, но эксперимент получится крайне интересный. Данный журнал одновременно представляет и мое погружение в замечательный мир низкоуровневых языков, действия будут выполнятся поэтапно, с построения базовых механизмов, и последующего дополнения их надстройками по расширению функционала, заодно попрактикуемся в работе с версиями и гитом.

### Часть 1. Хранилище на одну запись. **Linux_Secret_Keeper-v.0.01**

Поскольку procfs хранит в себе не фактические файлы, в привычном смысле этого понимания, а их эмуляцию, вернее даже сказать абстракцию, созданную для удобства и сохранения нервных клеток потенциальных разработчиков, нам нужно создать драйвер, который будет принимать системные вызовы чтения и записи, и возвращать какой то отклик в пользовательское пространство, это будет наш базовый функционал и наша первоочередная задача.

Начнем мы с организации необходимых нам файлов с макросами и определениями, указания общепринятой в сообществе GPL лицензии и своего авторства

```bash
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratochka Vyacheslav");
MODULE_DESCRIPTION("A simple procfs storage module.");
MODULE_VERSION("0.01");
```

Далее - по классике, объявления переменных

```c
#define SECRET_SIZE 128 //максимальный размер секрета
#define PROCFS_NAME "secret_stash" //будущее название "файла" в procfs

static struct proc_dir_entry *storage_filename; //указатель под наш файл
static char storage[PROCFS_MAX_SIZE]; //непосредственно будущая "база"
static unsigned long storage_size = 0; //размер секрета, пока просто объявлен 0

```

Теперь нам необходима функция для чтения данных, все необходимые параметры ей будет передавать имеющийся практически во всех дистрибутивах системный вызов read.

```c
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

```

Разберем данную функцию

```c
if (*offset >= SECRET_SIZE||copy_to_user(buffer, storage, SECRET_SIZE))
```

Фактически это алгоритм для чтения файла, есть проверка для указателя offset, он передается при вызове read и отвечает за текущий считываемый символ, как бы выступая указателем текущей позиции в файле. Когда он дойдет до конца файла - функция прервется, пока этого не произошло - файл считывается. 

Чтение на базовом уровне готово, теперь необходимо записывать

```c
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

```

Данная функция также принимает все необходимые параметры от системных вызовов, тригернуть ее можно например следующей командой

```c
echo "secret" | tee /proc/secret_stash
```

В ней присутствует небольшая проверка на то, что подаваемый секрет не превышает допустимый размер, идет копирование входных данных из юзерспейса в наше хранилище, а также добавление символа конца файла `\0` в конец строки, в последствии логику нужно будет переделать, но как временное решение - вариант неплохой. 

 Далее, небольшой технический модуль под прием системных вызовов и направления их в соответствующие функции

```c
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};
```

Теперь нам необходим функционал инициализации

```c
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
```

Он будет создавать сущность в procfs, удалять ее в случае, если процесс создания завершился ошибкой и возвращать 0 в случае успеха

Также нам необходим завершающий модуль

```c
static void __exit procfs2_exit(void)
{
    proc_remove(storage_filename);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
    printk(KERN_INFO "Goodbye, World!\n");
}
```

тут все достаточно прост - удаление нашего драйвера и вывод сообщения об этом, в конце кода добавляем следующую конструкцию и наша первая вариация можно сказать готова

```c
module_init(procfs2_init);
module_exit(procfs2_exit);
```

Работать с текущим кодом можно следующим образом

```bash
echo "secret" | tee /proc/secret_stash #запись
cat /proc/secret_stash #чтение
```
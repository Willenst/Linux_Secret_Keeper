# Linux_Secret_Keeper / RU

Итак, вам надоели эти пресловутые и хранящиеся на диске блокноты, которые вечно читают все, кто не попадя? Может вы просто мечтали организовать свой собственный тайник в пространстве ядра? Linux_Secret_Keeper - решение всех ваших проблем, этот инновационный модуль ядра позволит вам писать все свои секреты напрямую в память ядра, делая их максимально изолированными и недоступными! Данный проект пока находится на первичной стадии своей разработки. Версия v0.08 поддерживает запись нескольких строк данных в ядро (Оперативную память) и ее последующее чтение, данным можно присвоить чиcленные идентификаторы, в пределе от 0 до 30000, их можно читать по отдельности, читать все сразу, удалять по идентификатору. Также к данном проекту будет прилагаться журнал работ, служащий эдаким "гайдом" по созданию - journal.md.

### Дисклеймер:
Рекомендую после установки модуля запустить тесты, как это сделано - можно посмотреть ниже. Тесты могут не покрывать весь набор всевозможных ситуаций, в случае обнаружения непридвиденного поведения - создайте ISSUE, использовать драйвер на свой страх и риск.

### Установка:

```bash
git clone https://github.com/Willenst/Linux_Secret_Keeper
cd Linux_Secret_Keeper
make
insmod Linux_Secret_Keeper.ko
```

### Удаление ненужного вам журнала работ:
```
rm journal.md; rm -rf Images
```

### Использование:

```bash
echo "W 0 secret" | tee /proc/secret_stash #запись 0-й строки данных.
echo "R 0" | tee /proc/secret_stash #выбор строки для чтения.
echo "D 0" | tee /proc/secret_stash #выбор строки для удаления.
cat /proc/secret_stash #чтение данных, в зависимости от режима чтения. Если -1 - чтение всего, если n - чтение n-й записи, где n - выбраанная вами запись. 
```

### Очистка загруженного модуля:

```bash
rmmod Linux_Secret_Keeper
make clean
```

### Тестирование. Необходимые пакеты:

У вас должен присуствовать Python 3-й версии и модуль pytest
```bash
pip install pytest
```

### Тестирование. Запуск тестов:

```bash
py.test -s -v test.py
```

### Обработка ошибок.

В случае ошибок, смотрите лог ядра для более детальной информации:
```
dmesg
```

# Linux_Secret_Keeper / EN

So, are you tired of all these disk-stored notebooks that are always being read by everyone? Maybe you've always dreamed of organizing your own secret storage inside the Linux kernel space? Linux_Secret_Keeper - the solution to all your problems. This innovative kernel module will allow you to write all the secrets directly into kernel memory, making them as isolated and inaccessible as possible! This project is currently under development but already has some working functionality. Version v0.08 supports multi-string writing in kernel memory, reading from there, and deleting data, all of which can be assigned and manipulated with an individual ID (from 0 to 30000). Also, there is a journal in russian about the working process - journal.md, if someone will find it interesting, i can translate it one day!

### Disclainer:
I hardly recomend you to run tests after installing, the guide is below. Tests are not ideal and can not cower all the situations, if you find any mistakes, please, create an ISSUE. Use at your own risk!

### Installation:

```bash
git clone https://github.com/Willenst/Linux_Secret_Keeper
cd Linux_Secret_Keeper
make
insmod Linux_Secret_Keeper.ko
```

### Removing journal:
```
rm journal.md; rm -rf Images
```

### Usage:

```bash
echo "W 0 secret" | tee /proc/secret_stash # Writing data to the 0th line.
echo "R 0" | tee /proc/secret_stash # Selecting the line for reading.
echo "D 0" | tee /proc/secret_stash # Selecting the line for deletion.
cat /proc/secret_stash # Reading data, depending on the read mode. If -1, reads all; if n, reads the nth entry you've chosen.
```

### Cleaning up:

```bash
rmmod Linux_Secret_Keeper
make clean
```

### Testing. Requered packages:

Python version 3, pytest
```bash
pip install pytest
```

### Testing. Running tests:

```bash
py.test -s -v test.py
```

### Errors handling.

in case of errors, check kernel log for more detailed info:
```
dmesg
```
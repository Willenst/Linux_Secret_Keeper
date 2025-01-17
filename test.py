import os
import pytest
import subprocess
import time 

# функционал под выполнение команды и возврат ошибок
# functionality for executing commands and returning errors
def write(id, data):
    cmd = f'echo "W {id} {data}" | tee /proc/secret_stash'
    res = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = res.communicate()
    return (out + err).decode()

def read_mode(id):
    cmd = f'echo "R {id}" | tee /proc/secret_stash'
    res = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = res.communicate()
    return (out + err).decode()

def delete(id):
    cmd = f'echo "D {id}" | tee /proc/secret_stash'
    res = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = res.communicate()
    return (out + err).decode()

def read():
    cmd = 'cat /proc/secret_stash'
    res = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = res.communicate()
    return (out + err).decode()

# фикстура на очистку данных перед каждым тестом
# fixture to clean data before each test
@pytest.fixture(scope="class", autouse=True)
def flush_module():
    os.system('rmmod Linux_Secret_Keeper')
    time.sleep(0.1) #небольшая задержка на случай рассинхронов
    os.system('insmod Linux_Secret_Keeper.ko')

# каждому тесту прилагается свой набор параметров
# each test has its own set of parameters
@pytest.mark.parametrize("W_input_id, W_input_data, W_input_result", [
    ['1','123abc','W 1 123abc'], # проверка успешного ввода и вывода первой операции # checking successful input and output of the first operation
    ['1','123abc','W 1 123abc\ntee: /proc/secret_stash: Invalid argument\n']# режим поломки # error mode
])
class Test_group_0:
    def test_input_fucntional(self, W_input_id, W_input_data, W_input_result):
        assert W_input_result in write(W_input_id, W_input_data)

@pytest.mark.parametrize("WR_input_id, WR_input_data, R_input_id, WR_input_result", [
    ['1','123abc','-1','123abc'], # вводим данные, выбираем режим чтения, смотрим итог # input data, choose read mode, check result
    ['2','321cbd','-1','1. 123abc\n2. 321cbd'],
    ['3','testtest','3','3. testtest'],
    ['breakmode','breakmode','234','R 234\ntee: /proc/secret_stash: Invalid argument\n'] # проверка на чтение несуществующего пункта # check reading non-existent item
])
class Test_group_1: #класс не в привычном понимании класcа, надстройка для корректной очистки # class not in the usual class sense, enhancement for proper cleanup
    def test_input_output_fucntional(self, WR_input_id, WR_input_data, R_input_id, WR_input_result):
        if WR_input_id == 'breakmode':
            assert WR_input_result in read_mode(R_input_id)
            return

        write(WR_input_id, WR_input_data)
        read_mode(R_input_id)
        assert WR_input_result in read()

@pytest.mark.parametrize("WD_input_id, WD_input_data, D_input_id, WD_input_result", [
    ['1','123abc','preparing','preparing'], # готовим нагрузку # preparing payload
    ['2','321cbd','preparing','preparing'],
    ['3','testtest','2','1. 123abc\n3. testtest'], # удаляем и смотрим, что все затерлось # delete and check that everything is erased
    ['breakmode','breakmode','3512','D 3512\ntee: /proc/secret_stash: Invalid argument\n'] # режим поломки  # error mode
])
class Test_group_2:
    def test_input_delete_fucntional(self, WD_input_id, WD_input_data, D_input_id, WD_input_result):
        if WD_input_id == 'breakmode':
            assert WD_input_result in delete(D_input_id)
            return
        write(WD_input_id, WD_input_data)
        if WD_input_result != 'preparing':
            delete(D_input_id)
            assert WD_input_result in read()


class Test_group_3:
    def test_input_overflow(self):
        assert write(1,'1'*150) == 'W 1 '+'1'*150+'\ntee: /proc/secret_stash: Cannot allocate memory\n' #попытка превысить длинну секрета # attempt to exceed secret length
        assert write(-2,'1'*5) == 'W -2 '+'1'*5+'\ntee: /proc/secret_stash: Invalid argument\n' #попытка создать запись вне массива # attempt to create entry outside array
        assert write(35000,'1'*5) == 'W 35000 '+'1'*5+'\ntee: /proc/secret_stash: Invalid argument\n' 
        for i in range(1,15):
             write(i, i)
        assert write("16", '16') == 'W 16 16\ntee: /proc/secret_stash: Cannot allocate memory\n' #попытка записи сверх лимита # attempt to write beyond limit
        assert '1. 1\n2. 2\n3. 3\n4. 4\n5. 5\n6. 6\n7. 7\n8. 8\n9. 9\n10. 10\n' in read() #проверка что корректные записи сохранены, некорректных нет # check that correct entries are saved, incorrect ones are not

class Test_group_4:
    def test_automatization(self):
        write(2, 123123) #проверка корректности при наличии сторонней записи #check errors in case there is already a secret
        for i in range(0,3):
             write(-1, str(i)*10) #проверка работоспособности автоматического выбора id #check writing id automatization
        assert '2. 123123\n0. 0000000000\n1. 1111111111\n3. 2222222222\n' in read()
        delete(-1) #проверка удаления всех записей
        assert ('123123' or '0000000000' or '1111111111' or '2222222222') not in read()


class Testflush: #мелкая надстройка под очитку файла в proc # small enhancement to cleanup proc entry
    def test_flush(self):
        assert 1==1
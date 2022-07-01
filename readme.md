# Big File Sorter (BFS-9000)
Utility to sort values in large files (ascending order). Just for fun.


## Usage
```
Usage: bigsort [infile] [outfile] [mem_limit]
[infile]	- input file. Default = 'input'
[outfile]	- ouput file. Default = 'output'
[mem_limit]	- size of buffer for reading, bytes.
		  1000...120000000, Default = 64000000
Order of optional parameters is IMPORTANT.
```

## Restrictions

Maximum file size: 256TB (NTFS), 16TB (ext3)  
Maximum file streams per process: 512, up to 2048  (Windows), 1024 (most Linux), 256 (MacOS)  
Allowed maximum memory footprint: 128 MB

Effective max input file size ~= 500 * 120000000 == 60 GB

## How to build

### Windows
```
g++ -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort.cpp -o Release\bigsort.exe
```
or batch build
```
build_release.bat
```

### Linux
```
g++ -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort.cpp -o Release\bigsort
```
or batch build
```
./build_release.sh
```

## How to test
About 30GB of free disk space is required for passing all the tests.

### Windows
```
run_tests.bat
```  
For generating random files please download free dummycmd.exe utility at http://www.mynikko.com/dummy/

### Linux
```
./run_tests.sh
```


## Tested configurations
- i7-2630QM/16GB/1TB hdd(+TrueCrypt)  
  Windows 7 x64, g++ (Rev2, Built by MSYS2 project) 12.1.0

- i5-11400H @ 2.70GHz/8GB/512 NVMe  
  Ubuntu 22.04 LTS, g++ (Ubuntu 11.2.0-19ubuntu1) 11.2.0

- Core 2 Duo / 4GB  
  MacOS Sierra, Apple LLVM version 9.0.0 (clang-900.0.39.2)


## To Do

+ Add multithreading support (?)
+ Add better command line parser

## Original Scope of Work (in Russian)
Тестовое задание:
Необходимо написать программу, которая будет сортировать по возрастанию большой файл беззнаковых 32-х разрядных целых чисел.

При этом:
- Размер входного файла значительно больше объема доступной оперативной памяти, которой есть всего 128 Мб.
- Числа в файле записаны в бинарном виде.
- Есть достаточно дискового пространства для хранения результата сортировки в отдельном файле и для хранения промежуточных результатов.
- Программа будет компилироваться при помощи g++-5.3.0 с опциями -std=c++14 -D_NDEBUG -O3 -lpthread.
- Файлы будут находиться на SSD диске. На компьютере стоит многоядерный процессор.
- Входной файл будет находиться в той же директории что и исполняемый файл и будет называться input. Мы ожидаем там же увидеть отсортированный файл с именем output.
- Из вашей программы должен получиться исполняемый файл, а значит нужно чтобы в ней была функция main.
- Никаких дополнительных библиотек на компьютере не установлено. (Например, нет boost).
- Решение должно быть кросс-платформенным.
- Входной файл менять нельзя.

Рекомендации: не стоит писать свои собственные методы сортировки, вполне подойдет std::sort.
Не стоит излишне усложнять решение.


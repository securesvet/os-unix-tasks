#!/bin/bash

set -e

make clean
make

echo "Запуск тестов..." > result.txt

echo "Генерируем тестовые файлы..."

./generate_test_file

echo "===Test 1: Создание спарс файла B из A===" >> result.txt
./main -i fileA -o fileB
ls -lh fileA fileB >> result.txt

gzip -k fileA
gzip -k fileB

echo "===Test 2: Сжатие файлов A и B===" >> result.txt
ls -lh fileA.gz fileB.gz >> result.txt

echo "===Test 3: Распаковка из B.gz в C через main.c ===" >> result.txt
gzip -cd fileB.gz | ./main -o fileC
ls -lh fileC >> result.txt

echo "===Test 4: Копируем из A в D с размером блока по 100 байт===" >> result.txt
./main -i fileA -o fileD -b 100
ls -lh fileD >> result.txt

echo "Размеры файлов:" >> result.txt
stat --format="%n: %s bytes (%b blocks)" fileA fileA.gz fileB fileB.gz fileC fileD >> result.txt

echo "Все тесты прошли успешно!" >> result.txt

echo "Очистка..."

rm -f main generate_test_file fileA fileA.gz fileB fileB.gz fileC fileD
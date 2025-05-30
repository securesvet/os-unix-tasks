#!/bin/bash

IN=/tmp/io
OUT=/tmp/out
CONF=/tmp/myconf
LOGS=/tmp/myinit.log

echo "Build myinit"
make

echo "Создание IO files"
touch $IN
touch $OUT

echo "Создаем файл конфигурации"
touch $CONF

echo "Завершение остальных процессов myinit"
killall myinit || echo ""

echo /bin/sleep 3 $IN $OUT >/tmp/myconf
echo /bin/sleep 5 $IN $OUT >>/tmp/myconf
echo /bin/sleep 10 $IN $OUT >>/tmp/myconf

echo "Запуск myinit"
./myinit -c $CONF
MYINIT_PID=$(pgrep myinit)
echo $MYINIT_PID
sleep 1

echo "Дочерние процессы (ожидается 3)..." >result.txt
ps --ppid $MYINIT_PID -o cmd,pid >>result.txt

pkill -f "/bin/sleep 5"

sleep 1

echo "Дочерние процессы после убийства второго (ожидается 3)..." >>result.txt
ps --ppid $MYINIT_PID -o cmd,pid >>result.txt

echo "Ожидание завершения остальных процессов" >>result.txt
sleep 10

echo /bin/sleep 3 $IN $OUT >/tmp/myconf

killall -HUP myinit

echo "Дочерние процессы (ожидается 1)..." >>result.txt

ps --ppid $MYINIT_PID -o cmd,pid >>result.txt

echo "" >>result.txt
echo "========LOGS========" >>result.txt
echo "" >>result.txt
cat $LOGS >>result.txt

killall myinit

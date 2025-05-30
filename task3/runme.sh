#!/bin/bash

echo "Build myinit"
make

echo "Создание IO files"
touch /tmp/in
touch /tmp/out

echo "Создаем файл конфигурации"
touch /tmp/myconf

echo "Завершение остальных процессов myinit"
killall myinit

echo /bin/sleep 3 /tmp/in /tmp/out >/tmp/myconf
echo /bin/sleep 5 /tmp/in /tmp/out >>/tmp/myconf
echo /bin/sleep 10 /tmp/in /tmp/out >>/tmp/myconf

echo "Запуск myinit"
./myinit -c /tmp/myconf
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

echo /bin/sleep 3 /tmp/in /tmp/out >/tmp/myconf

killall -HUP myinit

echo "Дочерние процессы (ожидается 1)..." >>result.txt

ps --ppid $MYINIT_PID -o cmd,pid >>result.txt

echo "" >>result.txt
echo "========LOGS========" >>result.txt
echo "" >>result.txt
cat /tmp/myinit.log >>result.txt

killall myinit

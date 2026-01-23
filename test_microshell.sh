#!/bin/bash

MICROSHELL=./microshell

echo "=== 测试 1: cd 参数错误 ==="
$MICROSHELL cd 2> err.txt
cat err.txt
echo

echo "=== 测试 2: cd 多参数 ==="
$MICROSHELL cd src extra 2> err.txt
cat err.txt
echo

echo "=== 测试 3: cd + 管道 ==="
$MICROSHELL cd test "|" /bin/echo hello 2> err.txt
cat err.txt
echo

echo "=== 测试 4: 普通命令执行 ==="
$MICROSHELL /bin/echo test
echo

echo "=== 测试 5: 管道命令 ==="
$MICROSHELL /bin/echo hello "|" /usr/bin/wc -c
echo

echo "=== 测试 6: 分号组合 ==="
$MICROSHELL /bin/echo first ";" /bin/echo second ";" /bin/echo third
echo

echo "=== 测试 7: cd 正确执行 ==="
mkdir -p tmpdir
$MICROSHELL cd tmpdir
pwd
rmdir tmpdir
echo

echo "=== 测试 8: 执行不存在的命令 ==="
$MICROSHELL /bin/nonexistent 2> err.txt
cat err.txt
echo

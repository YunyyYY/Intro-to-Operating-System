#!bin/bash
gcc -lpthread main.c mapreduce.c -o main -Wall -Werror
args=$(<rin)
./main $args

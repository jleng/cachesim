#!/bin/sh
TEST1=gcc_25M.trace
TEST2=dealII_25M.trace
TEST3=mcf_25M.trace
TEST4=soplex_25M.trace
./cachesim 4 32 1024 4 4 4 4 $TEST1 $TEST2 $TEST3 $TEST4 > dump.test1
#./cachesim 4 32 1024 4 4 4 4 $TEST1 empty.trace empty.trace empty.trace > dump.test1000
#./cachesim 4 32 1024 4 4 4 4 empty.trace $TEST2 empty.trace empty.trace > dump.test0100
#./cachesim 4 32 1024 4 4 4 4 empty.trace empty.trace $TEST3 empty.trace > dump.test0010
#./cachesim 4 32 1024 4 4 4 4 empty.trace empty.trace empty.trace $TEST4 > dump.test0001
#./cachesim 4 32 1024 2 2 4 8 $TEST1 $TEST2 $TEST3 $TEST4 > dump.test2
./cachesim 1 32 1024 4 $TEST1  > dump.test1.multicache
#./cachesim 1 32 1024 4 $TEST2  > dump.test2.multicache
#./cachesim 1 32 1024 4 $TEST3  > dump.test3.multicache
#./cachesim 1 32 1024 4 $TEST4  > dump.test4.multicache
#./cachesim 4 32 1024 4 4 4 4 $TEST4 $TEST3 $TEST2 $TEST1 > dump.test2

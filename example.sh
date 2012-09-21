#!/bin/sh
TEST1=gcc_25M.trace
TEST2=dealII_25M.trace
TEST3=mcf_25M.trace
TEST4=soplex_25M.trace
./cachesim 4 32 1024 4 4 4 4 $TEST1 $TEST2 $TEST3 $TEST4 > dump.test1
#./cachesim 1 32 1024 4 $TEST1  > dump.test1.multicache
#./cachesim 1 32 1024 4 $TEST2  > dump.test2.multicache
#./cachesim 1 32 1024 4 $TEST3  > dump.test3.multicache
#./cachesim 1 32 1024 4 $TEST4  > dump.test4.multicache
#./cachesim 4 32 1024 4 4 4 4 $TEST4 $TEST3 $TEST2 $TEST1 > dump.test2

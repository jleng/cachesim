#!/bin/bash
TRACE_GEN=0
RUN=1
ARRAY_SIZE=8192
rm -rf result.csv
for i in `seq 1 1 8`;
#for i in `seq 1 1 1`;
	do
		echo $ARRAY_SIZE
		let ARRAY_SIZE=$ARRAY_SIZE*2
		if [ $TRACE_GEN -eq 1 ];then
			./autotest $ARRAY_SIZE
		fi

		if [ $RUN -eq 1 ];then
			let ARRAY_SIZE_KB=$ARRAY_SIZE/1024
			echo "${ARRAY_SIZE_KB}KB"
			echo "${ARRAY_SIZE_KB}KB" > Array_${ARRAY_SIZE_KB}KB.csv

			for j in `seq 0 1 30`; 
			do
				file=./traces/array${ARRAY_SIZE_KB}KB_stride${j}.trace
				#echo $file
				if [ -f $file ];then
					echo $file
					./singlecache $file > dump.tmp
					avglatency=`cat dump.tmp | grep AMAT | awk '{print $8}'`
					missrate=`./cachesim $file | awk -F' ' '{print $5}' | tail -n 1`;
					echo "$avglatency" >> Array_${ARRAY_SIZE_KB}KB.csv
				fi
			done
		fi
		#./cachesim 
	done

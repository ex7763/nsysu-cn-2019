#!/bin/bash

PORT=13131

make clean all && (./server $PORT &)

for i in {1..20};
do
    file_num=$(( (RANDOM % 4) + 1))
    ./client $PORT "$file_num"  "file_""$i""_""$file_num"_"$(( (RANDOM % 10000))).mp4" &
    sleep 0.05
done

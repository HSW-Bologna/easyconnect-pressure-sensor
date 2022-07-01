#!/usr/bin/env sh

PROJECT=easyconnect-peripheral
DIR=${PROJECT}_v$1_$2_$3
mkdir /tmp/$DIR
cp ./build/bootloader/bootloader.bin /tmp/$DIR
cp ./build/partition_table/partition-table.bin /tmp/$DIR
cp ./build/$PROJECT.bin /tmp/$DIR
cd /tmp
tar -czf $DIR.tar.gz $DIR

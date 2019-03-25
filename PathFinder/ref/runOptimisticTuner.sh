#!/bin/bash
FILE=benchmark.ot
OUTPUT_DIR=optimistic
# Bash way to get out all files names to be copied
ANNOTED=searchAlgorithms.c

MODEL=$(lscpu | grep "Model name:" | awk '{$1=$2=""; print $0 }')
mkdir $OUTPUT_DIR

echo $MODEL > $OUTPUT_DIR/machine.info

EXEC=$(grep "\"executable\":"  $FILE | cut -d '"' -f4)

#echo $EXEC
make 
mv $EXEC $OUTPUT_DIR/${EXEC}.original
#grep for exec and mv to optimistic with .original

make clean

python3.7 /gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/optimistic_llvm/optimistic_tuner/optimistic_tuner.py    benchmark.ot 2>  optimistic/stdout.log
#cp optimistic/stdout.log $OUTPUT_DIR

TMP=$(head -1 ${OUTPUT_DIR}/stdout.log | awk '{ print $14 }')

echo $TMP
cp -r $TMP $OUTPUT_DIR/ 

# grep for annotated files and cp it
cp $ANNOTED $OUTPUT_DIR/${ANNOTED}.optimistic

make
mv $EXEC $OUTPUT_DIR/${EXEC}.optimistic
make clean
#TODO: write analysis scripts



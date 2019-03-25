#!/bin/bash
FILE=benchmark.ot
OUTPUT_DIR=optimistic
# Bash way to get out all files names to be copied
ANNOTED=operators.ompif.c

MODEL=$(lscpu | grep "Model name:" | awk '{$1=$2=""; print $0 }')
mkdir $OUTPUT_DIR

echo $MODEL > $OUTPUT_DIR/machine.info

EXEC=$(grep "\"executable\":"  $FILE | cut -d '"' -f4)

#echo $EXEC
/gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/build/bin/clang -O3 -march=native -g -D__PRINT_NORM -o miniGMG box.c mg.c miniGMG.c operators.ompif.c solver.c timer.c -lm 
mv $EXEC $OUTPUT_DIR/${EXEC}.original
#grep for exec and mv to optimistic with .original


python3.7 /gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/optimistic_llvm/optimistic_tuner/optimistic_tuner.py    benchmark.ot 2>  optimistic/stdout.log
#cp optimistic/stdout.log $OUTPUT_DIR

TMP=$(head -1 ${OUTPUT_DIR}/stdout.log | awk '{ print $14 }')

echo $TMP
cp -r $TMP $OUTPUT_DIR/ 

# grep for annotated files and cp it
cp $ANNOTED $OUTPUT_DIR/${ANNOTED}.optimistic

/gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/build/bin/clang -O3 -march=native -g -D__PRINT_NORM -o miniGMG box.c mg.c miniGMG.c operators.ompif.c solver.c timer.c -lm
mv $EXEC $OUTPUT_DIR/${EXEC}.optimistic

#TODO: write analysis scripts



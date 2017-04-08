#!/bin/bash

export DATA_SIZE=$1
export INPUT_DATA=$2
export OUTPUT_DATA=$3
gcc data_filter.c -o ./compiled_filter_data
./compiled_filter_data $DATA_SIZE $INPUT_DATA $OUTPUT_DATA

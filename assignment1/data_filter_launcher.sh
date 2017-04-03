#!/bin/bash


gcc data_filter.c -o ./compiled_filter_data
echo $DATA_SIZE $INPUT_DATA $OUTPUT_DATA
./compiled_filter_data $DATA_SIZE $INPUT_DATA $OUTPUT_DATA

#!/bin/bash

gcc data_filter.c -o ./compiled_filter_data
./compiled_filter_data $DATA_SIZE $INPUT_DATA $OUTPUT_DATA

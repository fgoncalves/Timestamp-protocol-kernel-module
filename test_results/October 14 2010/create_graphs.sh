#!/bin/bash

for file in ./*.converted.data
do
    FILE=`basename $file`
    OUT="${FILE}.png"
    sed -e "s/@SED_OUTPUT_REPLACE@/${OUT}/g" -e "s/@SED_FILE_REPLACE@/${FILE}/g" < plot.gnuplot > "temp.gnuplot"
    gnuplot "temp.gnuplot"
done
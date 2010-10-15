set terminal png
set output "@SED_OUTPUT_REPLACE@"
plot "@SED_FILE_REPLACE@" using 2:1 with lines
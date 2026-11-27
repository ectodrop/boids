#!/bin/bash
set -e

# Define the data file prefix and the gnuplot script
N=$1
DATA_PREFIX=$2
OUTPUT_FILE=$3
GNUPLOT_SCRIPT="$(dirname "$0")/animation.gnuplot"

# Check if the gnuplot script exists
if [[ ! -f $GNUPLOT_SCRIPT ]]; then
  echo "Gnuplot script '$GNUPLOT_SCRIPT' not found."
  exit 1
fi

# Find all files with the specified prefix
find . -type f -name "${DATA_PREFIX}*" -printf "%f %s\n" | sort -V | parallel --colsep ' ' gnuplot -c $GNUPLOT_SCRIPT {1} {2} {1}.gif $N

# echo ${DATA_PREFIX}-*.gif

convert ${DATA_PREFIX}*.gif $OUTPUT_FILE

rm -f ${DATA_PREFIX}*.gif ${DATA_PREFIX}*.bin 

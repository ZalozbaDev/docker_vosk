#!/bin/bash

echo "SRT translation called with args $1 $2 $3 $4 $5"

python3 translate_srt_sotra_lsf.py "$1" "$2" "$3" "$4" "$5"

echo "SRT translation DONE"


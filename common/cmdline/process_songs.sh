#!/bin/bash

INDIR=$1

# handle spaces in filenames

export OLDIFS=$IFS
IFS=$'\n'

TMPINFILE=tmpaudio_for_recognition.mp3

for i in $(find $INDIR -name "*.mp3"); do
	OUTFILE=$(echo $i | sed -e 's/\.mp3/\.txt/')
	echo $i" --> "$OUTFILE
	
	rm -f $TMPINFILE
	cp "$i" $TMPINFILE
	
	./demucs.sh $TMPINFILE
	
	VOCALSFILE=$(find tmpout/ -name "vocals.mp3")
	
	./cmdline_example_transcripts_txtonly.sh $VOCALSFILE
	
	TRANSCRIPTFILE=$(find tmpout/ -name "vocals.txt")
	
	cp $TRANSCRIPTFILE "$OUTFILE"
	
	echo "$OUTFILE done"
	echo
	
done

export IFS=$OLDIFS

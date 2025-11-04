#!/bin/bash

INDIR=$1

# handle spaces in filenames

export OLDIFS=$IFS
IFS=$'\n'

TMPINFILE=tmpaudio_for_recognition.mp3

for i in $(find $INDIR -name "*.wav"); do
	OUTFILE_TRANSCRIPT=$(echo $i | sed -e 's/\.wav/\.txt/')
	OUTFILE_SUBTITLES=$(echo $i | sed -e 's/\.wav/\.srt/')
	OUTFILE_VOCALS=$(echo $i | sed -e 's/\.wav/\.voc\.mp3/')
	OUTFILE_NOVOCALS=$(echo $i | sed -e 's/\.wav/\.novoc\.mp3/')
	echo $i" --> "$OUTFILE_TRANSCRIPT" | "$OUTFILE_SUBTITLES" | "$OUTFILE_VOCALS" | "$OUTFILE_NOVOCALS
	
	rm -f $TMPINFILE
	cp "$i" $TMPINFILE
	
	./0999_demucs.sh $TMPINFILE
	
	VOCALSFILE=$(find tmpout/ -name "vocals.mp3")
	NOVOCALSFILE=$(find tmpout/ -name "no_vocals.mp3")
	
	./1999_recognize.sh $VOCALSFILE
	
	TRANSCRIPTFILE=$(find tmpout/ -name "vocals.txt")
	SUBTITLESFILE=$(find tmpout/ -name "vocals.srt")
	
	cp $TRANSCRIPTFILE "$OUTFILE_TRANSCRIPT"
	cp $SUBTITLESFILE  "$OUTFILE_SUBTITLES"
	cp $VOCALSFILE     "$OUTFILE_VOCALS"
	cp $NOVOCALSFILE   "$OUTFILE_NOVOCALS"
	
	echo "$OUTFILE done"
	echo
	
done

export IFS=$OLDIFS

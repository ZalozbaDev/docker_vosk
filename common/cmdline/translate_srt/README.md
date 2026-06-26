# translate SRT

Run the "sotra_lsf" container:

docker run -v ./models1:/app/models1 -p 3000:3000  -it sotra-lsf 

The shell script needs 4 parameters. Check the python script.

Example:

./translate_srt.sh ./hsb.srt ./de.srt hsb de http://localhost:3000/translate




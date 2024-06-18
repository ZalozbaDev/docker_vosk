# docker_vosk

Examples for using vosk with docker.

## vosk_server_dummy

Minimal example that prints out usage of the VOSK API. No other functionaliy.

To build the example, change into the directory and check the Dockerfile comments for build instructions.

## vosk_server_dlabpro

Combines the open source "dlabpro" speech recognition system with the VOSK API to
create a recognition system with simple (explicit or statistical) grammar.
Models still need to be provided externally.

To build the example, change into the directory and check the Dockerfile comments for build instructions.

## vosk_server_recikts

This implementation uses common code, so it needs to be built from the root directory:

```code

docker build -f vosk_server_recikts/Dockerfile --progress=plain -t vosk_server_recikts .

```

Check the Dockerfile comments for more build options.

## vosk_server_whisper

This implementation uses common code, so it needs to be built from the root directory:

```code

docker build -f vosk_server_whisper/Dockerfile --progress=plain -t vosk_server_whisper .

```

## Full vosk_server_whisper build

For building vosk_server_whisper together with the model and deploy it as part of a full jitsi installation you can clone the [mudrowak](https://github.com/ZalozbaDev/mudrowak) repo, which includes all required dependencies as submodules (including this `docker_vosk` repo).
See the readme there for instructions.

## Authors

- Dr. Frank Duckhorn (Fraunhofer Institute for Ceramic Technologies and Systems IKTS, Dresden, Germany)

- Daniel Sobe (Foundation for the Sorbian people)

## License

See file "LICENSE".

import json
import os
import sys
import asyncio
import websockets
import logging
import sounddevice as sd
import argparse
import numpy as np

# Try to import audio reading libraries
try:
    import soundfile as sf
    HAS_SOUNDFILE = True
except ImportError:
    HAS_SOUNDFILE = False

try:
    from scipy.io import wavfile
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

def int_or_str(text):
    """Helper function for argument parsing."""
    try:
        return int(text)
    except ValueError:
        return text

def callback(indata, frames, time, status):
    """This is called (from a separate thread) for each audio block."""
    loop.call_soon_threadsafe(audio_queue.put_nowait, bytes(indata))

def load_audio_file(file_path, target_samplerate):
    """Load audio file and resample to target sample rate."""
    if HAS_SOUNDFILE:
        audio_data, sample_rate = sf.read(file_path)
    elif HAS_SCIPY:
        sample_rate, audio_data = wavfile.read(file_path)
        audio_data = audio_data.astype('float32') / 32768.0  # Normalize if int16
    else:
        raise ImportError("Neither soundfile nor scipy is available. Please install one of them.")
    
    # Handle stereo by converting to mono
    if len(audio_data.shape) > 1:
        audio_data = np.mean(audio_data, axis=1)
    
    # Resample if necessary
    if sample_rate != target_samplerate:
        try:
            from scipy import signal
            num_samples = int(len(audio_data) * target_samplerate / sample_rate)
            audio_data = signal.resample(audio_data, num_samples)
        except ImportError:
            print(f"Warning: scipy not available. Cannot resample from {sample_rate} to {target_samplerate}")
    
    # Normalize and convert to int16
    if audio_data.dtype != np.int16:
        if audio_data.max() <= 1.0 and audio_data.min() >= -1.0:
            audio_data = (audio_data * 32767).astype('int16')
        else:
            # Clip and convert
            audio_data = np.clip(audio_data, -32768, 32767).astype('int16')
    
    return audio_data

async def run_test_mic():

    with sd.RawInputStream(samplerate=args.samplerate, blocksize = int(0.625 * args.samplerate), device=args.device, dtype='int16',
                           channels=1, callback=callback) as device:

        async with websockets.connect(args.uri) as websocket:
            await websocket.send('{ "config" : { "sample_rate" : %d } }' % (device.samplerate))

            while True:
                data = await audio_queue.get()
                await websocket.send(data)
                recieved = json.loads(await websocket.recv())
                if "listening" in recieved:
                    if recieved["listening"]:
                        print("Listening...")
                if "text" in recieved:
                    print(recieved)

            await websocket.send('{"eof" : 1}')
            print (await websocket.recv())

async def run_test_file():
    """Test with audio file instead of microphone."""
    print(f"Loading audio file: {args.audio_file}")
    audio_data = load_audio_file(args.audio_file, args.samplerate)
    
    async with websockets.connect(args.uri) as websocket:
        await websocket.send('{ "config" : { "sample_rate" : %d } }' % (args.samplerate))
        
        blocksize = int(0.625 * args.samplerate)
        # Calculate chunk duration in seconds
        chunk_duration = blocksize / args.samplerate
        
        print(f"Sending audio in {blocksize}-sample chunks ({chunk_duration:.3f}s per chunk)...")
        # Send audio in chunks at real-time speed
        for i in range(0, len(audio_data), blocksize):
            chunk = audio_data[i:i+blocksize]
            await websocket.send(bytes(chunk))
            
            recieved = json.loads(await websocket.recv())
            if "listening" in recieved:
                if recieved["listening"]:
                    print("Listening...")
            if "text" in recieved:
                print(recieved)
            
            # Wait for the chunk duration to simulate real-time audio input
            # (except for the last chunk)
            if i + blocksize < len(audio_data):
                await asyncio.sleep(chunk_duration)
        
        await websocket.send('{"eof" : 1}')
        print(await websocket.recv())

async def main():

    global args
    global loop
    global audio_queue

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-l', '--list-devices', action='store_true',
                        help='show list of audio devices and exit')
    args, remaining = parser.parse_known_args()
    if args.list_devices:
        print(sd.query_devices())
        parser.exit(0)
    parser = argparse.ArgumentParser(description="ASR Server",
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     parents=[parser])
    parser.add_argument('-u', '--uri', type=str, metavar='URL',
                        help='Server URL', default='ws://0.0.0.0:2700')
    parser.add_argument('-d', '--device', type=int_or_str,
                        help='input device (numeric ID or substring)')
    parser.add_argument('-r', '--samplerate', type=int, help='sampling rate', default=16000)
    parser.add_argument('-a', '--audio-file', type=str, metavar='FILE',
                        help='audio file to send to server (instead of microphone)')
    args = parser.parse_args(remaining)
    loop = asyncio.get_running_loop()
    audio_queue = asyncio.Queue()

    logging.basicConfig(level=logging.INFO)
    
    if args.audio_file:
        await run_test_file()
    else:
        await run_test_mic()

if __name__ == '__main__':
    asyncio.run(main())
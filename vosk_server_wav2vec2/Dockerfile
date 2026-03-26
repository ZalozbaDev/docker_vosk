# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set the working directory in the container
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . /app

RUN apt-get update -y && apt-get upgrade -y
RUN apt-get install -y libgomp1 cmake make g++

# Install any needed packages specified in requirements.txt
RUN pip install -r requirements.txt
RUN pip install https://github.com/kpu/kenlm/archive/master.zip

# Download the model from Hugging Face
# RUN python -c "from transformers import AutoModelForCTC, AutoTokenizer; AutoModelForCTC.from_pretrained('Korla/Wav2Vec2BertForCTC-hsb'); AutoTokenizer.from_pretrained('Korla/Wav2Vec2BertForCTC-hsb')"
# Uncomment if you don't want to use onnx

# Expose the port the app runs on
EXPOSE 2700

# Define environment variables
ENV ASR_SERVER_INTERFACE=0.0.0.0
ENV ASR_SERVER_PORT=2700
ENV ASR_MODEL_NAME=Korla/Wav2Vec2BertForCTC-hsb
ENV ASR_SAMPLE_RATE=16000
ENV ASR_VERBOSE_OUTPUT=true

# Run the application
CMD ["python", "asr_server.py"]

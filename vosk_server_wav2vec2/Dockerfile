# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set the working directory in the container
WORKDIR /app

# Copy the current directory contents into the container at /app

RUN apt-get update -y && apt-get upgrade -y
RUN apt-get install -y libgomp1 cmake make g++ hunspell libhunspell-dev

# Install any needed packages specified in requirements.txt
RUN pip install https://github.com/kpu/kenlm/archive/master.zip

COPY . /app
RUN pip install -r requirements.txt

# Download the model from Hugging Face
# RUN python -c "from transformers import AutoModelForCTC, AutoTokenizer; AutoModelForCTC.from_pretrained('Korla/Wav2Vec2BertForCTC-hsb'); AutoTokenizer.from_pretrained('Korla/Wav2Vec2BertForCTC-hsb')"
# Uncomment if you don't want to use onnx

# directories for bind-mounting data into container
RUN mkdir models
RUN mkdir lm
RUN mkdir spell

# Expose the port the app runs on
EXPOSE 2700

# Define environment variables
ENV ASR_SERVER_INTERFACE=0.0.0.0
ENV ASR_SERVER_PORT=2700
ENV ASR_MODEL_NAME=./models/Korla/Wav2Vec2BertForCTC-hsb-0
ENV ASR_SAMPLE_RATE=16000
ENV ASR_VERBOSE_OUTPUT=true
ENV ASR_ONNX=true

# Run the application
CMD ["python", "asr_server.py"]

from elevenlabs import generate
from flask import Flask, request, jsonify
import subprocess
import sys
import time
import audio2face_pb2
import audio2face_pb2_grpc
import grpc
import numpy as np
import soundfile
import io


def convert_mp3_stream_to_wav(audio_stream):
    """
    Converts an MP3 stream to WAV format using FFmpeg.
    """
    ffmpeg_command = ['ffmpeg', '-i', 'pipe:0', '-f', 'wav', '-acodec', 'pcm_s16le', '-ar', '44100', 'pipe:1']
    process = subprocess.Popen(ffmpeg_command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    wav_data, err = process.communicate(input=audio_stream)

    if process.returncode != 0:
        # Print the error output from FFmpeg to help diagnose issues
        print("FFmpeg Error Output:", err.decode())
        raise Exception("FFmpeg conversion failed")

    return wav_data


def generate_and_convert_audio(text):
    """
    Generates an audio stream using ElevenLabs and converts it to WAV format.
    """

    # Call the generate function to get the generator for streaming
    audio_stream_generator = generate(
      text=text,
      stream=True,
      api_key="ELEVEN_LABS_API_KEY"
    )

    # Use the stream function provided by ElevenLabs to properly consume the audio stream
    # Assuming stream() is designed to handle the audio_stream_generator correctly
    # This part needs adjustment based on the actual implementation details of stream()
    audio_stream_data = b""
    for chunk in audio_stream_generator:
        audio_stream_data += chunk

    # Convert the collected MP3 stream to WAV
    wav_data = convert_mp3_stream_to_wav(audio_stream_data)
    return wav_data


def push_audio_track_stream_from_elevenlabs(url, instance_name, text):
    """
    Converts text to audio using ElevenLabs, then sends the audio chunks sequentially via PushAudioStreamRequest().
    """
    wav_data = generate_and_convert_audio(text)
    with io.BytesIO(wav_data) as wav_io:
        data, samplerate = soundfile.read(wav_io, dtype="float32")

    push_audio_track_stream(url, data, samplerate, instance_name)

def push_audio_track(url, audio_data, samplerate, instance_name):
    """
    Pushes the whole audio track at once.
    """
    block_until_playback_is_finished = True
    with grpc.insecure_channel(url) as channel:
        stub = audio2face_pb2_grpc.Audio2FaceStub(channel)
        request = audio2face_pb2.PushAudioRequest()
        request.audio_data = audio_data.astype(np.float32).tobytes()
        request.samplerate = samplerate
        request.instance_name = instance_name
        request.block_until_playback_is_finished = block_until_playback_is_finished
        print("Sending audio data...")
        response = stub.PushAudio(request)
        if response.success:
            print("SUCCESS")
        else:
            print(f"ERROR: {response.message}")
    print("Closed channel")

def push_audio_track_stream(url, audio_data, samplerate, instance_name):
    """
    Pushes audio chunks sequentially.
    """
    chunk_size = samplerate // 10
    sleep_between_chunks = 0.04
    block_until_playback_is_finished = True

    with grpc.insecure_channel(url) as channel:
        print("Channel created")
        stub = audio2face_pb2_grpc.Audio2FaceStub(channel)

        def make_generator():
            start_marker = audio2face_pb2.PushAudioRequestStart(
                samplerate=samplerate,
                instance_name=instance_name,
                block_until_playback_is_finished=block_until_playback_is_finished,
            )
            yield audio2face_pb2.PushAudioStreamRequest(start_marker=start_marker)
            for i in range(0, len(audio_data), chunk_size):
                time.sleep(sleep_between_chunks)
                chunk = audio_data[i:i+chunk_size]
                yield audio2face_pb2.PushAudioStreamRequest(audio_data=chunk.astype(np.float32).tobytes())

        request_generator = make_generator()
        print("Sending audio data...")
        response = stub.PushAudioStream(request_generator)
        if response.success:
            print("SUCCESS")
        else:
            print(f"ERROR: {response.message}")
    print("Channel closed")

def main():
    """
    Main entry for the script, updated to handle dynamic text input.
    """
    if len(sys.argv) < 3:
        print("Usage for WAV file input: python test_client.py file PATH_TO_WAV INSTANCE_NAME")
        print("Usage for ElevenLabs generated input: python test_client.py elevenlabs \"Your text here\" INSTANCE_NAME")
        return

    option = sys.argv[1]
    instance_name = sys.argv[2]
    url = "localhost:50051"  # Adjust as necessary

    if option == "file":
        if len(sys.argv) < 4:
            print("Error: Missing path to WAV file.")
            return
        audio_fpath = sys.argv[3]
        data, samplerate = soundfile.read(audio_fpath, dtype="float32")
        if len(data.shape) > 1:
            data = np.average(data, axis=1)
        push_audio_track_stream(url, data, samplerate, instance_name)
    elif option == "elevenlabs":
        if len(sys.argv) < 2:
            print("Error: Missing text for ElevenLabs audio generation.")
            return

        text = sys.argv[3]  # Capture the text from the command line
        push_audio_track_stream_from_elevenlabs(url, instance_name, text)
    else:
        print("Invalid option. Use 'file' or 'elevenlabs'.")

    # python eleven_labs_client.py file path_to_your_audio_file.wav /World/audio2face/PlayerStreaming

    # python eleven_labs_client.py elevenlabs /World/audio2face/PlayerStreaming "hello world"


# Flask app setup
app = Flask(__name__)

@app.route('/process_audio', methods=['POST'])
def process_audio():
    data = request.json
    option = data.get('option')
    instance_name = data.get('instance_name')
    url = "localhost:50051"  # Adjust as necessary

    if option == "file":
        audio_fpath = data.get('audio_fpath')
        data, samplerate = soundfile.read(audio_fpath, dtype="float32")
        if len(data.shape) > 1:
            data = np.average(data, axis=1)
        push_audio_track_stream(url, data, samplerate, instance_name)
    elif option == "elevenlabs":
        text = data.get('text')
        push_audio_track_stream_from_elevenlabs(url, instance_name, text)
    else:
        return jsonify({"error": "Invalid option. Use 'file' or 'elevenlabs'."}), 400

    # Process and return response
    return jsonify({"message": "Audio processed successfully"}), 200

# Main entry point for the script
if __name__ == '__main__':
    if len(sys.argv) > 1:
        # Command line mode
        main()  # main() should be defined to handle your original script's CLI functionality
    else:
        # Run as a web server
        app.run(debug=True, port=5000)

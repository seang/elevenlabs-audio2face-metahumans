# ElevenLabs > Audio2Face > MetaHumans: Starter Project

Standard starter project with LiveLink plugin installed & accompanying audio2face usd file configured for use with the streaming audio player 

Copy `Audio2Face/eleven_labs_client.py` to `audio2face-2023.2.0\exts\omni.audio2face.player\omni\audio2face\player\scripts\streaming_server` or equivalent streaming_server directory with your audio2face installation

Example usage:

```
python eleven_labs_client.py elevenlabs /World/audio2face/PlayerStreaming "hello world"
```

Or start a basic flask server to accept POST requests with `python eleven_labs_client.py`


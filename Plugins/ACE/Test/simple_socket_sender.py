import argparse
import json
import os
import PyWave
import sched
import socket
import struct
import time
import threading

class RepeatedTimer(object):
    def __init__(self, interval, function, *args, **kwargs):
        self._timer = None
        self.interval = interval
        self.function = function
        self.args = args
        self.kwargs = kwargs
        self.is_running = False
        self.next_call = time.time()
        self.start()

    def _run(self):
        self.is_running = False
        self.start()
        self.function(*self.args, **self.kwargs)

    def start(self):
        if not self.is_running:
            self.next_call += self.interval
            self._timer = threading.Timer(self.next_call - time.time(), self._run)
            self._timer.start()
            self.is_running = True

    def stop(self):
        self._timer.cancel()
        self.is_running = False


class live_link_test_ue():
    def __init__(self, args) -> None:
        '''
        Initialize the sockets and open the wave and blendshape files
        '''
        self.all_blendshapes_sent = False
        self.all_audio_sent = False
        self.blendshapes_header_sent = False
        self.audio_header_sent = False
        self.sample_chunk_size = args.chunk_size
        self.frame_time = args.frame_time
        self.bs_fps = args.bs_fps
        self.stream_start_time = None
        self.audio_delay = args.audio_delay
        self.blendshape_delay = args.blendshape_delay
        self.wave_header = ""
        self.audio_socket = None
        self.blendshape_socket = None

        if args.no_audio:
            self.all_audio_sent = True
            self.audio_header_sent = True

        # Connect the socket to the port where the server is listening
        ipaddr = args.remote_address

        if not args.no_audio:
            self.audio_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = (ipaddr, args.audio_port)
            print(f"connecting to {server_address[0]} port {server_address[1]} for audio data")
            self.audio_socket.connect(server_address)

        server_address = (ipaddr, args.blendshape_port)
        print(f"connecting to {server_address[0]} port {server_address[1]} for blendshape data")
        self.blendshape_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.blendshape_socket.connect(server_address)

        audio_fpath = args.wave_file
        a2f_json_fpath = args.json_blendshape_file

        # open and format wave data
        self.wf = PyWave.open(audio_fpath)
        # Format:
        #- WAVE_FORMAT_PCM (1)
        #- WAVE_FORMAT_IEEE_FLOAT (3)
        #- WAVE_FORMAT_ALAW
        #- WAVE_FORMAT_MULAW
        #- WAVE_FORMAT_EXTENSIBLE
        print(f"rate: {self.wf.frequency}, channels: {self.wf.channels}, bits per sample: {self.wf.bits_per_sample}, format type: {self.wf.format}")

        # open the blendshape data file
        self.a2f_json_file = open(a2f_json_fpath)
        self.a2f_json_data = json.load(self.a2f_json_file)

        print(f"BlendShapes FPS: {self.bs_fps}")

    def send_with_validation(self, socket, raw_data, ascii_convert):
        verify_size = struct.pack("!Q", len(raw_data))
        send_data = verify_size
        if ascii_convert:
            send_data += bytes(raw_data, "ascii")
        else:
            send_data += raw_data
        if socket:
            socket.send(send_data)

    def send_eos(self):
        eos_symbol = f"EOS"
        self.send_with_validation(self.audio_socket, eos_symbol, True)
        self.send_with_validation(self.blendshape_socket, eos_symbol, True)
        print("E", end="", flush=True)

    def send_frame_data(self):
        '''
        Callled by a repeating timer to transmit the wave and blendshape data to Unreal, frame by frame
        '''
        # | indicates a longer time than the designated frame time, . means it was at or below
        current_time = time.time()
        last_frame_time = current_time - self.last_time
        if last_frame_time < self.frame_time * 0.9:
            print(".", end="", flush=True)
        elif last_frame_time > self.frame_time * 1.1:
            print("|", end="", flush=True)
        else:
            print("-", end="", flush=True)

        self.last_time = time.time()

        # send audio data --- account for the audio_start delay
        if current_time - self.stream_start_time > self.audio_delay:
            if not self.audio_header_sent:
                # send wave header
                # "SamplesPerSecond Channels BitsPerSample SampleType"
                print("a", end="", flush=True)
                self.audio_header_sent = True
                # Send the wave header and the first sample chunk immediately
                self.send_with_validation(self.audio_socket, self.wave_header, True)

            if not self.all_audio_sent:
                self.audio_sample_data = self.wf.read_samples(self.sample_chunk_size)
                if self.audio_sample_data:
                    self.send_with_validation(self.audio_socket, self.audio_sample_data, False)

            # Print an A when the audio data is completely sent
            if self.audio_data_size == self.wf.tell() and not self.all_audio_sent:
                print("A", end="", flush=True)
                self.all_audio_sent = True

        # send blendshape data
        current_time = time.time()
        if current_time - self.stream_start_time > self.blendshape_delay:
            if not self.blendshapes_header_sent:
                print("b", end="", flush=True)
                blendshape_header = f"A2F:{self.bs_fps}"
                self.send_with_validation(self.blendshape_socket, blendshape_header, True)
                self.blendshapes_header_sent = True

            if str(self.blendshape_frame_counter) in self.a2f_json_data.keys():        
                out_data = self.a2f_json_data[str(self.blendshape_frame_counter)]
                frame_data = json.dumps(out_data, separators=(",", ":"))
                self.send_with_validation(self.blendshape_socket, frame_data, True)
                self.blendshape_frame_counter += 1
            elif not self.all_blendshapes_sent:
                # Print a B when the blendshape data is completely sent
                print("B", end="", flush=True)
                self.all_blendshapes_sent = True


    def initiate_data_transfer(self):
        '''
        Initiates a repeating timer that will send the data to Unreal
        '''
        self.audio_data_size = (self.wf.bits_per_sample/8) * self.wf.samples
        self.last_time = time.time()
        self.blendshape_frame_counter = 0

        self.wave_header = f"WAVE:{self.wf.frequency}:{self.wf.channels}:{self.wf.bits_per_sample}:{self.wf.format}"
        print(f"Wave data stream header: {self.wave_header}")

        self.stream_start_time = self.last_time

        if self.frame_time < 0.001:
            while not (self.all_audio_sent and self.all_blendshapes_sent):
                self.send_frame_data()
            self.send_eos()
        else:
            rt = RepeatedTimer(self.frame_time, self.send_frame_data)  # it auto-starts, no need of rt.start()
            try:
                # Sleep a little longer to ensure all of the data is pushed
                sleep_time = (self.wf.samples / self.wf.frequency) * 1.1 + self.audio_delay + self.blendshape_delay
                print(f"Waiting for {sleep_time} seconds")

                # if we're streaming blendshapes there's no reason to send a blendshape header
                self.blendshapes_header_sent = True
                print("b", end="", flush=True)

                time.sleep(sleep_time)  # your long-running job goes here...
            finally:
                rt.stop()  # better in a try/finally block to make sure the program ends!     

    def close(self):
        '''
        Close all of the open files and sockets
        '''
        # close audio and blendshape files
        self.wf.close()
        self.a2f_json_file.close()

        if self.audio_socket:
            print('\nclosing audio socket')
            self.audio_socket.close()

        if self.blendshape_socket:
            print('\nclosing blendshape socket')
            self.blendshape_socket.close()


if __name__ == "__main__":
    default_audio_fpath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "audio_violet.wav")
    default_a2f_json_fpath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "bs_violet.json")

    parser = argparse.ArgumentParser(description="A2F LiveLink Unreal Test Stub",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("-r", "--remote-addr", dest="remote_address", action="store", default="localhost", required=False, help="Unreal network name/IP address")
    parser.add_argument("-a", "--audio-port", dest="audio_port", action="store", default=12031, type=int, required=False, help="Audio socket port")
    parser.add_argument("-b", "--blendshape-port", dest="blendshape_port", action="store", default=12030, type=int, required=False, help="Blendshape socket port")
    parser.add_argument("-w", "--wave-file", dest="wave_file", action="store", default=default_audio_fpath, required=False, help="Wave file path")
    parser.add_argument("-j", "--json-blendshape-file", dest="json_blendshape_file", action="store", default=default_a2f_json_fpath, required=False, help="Blendshape JSON file path")
    parser.add_argument("-c", "--chunk-size", dest="chunk_size", action="store", default=4000, type=int, required=False, help="Audio sample chunk size sent per frame")
    parser.add_argument("-f", "--frame_time", dest="frame_time", action="store", default=0.033, type=float, required=False, help="Time in seconds between sending blendshape frames - set to 0 to burst")
    parser.add_argument("-p", "--blendshape-fps", dest="bs_fps", action="store", default=30.0, type=float, required=False, help="FPS value in blendshape JSON data")
    parser.add_argument("-d", "--audio-delay", dest="audio_delay", action="store", default=0.0, type=float, required=False, help="Seconds delay to wait before sending audio")
    parser.add_argument("-e", "--blendshape-delay", dest="blendshape_delay", action="store", default=0.0, type=float, required=False, help="Seconds delay to wait before sending blendshapes")
    parser.add_argument("-n", "--no-audio", dest="no_audio", action="store_true", default=False, required=False, help="Pass this to send no audio data")

    args = parser.parse_args()

    live_link_inst = live_link_test_ue(args)
    live_link_inst.initiate_data_transfer()
    live_link_inst.close()

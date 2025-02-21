import numpy as np
import sounddevice as sd

from synth import pulseconfig, pulse, snare
from consts import master_clock, clocks_per_sample, samples_per_tick, bpm, samplerate

beats_per_tick = 4
ticks_per_beat = round(master_clock * 60 / (clocks_per_sample*samples_per_tick*beats_per_tick*bpm))
actualbpm = master_clock * 60 / (clocks_per_sample*samples_per_tick*beats_per_tick * ticks_per_beat)
print("samplerate =", samplerate)
print("ticks_per_beat =", ticks_per_beat, "actual bpm", actualbpm, "/", bpm)


bassconfig = pulseconfig()
bassconfig.pulse_width = 1
bassconfig.octave_transpose = -1
bassconfig.detune = 4
bassconfig.carrier_multiplier = 2
bassconfig.decay = 2
bassconfig.sustain = 1024
bassconfig.release = 3
bassconfig.vibrato_depth = 0
bassconfig.vibrato_rate = 0
bassconfig.vibrato_envelope = 0

#bassn = 'C.C.C.C-..C-..C-..C-..C.C.C.C-..'
#basso = '00000000000000000000000000000000'

bassn = '''
C.C.C.C.C.C.C.C.C.C.C.C.-.C.C.C.
G.G.G.G.G.G.G.G.G.G.G.G.-.G.G.G.
a.a.a.a.a.a.a.a.e.e.e.e.-.e.e.e.
C.C.C.C.C.C.C.C.-...............
'''
basso = '''
11111111111111111111111111111111
00000000000000000000000000000000
00000000000000001111111111111111
11111111111111111111111111111111
'''

snaren = '''
....x.......x.......x.......x...
....x.......x.......x.......x...
....x.......x.......x.......x...
'''


def strip_spaces(s):
    return ''.join(s.split())

class audiogen:
    def __init__(self):
        self.tick_samples = 0
        self.tickcount = 0
        self.songcount = 0
        self.instruments = []
        self.instruments.append(pulse(bassconfig))
        self.instruments.append(snare(2))
        self.tracks = []
        self.tracks.append((strip_spaces(bassn), strip_spaces(basso)))
        self.tracks.append((strip_spaces(snaren),))

    def get_next_audio_chunk(self, frame_count):
        print('get_next_audio_chunk', frame_count, self.tick_samples, self.tickcount, self.songcount)
        out = np.zeros(frame_count, np.float32)
        offset = 0
        while frame_count > 0:
            if self.tick_samples == 0:
                if self.tickcount == 0:
                    self.tickcount = 0
                    for track, instrument in zip(self.tracks, self.instruments):
                        i = self.songcount % len(track[0])
                        instrument.tickSong(i, *track)
                    self.songcount += 1
                for instrument in self.instruments:
                    instrument.tickEnvelopes()
                self.tickcount += 1
                if self.tickcount >= ticks_per_beat:
                    self.tickcount = 0

            runlength = frame_count
            if self.songcount + frame_count > samples_per_tick:
                runlength = samples_per_tick - self.tick_samples

            for instrument in self.instruments:
                instrument.render(out[offset:offset+runlength])

            frame_count -= runlength
            offset += runlength
            self.tick_samples += runlength
            if self.tick_samples >= samples_per_tick:
                self.tick_samples = 0
        return out
               
 
def main():
    # Set up the stream
    sample_rate = round(master_clock / clocks_per_sample)
    channels = 1
    blocksize = 2048  # Adjust this value based on your needs

    gen = audiogen()
    def audio_callback(outdata, frames, _, status):
        if status:
            print(status)
        outdata[:] = gen.get_next_audio_chunk(frames).reshape(-1, 1) * (1.0/32768.0)


    stream = sd.OutputStream(
        samplerate=sample_rate,
        channels=channels,
        callback=audio_callback,
        blocksize=blocksize
    )

    # Start the stream
    stream.start()

    print(f"Actual sample rate: {stream.samplerate} Hz")


    # Keep the stream running
    input("Press Enter to stop the audio stream...")

    # Stop the stream
    stream.stop()
    stream.close()


if __name__ == '__main__':
    main()

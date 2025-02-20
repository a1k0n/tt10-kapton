from consts import PHASEBITS, samplerate


def freqinc(freq):
    return round(freq * (1<<PHASEBITS) / samplerate)


def noteinc(noteidx):
    return freqinc(55 * 2**((noteidx+3)/12))


def notehex(noteidx):
    return hex(noteinc(noteidx))


# lowercase indicates flat
def letterinc(name, octave):
    idx = 'CdDeEFgGaAbB'.index(name)
    return noteinc(idx + 12*(ord(octave) - ord('0')))

    
class pulseconfig:
    def __init__(self):
        self.pulse_width = 0
        self.octave_transpose = 0
        self.detune = 0
        self.carrier_multiplier = 1
        self.decay = 2
        self.sustain = 1024
        self.release = 4
        self.vibrato_depth = 0
        self.vibrato_rate = 4
        self.vibrato_envelope = 0


class pulse:
    def __init__(self, config: pulseconfig):
        self.primary_phase = 0
        self.secondary_phase = 0
        self.volume = 0
        self.phase_inc = 0
        self.vibrato_sin = 0
        self.vibrato_cos = 1023
        self.vibrato_level = 0
        self.adsr_state = 0
        self.vibrato_level = 0
        self.config = config

    def tickSong(self, note: str, octave: str):
        if note != '.':
            if note == '-':
                # note off
                self.adsr_state = 4
            else:
                # note on
                self.adsr_state = 2
                self.volume = 4095
                self.phase_inc = letterinc(note, octave)
                self.vibrato_level = 0
                print('tickSong', note, octave, self.phase_inc)
                #self.vibrato_sin = 0
                #self.vibrato_cos = 1023
                #self.primary_phase = 0
                #self.secondary_phase = 0

    def tickEnvelopes(self):
        self.vibrato_cos -= self.vibrato_sin >> self.config.vibrato_rate
        self.vibrato_sin += self.vibrato_cos >> self.config.vibrato_rate
        if self.adsr_state == 0: # idle
            return
        elif self.adsr_state == 1: # attack
            # attack
            self.volume = 4095
            self.adsr_state = 2 # decay
        elif self.adsr_state == 2: # decay
            dv = self.volume - self.config.sustain
            rounding = (1<<self.config.decay) - 1
            self.volume -= (dv + rounding) >> self.config.decay
            if self.volume <= self.config.sustain:
                self.volume = self.config.sustain
                self.adsr_state = 3 # sustain
            if self.config.vibrato_envelope:
                dl = self.config.vibrato_depth - self.vibrato_level
                rounding = (1<<self.config.vibrato_envelope) - 1
                self.vibrato_level += (dl + rounding) >> self.config.vibrato_envelope
        elif self.adsr_state == 3: # sustain
            self.volume = self.config.sustain
        elif self.adsr_state == 4: # release
            dv = self.volume
            rounding = (1<<self.config.release) - 1
            self.volume -= (dv + rounding) >> self.config.release
            if self.volume <= 0:
                self.volume = 0
                self.adsr_state = 0 # idle

    def render(self, buffer):
        N = len(buffer)
        for i in range(N):
            sample = 0
            mask = 1<<(PHASEBITS-1)
            if self.config.pulse_width & 1:
                mask |= 1<<(PHASEBITS-2)
            if (self.primary_phase & mask) == mask:
                sample += self.volume
            else:
                sample -= self.volume
            if (self.secondary_phase & mask) == mask:
                sample += self.volume
            else:
                sample -= self.volume
            buffer[i] += sample

            vibrato_inc = self.vibrato_level * self.vibrato_cos >> 10
            self.primary_phase += self.phase_inc + vibrato_inc
            self.secondary_phase += (self.phase_inc + vibrato_inc) * self.config.carrier_multiplier + self.config.detune
            self.primary_phase &= (1<<PHASEBITS)-1
            self.secondary_phase &= (1<<PHASEBITS)-1



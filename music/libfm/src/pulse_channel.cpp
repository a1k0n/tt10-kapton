#include "libfm/pulse_channel.hpp"
#include "libfm/tables.hpp"
#include <cmath>

namespace fm {

namespace {
// at 30kHz this gives a frequency resolution of 1.8Hz
constexpr int PHASEBITS = 18;
}  // namespace

PulseState::PulseState() = default;

PulseConfig::PulseConfig() = default;

void PulseState::noteOn(int note, int velocity, const PulseConfig& config) {
  this->note = note;
  this->octave = note / 12;

  // TODO: optimize phase increments to use a 12-entry table and mux the octave
  // from the phase bits

  this->phase_inc = (1 << PHASEBITS) * 440 * pow(2, (note - 69 + (12 * config.octave_transpose)) / 12.0) / config.sample_rate;
  this->adsr_state = 2;
  this->volume = 4095;

  this->primary_phase = 0;
  this->secondary_phase = 0;
}

void PulseState::noteOff(const PulseConfig& config) {
  this->adsr_state = 4;
}


void PulseState::tickEnvelopes(const PulseConfig& config) {
  switch (adsr_state) {
    case 0:
      return;
    case 1:
      // attack is always instantaneous
      volume = 4095;
      adsr_state = 2;
      break;
    case 2:
    {
      int dv = volume - config.sustain;
      int rounding = (1<<config.decay) - 1;
      volume -= (dv + rounding) >> config.decay;
      if (volume <= config.sustain) {
        volume = config.sustain;
        adsr_state = 3;
      }
      break;
    }
    case 3:
      volume = config.sustain;
      break;
    case 4:
    {
      int dv = volume;
      int rounding = (1<<config.release) - 1;
      volume -= (dv + rounding) >> config.release;
      if (volume <= 0) {
        volume = 0;
        adsr_state = 0;
      }
      break;
    }
  }

  // TODO: vibrato
}


void PulseState::render(int16_t* buffer, int num_samples, const PulseConfig& config) {
  for (int i = 0; i < num_samples; i++) {
    int sample = 0;
    int mask = (1<<(PHASEBITS-1));
    if (config.pulse_width & 1) {
      mask |= (1<<(PHASEBITS-2));
    }
    if ((primary_phase & mask) == mask) {
      sample += volume;
    } else {
      sample -= volume;
    }
    if ((secondary_phase & mask) == mask) {
      sample += volume;
    } else {
      sample -= volume;
    }
    buffer[i] += sample;

    primary_phase += phase_inc;
    // synchronize scope buffer to primary phase
    if (scope_index >= 1024 && primary_phase & (1<<PHASEBITS)) {
      scope_index = 0;
    }
    secondary_phase += phase_inc * config.carrier_multiplier + config.detune;
    primary_phase &= (1<<PHASEBITS)-1;
    secondary_phase &= (1<<PHASEBITS)-1;
    if (scope_index < 1024) {
      scope_buffer[scope_index++] = sample / 4096.0f;
    }
  }
}

}  // namespace fm
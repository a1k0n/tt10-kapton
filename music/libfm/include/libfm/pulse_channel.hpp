#pragma once

#include <cstdint>

namespace fm {

class PulseConfig;

struct PulseState {
  PulseState();

  // only used for tracking midi notes
  int note{0};

  // actual hardware state
  int octave{0};
  int primary_phase{0};
  int secondary_phase{0};
  int volume{0};
  int phase_inc{0};
  int vibrato_sin{0}, vibrato_cos{1023};
  int adsr_state{0};  // 0=idle, 1=attack, 2=decay, 3=sustain, 4=release
  int vibrato_level{0};  // vibrato rises to config.vibrato_depth during sustain
  // secondary phase increment is phase_inc * carrier_multiplier + detune

  int lpf_y{0};
  int lpf_v{0};

  float scope_buffer[1024]{};
  int scope_index{0};

  int sample_count{0};

  void noteOn(int note, int velocity, const PulseConfig& config);
  void noteOff(const PulseConfig& config);
  void tickEnvelopes(const PulseConfig& config);
  void render(int16_t* buffer, int num_samples, const PulseConfig& config);
};

class PulseConfig {
 public:
  PulseConfig();

  float sample_rate{48000.0f};

  // Configuration
  int pulse_width{0};  // is actually number of MSBs (0=50%, 1=25%, 2=12.5%, ...)
  int octave_transpose{0};

  // secondary oscillator offsets
  int detune{0};
  int carrier_multiplier{1};

  int decay{2};  // decay speed (attack is instantaneous, decay is exponential toward sustain level)
  int sustain{1024};  // sustain level
  int release{4};  // release speed (exponential toward 0)
  int vibrato_depth{0};  // vibrato depth
  int vibrato_rate{4};  // vibrato speed shift (0=30Hz, 1=15Hz, 2=7.5Hz, 3=3.75Hz, 4=1.875Hz, 5=0.9375Hz)
  int vibrato_envelope{0};  // vibrato envelope speed

  bool lpf_enabled{false};
  int lpf_k1{0};
  int lpf_k2{0};
};

}  // namespace libfm
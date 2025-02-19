#pragma once
#include <cstdint>
#include "libfm/adsr.hpp"

namespace fm {

class FMConfig;

struct FMState {
  FMState();

  int note{0};
  int carrier_velocity{0};
  int carrier_phase{0};
  int modulator_phase{0};
  int carrier_phase_inc{0};
  int modulator_phase_inc{0};
  int modulator_sample{0};  // for feedback

  ADSRState carrier_adsr;
  ADSRState modulator_adsr;

  // For visualization
  float scope_buffer[1024]{};
  int scope_index{0};

  int sample_count{0};

  void noteOn(int note, int velocity, const FMConfig& config);
  void noteOff(const FMConfig& config);
  void render(int16_t* buffer, int num_samples, const FMConfig& config);
};

class FMConfig {
 public:
  FMConfig();

  // Configuration
  int modulation_index{1};
  int modulation_depth{6};
  int modulation_feedback{6};  // shift amount+1, or 0 for no feedback
  int octave_transpose{0};
  int carrier_decay{0};

  ADSR carrier_adsr;
  ADSR modulator_adsr;

  float sample_rate{48000.0f};
};

}  // namespace fm 
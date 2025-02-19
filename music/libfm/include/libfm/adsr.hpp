#pragma once

namespace fm {

enum ADSRStatus {
  IDLE,
  ATTACK,
  DECAY,
  SUSTAIN,
  RELEASE,
};

class ADSR {
 public:
  ADSR();

  // Configuration
  int attack_speed{0};
  int decay_speed{3};
  int release_speed{0};
  int sustain{5};
};

struct ADSRState {
  ADSRStatus state{IDLE};
  int value{511};  // max attenuation

  void trigger();
  void release();
  int step(int carry, const ADSR& config);
};

}  // namespace fm 
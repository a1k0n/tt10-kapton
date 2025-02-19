#include "libfm/adsr.hpp"

namespace fm {

ADSR::ADSR() = default;

void ADSRState::trigger() {
  state = ATTACK;
}

void ADSRState::release() {
  state = RELEASE;
}

int ADSRState::step(int carry, const ADSR &config) {
  switch (state) {
    case ATTACK:
      if (carry & (1 << config.attack_speed)) {
        value = value - (value >> 3) - 1;
        if (value <= 0) {
          value = 0;
          state = DECAY;
        }
      }
      break;
    case DECAY:
      if (carry & (1 << config.decay_speed)) {
        value++;
        if (value >= (1 << config.sustain)) {
          value = (1 << config.sustain);
          state = SUSTAIN;
        }
      }
      break;
    case RELEASE:
      if (carry & (1 << config.release_speed)) {
        value++;
        if (value >= 511) {
          value = 511;
          state = IDLE;
        }
      }
      break;
    case SUSTAIN:
      value = (1 << config.sustain);
      break;
    case IDLE:
      value = 511;
      break;
  }
  return value;
}

}  // namespace fm 
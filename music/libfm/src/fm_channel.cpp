#include "libfm/fm_channel.hpp"
#include "libfm/tables.hpp"
#include <cmath>

namespace fm {

namespace {
constexpr int PHASEBITS = 9;
constexpr int PARTIALPHASEBITS = 10;

int velocity_to_logatten(int velocity) {
  float v = velocity / 127.0f;
  float l = -log(v) / log(2);
  return static_cast<int>(l * 64);
}
}  // namespace

FMState::FMState() = default;

void FMState::noteOn(int note, int velocity, const FMConfig& config) {
  carrier_adsr.trigger();
  modulator_adsr.trigger();
  carrier_velocity = velocity_to_logatten(velocity);
  float carrier_freq = 440 * pow(2, (note - 69 + (12 * config.octave_transpose)) / 12.0);
  carrier_phase_inc = static_cast<int>((1 << (PHASEBITS + PARTIALPHASEBITS)) * carrier_freq / config.sample_rate);
  this->note = note;
}

void FMState::noteOff(const FMConfig& config) {
  carrier_adsr.release();
  modulator_adsr.release();
}

void FMState::render(int16_t* buffer, int n, const FMConfig& config) {
  for (int i = 0; i < n; i++) {
    int sample_carry = sample_count;
    sample_count++;
    sample_carry = sample_carry ^ sample_count;

    if (config.carrier_decay && (sample_carry&0x40)) {
      if (carrier_phase_inc > 0) {
        carrier_phase_inc -= (carrier_phase_inc >> config.carrier_decay);
      }
      if (modulator_phase_inc > 0) {
        modulator_phase_inc -= (modulator_phase_inc >> config.carrier_decay);
      }
    }
    carrier_adsr.step(sample_carry, config.carrier_adsr);
    modulator_adsr.step(sample_carry, config.modulator_adsr);

    int fb = 0;
    if (config.modulation_feedback > 0) {
      fb = modulator_sample << (config.modulation_feedback - 1);
    }
    int modsign = 1;
    int logmod = modulator_adsr.value + logsin9((modulator_phase + fb) >> PARTIALPHASEBITS, &modsign);
    modulator_sample = iexp11(logmod) * modsign;
    int cp = carrier_phase + (modulator_sample << config.modulation_depth);
    int carsign = 1;
    int logcar = carrier_adsr.value + carrier_velocity + logsin9(cp >> PARTIALPHASEBITS, &carsign);
    int carrier_sample = iexp11(logcar) * carsign;

    buffer[i] += carrier_sample << 3;

    int old_carrier_phase = carrier_phase;
    carrier_phase += carrier_phase_inc;
    if (((old_carrier_phase ^ carrier_phase) & (1<<(PHASEBITS + PARTIALPHASEBITS))) && scope_index >= 1024) {
      scope_index = 0;
    }
    modulator_phase += carrier_phase_inc * config.modulation_index;

    if (scope_index < 1024) {
      scope_buffer[scope_index] = carrier_sample * (1.0/2047.0);
      scope_index++;
    }
  }
}

FMConfig::FMConfig() = default;

}  // namespace fm 
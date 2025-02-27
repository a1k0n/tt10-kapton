#pragma once
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <atomic>
#include <array>
#include "libfm/pulse_channel.hpp"

class Audio {
 public:
  Audio();
  ~Audio();

  void noteOn(int note, int velocity);
  void noteOff(int note);
  void gui();

  void saveParameters(const char* filename);
  void loadParameters(const char* filename);

 private:
  static void* AudioThreadEntry(void* arg);
  void audioThread();
  int stealChannel();

  snd_pcm_t* pcm_handle_;
  pthread_t audio_thread_;
  std::atomic<bool> running_{true};

  fm::PulseConfig pulse_config_;
  fm::PulseState pulse_state_[4];

  int tempo_ticks_per_beat_{5};
  bool metronome_on_{false};
  int metronome_beat_count_{0};
  int metronome_tick_count_{0};
}; 

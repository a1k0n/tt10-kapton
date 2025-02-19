#pragma once
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <atomic>
#include "audio.hpp"

class Midi {
 public:
  explicit Midi(Audio& audio);
  ~Midi();
  bool open();

 private:
  static void* MidiThreadEntry(void* arg);
  void midiThread();

  Audio& audio_;
  snd_seq_t* seq_handle_{nullptr};
  int port_id_{-1};
  pthread_t midi_thread_;
  std::atomic<bool> running_{true};
}; 
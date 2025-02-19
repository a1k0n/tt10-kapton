#include "audio.hpp"
#include <imgui.h>
#include <cstring>
#include <stdexcept>

constexpr int SAMPLE_RATE = 48000;
constexpr int VSYNC_SAMPLES = 525;

void* Audio::AudioThreadEntry(void* arg) {
  Audio* audio = static_cast<Audio*>(arg);
  audio->audioThread();
  return NULL;
}

Audio::Audio() {
  int err = snd_pcm_open(&pcm_handle_, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0) {
    throw std::runtime_error("Failed to open PCM device");
  }

  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_alloca(&hw_params);
  snd_pcm_hw_params_any(pcm_handle_, hw_params);

  snd_pcm_hw_params_set_access(pcm_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm_handle_, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle_, hw_params, 1);

  unsigned int sample_rate = SAMPLE_RATE;
  snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params, &sample_rate, 0);

  snd_pcm_uframes_t buffer_size = 256;
  snd_pcm_uframes_t period_size = 64;

  snd_pcm_hw_params_set_buffer_size_near(pcm_handle_, hw_params, &buffer_size);
  snd_pcm_hw_params_set_period_size_near(pcm_handle_, hw_params, &period_size, 0);

  err = snd_pcm_hw_params(pcm_handle_, hw_params);
  if (err < 0) {
    throw std::runtime_error("Failed to set hardware parameters");
  }

  pulse_config_.sample_rate = SAMPLE_RATE;

  snd_pcm_nonblock(pcm_handle_, 0);
  pthread_create(&audio_thread_, NULL, AudioThreadEntry, this);
  loadParameters("params.txt");
}

Audio::~Audio() {
  running_ = false;
  pthread_join(audio_thread_, NULL);
  snd_pcm_close(pcm_handle_);
}

void Audio::noteOn(int note, int velocity) {
  int channel = stealChannel();
  pulse_state_[channel].noteOn(note, velocity, pulse_config_);
}

void Audio::noteOff(int note) {
  for (auto& channel : pulse_state_) {
    if (channel.note == note) {
      channel.noteOff(pulse_config_);
    }
  }
}

void Audio::gui() {
  int val;
  
  /*
  ImGui::SliderInt("Octave Transpose", &fm_config_.octave_transpose, -4, 4);
  ImGui::SliderInt("Modulation Index", &fm_config_.modulation_index, 0, 10);
  ImGui::SliderInt("Modulation Depth", &fm_config_.modulation_depth, 0, 10);
  ImGui::SliderInt("Modulation Feedback", &fm_config_.modulation_feedback, 0, 10);

  ImGui::SliderInt("Carrier Decay", &fm_config_.carrier_decay, 0, 11);

  // Carrier ADSR
  if (ImGui::TreeNode("Carrier ADSR")) {
    auto& adsr = fm_config_.carrier_adsr;
    ImGui::SliderInt("Attack", &adsr.attack_speed, 0, 11);
    ImGui::SliderInt("Decay", &adsr.decay_speed, 0, 11);
    ImGui::SliderInt("Release", &adsr.release_speed, 0, 11);
    ImGui::SliderInt("Sustain", &adsr.sustain, 0, 11);
    ImGui::TreePop();
  }

  // Modulator ADSR
  if (ImGui::TreeNode("Modulator ADSR")) {
    auto& adsr = fm_config_.modulator_adsr;
    ImGui::SliderInt("Attack", &adsr.attack_speed, 0, 11);
    ImGui::SliderInt("Decay", &adsr.decay_speed, 0, 11);
    ImGui::SliderInt("Release", &adsr.release_speed, 0, 11);
    ImGui::SliderInt("Sustain", &adsr.sustain, 0, 11);
    ImGui::TreePop();
  }
  */

  ImGui::SliderInt("Pulse Width", &pulse_config_.pulse_width, 0, 7);
  ImGui::SliderInt("Octave Transpose", &pulse_config_.octave_transpose, -4, 4);
  ImGui::SliderInt("Detune", &pulse_config_.detune, -100, 100);
  ImGui::SliderInt("Carrier Multiplier", &pulse_config_.carrier_multiplier, 1, 10);
  ImGui::SliderInt("Decay", &pulse_config_.decay, 0, 11);
  ImGui::SliderInt("Sustain", &pulse_config_.sustain, 0, 4095);
  ImGui::SliderInt("Release", &pulse_config_.release, 0, 11);
  ImGui::SliderInt("Vibrato Depth", &pulse_config_.vibrato_depth, 0, 10);
  ImGui::SliderInt("Vibrato Rate", &pulse_config_.vibrato_rate, 0, 10);

  // Visualizations
  for (auto& channel : pulse_state_) {
    ImGui::Text("Note: %d", channel.note);
    ImGui::PlotLines("", channel.scope_buffer, 1024, 0, NULL, -1.0f, 1.0f, ImVec2(1024, 100));
  }
}

void Audio::audioThread() {
  const int BUFFER_SIZE = 64;
  int16_t buffer[BUFFER_SIZE];
  int vsync_counter = 0;

  while (running_) {
    memset(buffer, 0, sizeof(buffer));
    int samples_to_render = BUFFER_SIZE;
    int offset = 0;
    while (samples_to_render > 0) {
      int runlength = samples_to_render;
      if (vsync_counter + BUFFER_SIZE > VSYNC_SAMPLES) {
        runlength = VSYNC_SAMPLES - vsync_counter;
      }
      for (int i = 0; i < 4; i++) {
        pulse_state_[i].render(buffer+offset, runlength, pulse_config_);
      }
      samples_to_render -= runlength;
      offset += runlength;
      vsync_counter += runlength;
      if (vsync_counter >= VSYNC_SAMPLES) {
        for (int i = 0; i < 4; i++) {
          pulse_state_[i].tickEnvelopes(pulse_config_);
        }
        vsync_counter = 0;
      }
    }


    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle_, buffer, BUFFER_SIZE);
    if (frames < 0) {
      frames = snd_pcm_recover(pcm_handle_, frames, 0);
    }
  }
}

int Audio::stealChannel() {
  int channel = -1;
  int min_volume = 10000;
  for (size_t i = 0; i < 4; i++) {
    auto s = pulse_state_[i].adsr_state;
    auto v = pulse_state_[i].volume;
    if (s == 0) {
      return i;
    } else if (s == 4 && v < min_volume) {
      channel = i;
      min_volume = v;
    }
  }
  return channel;
}

void Audio::saveParameters(const char* filename) {
  FILE* file = fopen(filename, "w");
  if (!file) return;

/*
  fprintf(file, "ModIndex=%d\n", fm_config_.modulation_index);
  fprintf(file, "ModDepth=%d\n", fm_config_.modulation_depth);
  fprintf(file, "ModFeedback=%d\n", fm_config_.modulation_feedback);
  fprintf(file, "OctaveTrans=%d\n", fm_config_.octave_transpose);
  
  fprintf(file, "CarrierAttack=%d\n", fm_config_.carrier_adsr.attack_speed);
  fprintf(file, "CarrierDecay=%d\n", fm_config_.carrier_adsr.decay_speed);
  fprintf(file, "CarrierSustain=%d\n", fm_config_.carrier_adsr.sustain);
  fprintf(file, "CarrierRelease=%d\n", fm_config_.carrier_adsr.release_speed);
  
  fprintf(file, "ModAttack=%d\n", fm_config_.modulator_adsr.attack_speed);
  fprintf(file, "ModDecay=%d\n", fm_config_.modulator_adsr.decay_speed);
  fprintf(file, "ModSustain=%d\n", fm_config_.modulator_adsr.sustain);
  fprintf(file, "ModRelease=%d\n", fm_config_.modulator_adsr.release_speed);

  fprintf(file, "CarrierDecay=%d\n", fm_config_.carrier_decay);
*/
  fprintf(file, "PulseWidth=%d\n", pulse_config_.pulse_width);
  fprintf(file, "OctaveTrans=%d\n", pulse_config_.octave_transpose);
  fprintf(file, "Detune=%d\n", pulse_config_.detune);
  fprintf(file, "CarrierMultiplier=%d\n", pulse_config_.carrier_multiplier);
  fprintf(file, "Decay=%d\n", pulse_config_.decay);
  fprintf(file, "Sustain=%d\n", pulse_config_.sustain);
  fprintf(file, "Release=%d\n", pulse_config_.release);
  fprintf(file, "VibratoDepth=%d\n", pulse_config_.vibrato_depth);
  fprintf(file, "VibratoRate=%d\n", pulse_config_.vibrato_rate);

  fclose(file);
}

static void check_parami(const char* line, const char* param, int* value) {
  if (strncmp(line, param, strlen(param)) == 0) {
    *value = atoi(line + strlen(param));
  }
}

void Audio::loadParameters(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (!file) return;

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    /*
    check_parami(line, "ModIndex=", &fm_config_.modulation_index);
    check_parami(line, "ModDepth=", &fm_config_.modulation_depth);
    check_parami(line, "ModFeedback=", &fm_config_.modulation_feedback);
    check_parami(line, "OctaveTrans=", &fm_config_.octave_transpose);
    check_parami(line, "CarrierAttack=", &fm_config_.carrier_adsr.attack_speed);
    check_parami(line, "CarrierDecay=", &fm_config_.carrier_adsr.decay_speed);
    check_parami(line, "CarrierSustain=", &fm_config_.carrier_adsr.sustain);
    check_parami(line, "CarrierRelease=", &fm_config_.carrier_adsr.release_speed);
    check_parami(line, "ModAttack=", &fm_config_.modulator_adsr.attack_speed);
    check_parami(line, "ModDecay=", &fm_config_.modulator_adsr.decay_speed);
    check_parami(line, "ModSustain=", &fm_config_.modulator_adsr.sustain);
    check_parami(line, "ModRelease=", &fm_config_.modulator_adsr.release_speed);
    check_parami(line, "CarrierDecay=", &fm_config_.carrier_decay);
    */
    check_parami(line, "PulseWidth=", &pulse_config_.pulse_width);
    check_parami(line, "OctaveTrans=", &pulse_config_.octave_transpose);
    check_parami(line, "Detune=", &pulse_config_.detune);
    check_parami(line, "CarrierMultiplier=", &pulse_config_.carrier_multiplier);
    check_parami(line, "Decay=", &pulse_config_.decay);
    check_parami(line, "Sustain=", &pulse_config_.sustain);
    check_parami(line, "Release=", &pulse_config_.release);
    check_parami(line, "VibratoDepth=", &pulse_config_.vibrato_depth);
    check_parami(line, "VibratoRate=", &pulse_config_.vibrato_rate);
  }
  fclose(file);
} 
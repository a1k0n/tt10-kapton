#include "midi.hpp"
#include <stdexcept>
#include <poll.h>

void* Midi::MidiThreadEntry(void* arg) {
  Midi* midi = static_cast<Midi*>(arg);
  midi->midiThread();
  return NULL;
}

Midi::Midi(Audio& audio) : audio_(audio) {}

bool Midi::open() {
  if (snd_seq_open(&seq_handle_, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
    return false;
  }

  snd_seq_set_client_name(seq_handle_, "FM Synth");
  port_id_ = snd_seq_create_simple_port(seq_handle_, "FM Synth Input",
      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
      SND_SEQ_PORT_TYPE_APPLICATION);

  // Connect to first available MIDI input
  snd_seq_client_info_t* cinfo;
  snd_seq_port_info_t* pinfo;
  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);
  snd_seq_client_info_set_client(cinfo, -1);

  while (snd_seq_query_next_client(seq_handle_, cinfo) >= 0) {
    int client = snd_seq_client_info_get_client(cinfo);
    if (client == 0) continue;  // Skip system client

    snd_seq_port_info_set_client(pinfo, client);
    snd_seq_port_info_set_port(pinfo, -1);

    while (snd_seq_query_next_port(seq_handle_, pinfo) >= 0) {
      int cap = snd_seq_port_info_get_capability(pinfo);
      if ((cap & SND_SEQ_PORT_CAP_READ) && (cap & SND_SEQ_PORT_CAP_SUBS_READ)) {
        snd_seq_connect_from(seq_handle_, port_id_,
            snd_seq_port_info_get_client(pinfo),
            snd_seq_port_info_get_port(pinfo));
        break;
      }
    }
  }

  pthread_create(&midi_thread_, NULL, MidiThreadEntry, this);
  return true;
}

Midi::~Midi() {
  running_ = false;
  pthread_join(midi_thread_, NULL);
  snd_seq_close(seq_handle_);
}

void Midi::midiThread() {
  struct pollfd* pfds;
  int npfds = snd_seq_poll_descriptors_count(seq_handle_, POLLIN);
  pfds = static_cast<struct pollfd*>(alloca(npfds * sizeof(struct pollfd)));
  snd_seq_poll_descriptors(seq_handle_, pfds, npfds, POLLIN);

  while (running_) {
    if (poll(pfds, npfds, 100) > 0) {  // 100ms timeout
      snd_seq_event_t* ev;
      while (snd_seq_event_input_pending(seq_handle_, 1) > 0) {
        snd_seq_event_input(seq_handle_, &ev);
        
        switch (ev->type) {
          case SND_SEQ_EVENT_NOTEON:
            audio_.noteOn(ev->data.note.note, ev->data.note.velocity);
            break;
          case SND_SEQ_EVENT_NOTEOFF:
            audio_.noteOff(ev->data.note.note);
            break;
          default:
            break;
        }
        
        snd_seq_free_event(ev);
      }
    }
  }
} 
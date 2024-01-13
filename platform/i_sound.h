#pragma once

#include <vector>

#include "s_midi.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//Number of possible sound channels
#define _MAX_VOICES 32
//For handles. Can I use a consteval function to ensure these are accurate at compile time. 
#define _VOICESHIFT 5

//Id of a bad handle
#define _NULL_HANDLE 0xFFFFFFFF
#define _ERR_NO_SLOTS 0xFFFFFFFE

//-----------------------------------------------------------------------------
// Init and shutdown
//-----------------------------------------------------------------------------

//Initalizes the audio library. Returns 0 on success, 1 on failure.
int plat_init_audio();

//Shutdown the audio library
void plat_close_audio();

//-----------------------------------------------------------------------------
// Handles and data
//-----------------------------------------------------------------------------

//Gets a handle to refer to this sound.
uint32_t plat_get_new_sound_handle();
//Like plat_get_new_sound_handle, but will return the handle of any prior source using soundNum, even if it is currently playing.
uint32_t plat_get_unique_sound_handle(int soundNum);

//Provide the PCM data for this sound effect
void plat_set_sound_data(uint32_t handle, unsigned char* data, int length, int sampleRate);

void plat_set_sound_position(uint32_t handle, int volume, int angle);

void plat_set_sound_angle(uint32_t handle, int angle);

void plat_set_sound_volume(uint32_t handle, int volume);

void plat_set_sound_loop_points(uint32_t handle, int start, int end);

//-----------------------------------------------------------------------------
// Emitting noise at player
//-----------------------------------------------------------------------------

void plat_start_sound(uint32_t handle, int loop);
void plat_stop_sound(uint32_t handle);

bool plat_check_if_sound_playing(uint32_t handle);

bool plat_check_if_sound_finished(uint32_t handle);

//-----------------------------------------------------------------------------
// Emitting pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

//Starts the audio system's MIDI system with the specified sequencer for playback.
int plat_start_midi(MidiSequencer *sequencer);
//Gets the synth's sample rate. This lets the Windows backend run the synth at its preferred sample rate
uint32_t plat_get_preferred_midi_sample_rate();
//Stops the audio system's MIDI system
void plat_close_midi();
//Sets the current music volume. In the range 0-127.
void plat_set_music_volume(int volume);

//Starts a given song
void plat_start_midi_song(HMPFile* song, bool loop);
void plat_stop_midi_song();

//[ISB] sigh. Hindsight's 20/20.
//[FUTURE ISB] no I really should have known better.
//[FUTURE FUTURE ISB] agh, no, we're restoring this interface, since it's needed for
//hardsynths.

//Returns true if there are available buffer slots in the music source's buffer queue.
bool midi_queue_slots_available();
//Clears all finished buffers from the queue.
void midi_dequeue_midi_buffers();
//Queues a new buffer into the music source, at MIDI_SAMPLERATE sample rate.
void midi_queue_buffer(int numSamples, uint16_t* data);

//Readies the source for playing MIDI music.
void midi_start_source();
//Stops the MIDI music source.
void midi_stop_source();
//Starts the MIDI source if it hasn't started already, and starts it again if it starved.
void midi_check_status();


//-----------------------------------------------------------------------------
// Emitting recordings of pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

void plat_start_hq_song(int sample_rate, std::vector<float>&& song_data, bool loop);
void plat_stop_hq_song();

//-----------------------------------------------------------------------------
// Emitting buffered movie sound at player
//-----------------------------------------------------------------------------

#define MVESND_S16LSB 1
#define MVESND_U8 2

void mvesnd_init_audio(int format, int samplerate, int stereo);
void mvesnd_queue_audio_buffer(int len, short* data);
void mvesnd_close();

void mvesnd_pause();
void mvesnd_resume();

#pragma once

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//Number of possible sound channels
#define _MAX_VOICES 32

//Id of a bad handle
#define _ERR_NO_SLOTS 0xFFFE

//-----------------------------------------------------------------------------
// Init and shutdown
//-----------------------------------------------------------------------------

//Initalizes the audio library. Returns 0 on success, 1 on failure.
int I_InitAudio();

//Shutdown the audio library
void I_ShutdownAudio();

//-----------------------------------------------------------------------------
// Handles and data
//-----------------------------------------------------------------------------

//Gets a handle to refer to this sown
int I_GetSoundHandle();

//Provide the PCM data for this sound effect
void I_SetSoundData(int handle, unsigned char* data, int length, int sampleRate);

void I_SetSoundInformation(int handle, int volume, int angle);

void I_SetAngle(int handle, int angle);

void I_SetVolume(int handle, int volume);

//-----------------------------------------------------------------------------
// Emitting noise at player
//-----------------------------------------------------------------------------

void I_PlaySound(int handle, int loop);
void I_StopSound(int handle);

int I_CheckSoundPlaying(int handle);

int I_CheckSoundDone(int handle);

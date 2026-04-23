Load a sound from a decoded raw LPCM memory block and return a reference to it.

The input pointer is borrowed only for the duration of the call. The audio data
is copied to the audio backend before the function returns.

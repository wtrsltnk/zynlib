/*
  ZynAddSubFX - a software synthesizer

  PAaudiooutput.h - Audio output for PortAudio
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/
#ifndef SDL2_ENGINE_H
#define SDL2_ENGINE_H

#include <SDL2/SDL.h>

#include "zyn.common/globals.h"
#include "AudioOut.h"

class SdlEngine:public AudioOut
{
    public:
        SdlEngine();
        ~SdlEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

    protected:
        static void my_audio_callback(void *userdata, Uint8 *stream, int len);
        int process(Uint8 *stream, int len);

    private:
        SDL_AudioDeviceID _dev;
};

#endif
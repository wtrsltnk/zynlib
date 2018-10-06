/*
  ZynAddSubFX - a software synthesizer

  Master.h - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
  Copyright (C) 2002-2005 Nasca Octavian Paul
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

#ifndef MIXER_H
#define MIXER_H

#include "Microtonal.h"
#include <zyn.common/IPresetsSerializer.h>
#include <zyn.common/globals.h>
#include <zyn.synth/Controller.h>
#include <zyn.synth/ifftwrapper.h>

#include <pthread.h>

typedef enum {
    MUTEX_TRYLOCK,
    MUTEX_LOCK,
    MUTEX_UNLOCK
} lockset;

struct vuData
{
    vuData(void);
    float outpeakl, outpeakr, maxoutpeakl, maxoutpeakr,
        rmspeakl, rmspeakr;
    int clipped;
};

/** It sends Midi Messages to Instruments, receives samples from instruments,
 *  process them with system/insertion effects and mix them */
class Mixer : public IMixer
{
public:
    /** Constructor TODO make private*/
    Mixer(SystemSettings *synth_, IBankManager *bank_);
    /** Destructor*/
    virtual ~Mixer();

    virtual IBankManager* GetBankManager();

    /**Saves all settings to a XML file
         * @return 0 for ok or <0 if there is an error*/
    int saveXML(const char *filename);

    void defaults();

    /**loads all settings from a XML file
         * @return 0 for ok or -1 if there is an error*/
    int loadXML(const char *filename);
    void applyparameters(bool lockmutex = true);

    /**get all data to a newly allocated array (used for VST)
         * @return the datasize*/
    int getalldata(char **data);
    /**put all data from the *data array to zynaddsubfx parameters (used for VST)*/
    void putalldata(char *data, int size);

    // Mutex
    virtual void Lock();
    virtual void Unlock();

    //Midi IN
    virtual void NoteOn(unsigned char chan, unsigned char note, unsigned char velocity);
    virtual void NoteOff(unsigned char chan, unsigned char note);
    virtual void PolyphonicAftertouch(unsigned char chan, unsigned char note, unsigned char velocity);
    virtual void SetController(unsigned char chan, int type, int par);
    virtual void SetProgram(unsigned char chan, unsigned int pgm);

    void ShutUp();
    int shutup;

    void vuUpdate(const float *outl, const float *outr);

    /**Audio Output*/
    virtual void AudioOut(float *outl, float *outr);
    /**Audio Output (for callback mode). This allows the program to be controled by an external program*/
    virtual void GetAudioOutSamples(size_t nsamples,
                                    unsigned samplerate,
                                    float *outl,
                                    float *outr);

    void partonoff(int npart, int what);

    virtual int GetInstrumentCount();
    virtual class Instrument *GetInstrument(int index);

    /**parts \todo see if this can be made to be dynamic*/
    class Instrument *part[NUM_MIDI_PARTS]{};

    //parameters

    unsigned char Pvolume{};
    unsigned char Pkeyshift{};
    unsigned char Psysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS]{};
    unsigned char Psysefxsend[NUM_SYS_EFX][NUM_SYS_EFX]{};

    //parameters control
    void setPvolume(unsigned char Pvolume_);
    void setPkeyshift(unsigned char Pkeyshift_);
    void setPsysefxvol(int Ppart, int Pefx, unsigned char Pvol);
    void setPsysefxsend(int Pefxfrom, int Pefxto, unsigned char Pvol);

    //effects
    class EffectManager *sysefx[NUM_SYS_EFX]{}; //system
    class EffectManager *insefx[NUM_INS_EFX]{}; //insertion
                                              //      void swapcopyeffects(int what,int type,int neff1,int neff2);

    //part that's apply the insertion effect; -1 to disable
    short int Pinsparts[NUM_INS_EFX]{};

    //peaks for VU-meter
    void vuresetpeaks();
    //get VU-meter data
    vuData getVuData();

    //peaks for part VU-meters
    /**\todo synchronize this with a mutex*/
    float vuoutpeakpart[NUM_MIDI_PARTS]{};
    unsigned char fakepeakpart[NUM_MIDI_PARTS]{}; //this is used to compute the "peak" when the part is disabled

    Controller ctl;
    bool swaplr; //if L and R are swapped

    //other objects
    Microtonal microtonal;
    class IBankManager *bank;

    IFFTwrapper *fft;
    pthread_mutex_t mutex{};
    pthread_mutex_t vumutex{};

private:
    /**This adds the parameters to the XML data*/
    void add2XML(IPresetsSerializer *xml);
    /**This loads the parameters from the XML data*/
    void getfromXML(IPresetsSerializer *xml);

private:
    vuData vu;
    float volume{};
    float sysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS]{};
    float sysefxsend[NUM_SYS_EFX][NUM_SYS_EFX]{};
    int keyshift{};

    //information relevent to generating plugin audio samples
    float *bufl;
    float *bufr;
    off_t off;
    size_t smps;
};

#endif // MIXER_H

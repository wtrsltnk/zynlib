#include "par.h"
#include <fstream>
#include <iostream>
#include <memory.h>
#include <zyn.mixer/Mixer.h>
#include <zyn.serialization/SaveToFileSerializer.h>

extern "C" Mixer *CreateMixer();

extern "C" void DestroyMixer(
    Mixer *mixer);

extern "C" void AudioOut(
    Mixer *mixer,
    float outl[],
    float outr[]);

extern "C" void LoadPresets(
    Mixer *mixer,
    unsigned char chan,
    const char *xmldata,
    int size);

extern "C" void NoteOn(
    Mixer *mixer,
    unsigned char chan,
    unsigned char note,
    unsigned char velocity);

extern "C" void NoteOff(
    Mixer *mixer,
    unsigned char chan,
    unsigned char note);

extern "C" void SetPar(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    unsigned char par);

extern "C" void SetParBool(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    bool value);

extern "C" void SetParReal(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    float value);

static std::ofstream logfile;

Mixer *CreateMixer()
{
    Config::Current().init();

    if (!logfile.is_open())
    {
        logfile.open("c:\\temp\\log.txt", std::ios::out);
    }

    auto m = new Mixer();
    m->Init();

    logfile << "CreateMixer" << std::endl;
    logfile << "SampleRate:        " << m->SampleRate() << std::endl;
    logfile << "BufferSize:        " << m->BufferSize() << std::endl;
    logfile << "BufferSizeInBytes: " << m->BufferSizeInBytes() << std::endl;

    return m;
}

void AudioOut(
    Mixer *mixer,
    float outl[],
    float outr[])
{
    if (mixer != nullptr)
    {
        mixer->Lock();
        mixer->AudioOut(outl, outr);
        mixer->Unlock();
    }
}

extern "C" void LoadPresets(
    Mixer *mixer,
    unsigned char chan,
    const char *xmldata,
    int size)
{
    (void)size;

    logfile << "LoadPresets: " << std::endl;
    if (mixer == nullptr)
    {
        return;
    }

    auto track = mixer->GetTrack(chan);

    if (track == nullptr)
    {
        return;
    }

    if (track->Penabled == false)
    {
        mixer->EnableTrack(chan, true);
    }

    track->Lock();

    track->AllNotesOff();

    track->InstrumentDefaults();

    auto result = SaveToFileSerializer().LoadTrackFromData(track, xmldata);

    track->ApplyParameters();

    track->Unlock();

    logfile << "loading done with result == " << result << std::endl;
}

void NoteOn(
    Mixer *mixer,
    unsigned char chan,
    unsigned char note,
    unsigned char velocity)
{
    logfile << "NoteOn  : " << (int)note << std::endl;
    if (mixer == nullptr)
    {
        return;
    }

    mixer->NoteOn(chan, note, velocity);
}

void NoteOff(
    Mixer *mixer,
    unsigned char chan,
    unsigned char note)
{
    logfile << "NoteOff : " << (int)note << std::endl;
    if (mixer == nullptr)
    {
        return;
    }

    mixer->NoteOff(chan, note);
}

void SetPar(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    unsigned char par)
{
    auto track = mixer->GetTrack(chan);

    if (track == nullptr)
    {
        return;
    }
}

void SetParBool(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    bool value)
{
    auto track = mixer->GetTrack(chan);

    if (track == nullptr)
    {
        return;
    }
}

void SetParReal(
    Mixer *mixer,
    unsigned char chan,
    const char *id,
    float value)
{
    auto track = mixer->GetTrack(chan);

    if (track == nullptr)
    {
        return;
    }
}

void DestroyMixer(
    Mixer *mixer)
{
    logfile << "DestroyMixer" << std::endl;
    if (logfile.is_open())
    {
        logfile.close();
    }

    if (mixer != nullptr)
    {
        delete mixer;
    }
}

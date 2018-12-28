#ifndef EFFECTPRESETS_H
#define EFFECTPRESETS_H

enum ReverbPresets
{
    ReverbVolume = 0,
    ReverbPanning = 1,
    ReverbTime = 2,
    ReverbInitialDelay = 3,
    ReverbInitialDelayFeedback = 4,
    ReverbUnused1 = 5,
    ReverbUnused2 = 6,
    ReverbLowPassFilter = 7,
    ReverbHighPassFilter = 8,
    ReverbDampening = 9,
    ReverbType = 10,
    ReverbRoomSize = 11,
    ReverbBandwidth = 12,
};

enum EchoPresets
{
    EchoVolume = 0,
    EchoPanning = 1,
    EchoDelay = 2,
    EchoDelayBetweenLR = 3,
    EchoChannelRouting = 4,
    EchoFeedback = 5,
    EchoDampening = 6,
};

enum ChorusPresets
{
    ChorusVolume = 0,
    ChorusPanning = 1,
    ChorusLFOFrequency = 2,
    ChorusLFORandomness = 3,
    ChorusLFOFunction = 4,
    ChorusLFOStereo = 5,
    ChorusDepth = 6,
    ChorusDelay = 7,
    ChorusFeedback = 8,
    ChorusChannelRouting = 9,
    ChorusUnused1 = 10,
    ChorusSubtract = 11,
};

enum PhaserPresets
{
    PhaserVolume = 0,
    PhaserPanning = 1,
    PhaserLFOFrequency = 2,
    PhaserLFORandomness = 3,
    PhaserLFOFunction = 4,
    PhaserLFOStereo = 5,
    PhaserDepth = 6,
    PhaserFeedback = 7,
    PhaserStages = 8,
    PhaserChannelRouting = 9,
    PhaserSubtract = 10,
    PhaserPhase = 11,
    PhaserHyper = 12,
    PhaserDistortion = 13,
    PhaserAnalog = 14,
};

enum AlienWahPresets
{
    AlienWahVolume = 0,
    AlienWahPanning = 1,
    AlienWahLFOFrequency = 2,
    AlienWahLFORandomness = 3,
    AlienWahLFOFunction = 4,
    AlienWahLFOStereo = 5,
    AlienWahDepth = 6,
    AlienWahFeedback = 7,
    AlienWahDelay = 8,
    AlienWahChannelRouting = 9,
    AlienWahPhase = 10,
};

enum DistorsionPresets
{
    DistorsionVolume = 0,
    DistorsionPanning = 1,
    DistorsionChannelRouting = 2,
    DistorsionDrive = 3,
    DistorsionLevel = 4,
    DistorsionType = 5,
    DistorsionNegate = 6,
    DistorsionLowPassFilter = 7,
    DistorsionHighPassFilter = 8,
    DistorsionStereo = 9,
    DistorsionPreFiltering = 10,
};

enum EQPresets
{
    EQVolume = 0,
    EQBandType = 10,
    EQBandFrequency = 11,
    EQBandGain = 12,
    EQBandQ = 13,
    EQBandStages = 14,
};

enum DynFilterPresets
{
    DynFilterVolume = 0,
    DynFilterPanning = 1,
    DynFilterLFOFrequency = 2,
    DynFilterLFORandomness = 3,
    DynFilterLFOFunction = 4,
    DynFilterLFOStereo = 5,
    DynFilterDepth = 6,
    DynFilterAmplitudeSense = 7,
    DynFilterAmplitudeSenseInvert = 8,
    DynFilterAmplitudeSmooth = 9,
};

#endif // EFFECTPRESETS_H

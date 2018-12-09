#include "../app.threedee.h"

#include "../imgui_addons/imgui_knob.h"
#include <zyn.synth/ADnoteParams.h>

void AppThreeDee::ADNoteVoiceEditor(ADnoteVoiceParam *parameters)
{
    ImGui::Text("ADsynth Voice Parameters of the Instrument");

    auto enabled = parameters->Enabled != 0;
    if (ImGui::Checkbox("##enabled", &enabled))
    {
        parameters->Enabled = enabled ? 1 : 0;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Enable this voice");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::Text("Enabled");

    if (ImGui::BeginTabBar("ADNote Voice"))
    {
        if (ImGui::BeginTabItem("Oscillator"))
        {
            ADNoteVoiceEditorOscillator(parameters);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Amplitude"))
        {
            ADNoteVoiceEditorAmplitude(parameters);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Filter"))
        {
            ADNoteVoiceEditorFilter(parameters);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Frequency"))
        {
            ADNoteVoiceEditorFrequency(parameters);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Modulation"))
        {
            ADNoteVoiceEditorModulation(parameters);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void AppThreeDee::ADNoteVoiceEditorOscillator(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Oscillator Parameters");

    auto phase = static_cast<float>(64 - parameters->Poscilphase);
    if (ImGui::SliderFloat("Phase", &phase, 0, 128))
    {
        parameters->PVolume = static_cast<unsigned char>(phase);
    }

    ImGui::Separator();

    ADNoteVoiceEditorOscillatorUnison(parameters);
}

void AppThreeDee::ADNoteVoiceEditorOscillatorUnison(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Oscillator Unison Parameters");

    static char const *unison_sizes[] = {
        "OFF",
        "Size 2",
        "Size 3",
        "Size 4",
        "Size 5",
        "Size 6",
        "Size 8",
        "Size 10",
        "Size 12",
        "Size 15",
        "Size 20",
        "Size 25",
        "Size 30",
        "Size 40",
        "Size 50",
    };
    static char const *current_unison_size_item = nullptr;

    auto unison_size = static_cast<int>(parameters->PFMEnabled);
    current_unison_size_item = unison_sizes[unison_size];
    if (ImGui::BeginCombo("Unison", current_unison_size_item))
    {
        for (int n = 0; n < 15; n++)
        {
            bool is_selected = (current_unison_size_item == unison_sizes[n]);
            if (ImGui::Selectable(unison_sizes[n], is_selected))
            {
                current_unison_size_item = unison_sizes[n];
                parameters->PFMEnabled = static_cast<unsigned char>(n);
            }
        }

        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Unison size");
        ImGui::EndTooltip();
    }

    auto frequency_spread = static_cast<float>(parameters->Unison_frequency_spread);
    if (ImGui::SliderFloat("Frequency Spread", &frequency_spread, 0, 128))
    {
        parameters->Unison_frequency_spread = static_cast<unsigned char>(frequency_spread);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Frequency Spread of the Unison (cents)");
        ImGui::EndTooltip();
    }

    auto phase_randomness = static_cast<float>(parameters->Unison_phase_randomness);
    if (ImGui::Knob("Ph.rand", &phase_randomness, 0, 128, ImVec2(45, 30)))
    {
        parameters->Unison_phase_randomness = static_cast<unsigned char>(phase_randomness);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Phase randomness");
        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    auto stereo_spread = static_cast<float>(parameters->Unison_stereo_spread);
    if (ImGui::Knob("Stereo", &stereo_spread, 0, 128, ImVec2(45, 30)))
    {
        parameters->Unison_stereo_spread = static_cast<unsigned char>(stereo_spread);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Stereo Spread");
        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    auto vibratto = static_cast<float>(parameters->Unison_vibratto);
    if (ImGui::Knob("Vibrato", &vibratto, 0, 128, ImVec2(45, 30)))
    {
        parameters->Unison_vibratto = static_cast<unsigned char>(vibratto);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Vibrato");
        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    auto vibratto_speed = static_cast<float>(parameters->Unison_vibratto_speed);
    if (ImGui::Knob("V.speed", &vibratto_speed, 0, 128, ImVec2(45, 30)))
    {
        parameters->Unison_vibratto_speed = static_cast<unsigned char>(vibratto_speed);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Vibrato Average Speed");
        ImGui::EndTooltip();
    }
}

void AppThreeDee::ADNoteVoiceEditorAmplitude(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Amplitude Parameters");

    auto vol = static_cast<float>(parameters->PVolume);
    if (ImGui::SliderFloat("Vol", &vol, 0, 128))
    {
        parameters->PVolume = static_cast<unsigned char>(vol);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Volume");
        ImGui::EndTooltip();
    }

    auto velocityScale = static_cast<float>(parameters->PAmpVelocityScaleFunction);
    if (ImGui::SliderFloat("V.Sns", &velocityScale, 0, 128))
    {
        parameters->PAmpVelocityScaleFunction = static_cast<unsigned char>(velocityScale);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Velocity Sensing Function (rightmost to disable)");
        ImGui::EndTooltip();
    }

    auto pan = static_cast<float>(parameters->PPanning);
    if (ImGui::Knob("Pan", &pan, 0, 128, ImVec2(40, 40)))
    {
        parameters->PPanning = static_cast<unsigned char>(pan);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Panning (leftmost is random)");
        ImGui::EndTooltip();
    }

    ImGui::Separator();

    Envelope("Amplitude Envelope", parameters->AmpEnvelope);

    ImGui::Separator();

    LFO("Amplitude LFO", parameters->AmpLfo);
}

void AppThreeDee::ADNoteVoiceEditorFilter(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Filter Parameters");

    FilterParameters(parameters->VoiceFilter);

    ImGui::Separator();

    Envelope("Filter Envelope", parameters->FilterEnvelope);

    ImGui::Separator();

    LFO("Filter LFo", parameters->FilterLfo);
}

void AppThreeDee::ADNoteVoiceEditorFrequency(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Frequency Parameters");

    auto detune = static_cast<float>(parameters->PDetune) - 8192;
    if (ImGui::SliderFloat("Detune", &detune, -35, 35))
    {
        parameters->PDetune = static_cast<unsigned short int>(detune + 8192);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Fine detune (cents)");
        ImGui::EndTooltip();
    }

    auto octave = static_cast<int>(parameters->PCoarseDetune / 1024);
    if (octave >= 8)
    {
        octave -= 16;
    }
    if (ImGui::InputInt("Octave", &octave))
    {
        if (octave < -8)
        {
            octave = -8;
        }
        else if (octave > 7)
        {
            octave = 7;
        }

        if (octave < 0)
        {
            octave += 16;
        }
        parameters->PCoarseDetune = static_cast<unsigned short>(octave * 1024 + parameters->PCoarseDetune % 1024);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Octave");
        ImGui::EndTooltip();
    }

    static char const *detune_types[] = {
        "L35cents",
        "L10cents",
        "E100cents",
        "E1200cents",
    };
    static char const *current_detune_types_item = nullptr;

    auto detune_type = static_cast<int>(parameters->PDetuneType - 1);
    current_detune_types_item = detune_types[detune_type];
    if (ImGui::BeginCombo("Detune type", current_detune_types_item))
    {
        for (int n = 0; n < 4; n++)
        {
            bool is_selected = (current_detune_types_item == detune_types[n]);
            if (ImGui::Selectable(detune_types[n], is_selected))
            {
                current_detune_types_item = detune_types[n];
                parameters->PDetuneType = static_cast<unsigned char>(n) + 1;
            }
        }

        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Detune type");
        ImGui::EndTooltip();
    }

    ImGui::Separator();

    Envelope("Frequency Envelope", parameters->FreqEnvelope);

    ImGui::Separator();

    LFO("Frequency LFo", parameters->FreqLfo);
}

void AppThreeDee::ADNoteVoiceEditorModulation(ADnoteVoiceParam *parameters)
{
    ImGui::Text("Voice Modulation Parameters");

    static char const *modulation_types[] = {
        "OFF",
        "MORPH",
        "RING",
        "PM",
        "FM",
    };
    static char const *current_modulation_type_item = nullptr;

    auto modulation_type = static_cast<int>(parameters->PFMEnabled);
    current_modulation_type_item = modulation_types[modulation_type];
    if (ImGui::BeginCombo("Modulation type", current_modulation_type_item))
    {
        for (int n = 0; n < 5; n++)
        {
            bool is_selected = (current_modulation_type_item == modulation_types[n]);
            if (ImGui::Selectable(modulation_types[n], is_selected))
            {
                current_modulation_type_item = modulation_types[n];
                parameters->PFMEnabled = static_cast<unsigned char>(n);
            }
        }

        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Modulation type");
        ImGui::EndTooltip();
    }

    if (parameters->PFMEnabled > 0)
    {
        auto vol = static_cast<float>(parameters->PVolume);
        if (ImGui::SliderFloat("Vol", &vol, 0, 128))
        {
            parameters->PVolume = static_cast<unsigned char>(vol);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Volume");
            ImGui::EndTooltip();
        }

        auto velocityScale = static_cast<float>(parameters->PAmpVelocityScaleFunction);
        if (ImGui::SliderFloat("V.Sns", &velocityScale, 0, 128))
        {
            parameters->PAmpVelocityScaleFunction = static_cast<unsigned char>(velocityScale);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Velocity Sensing Function (rightmost to disable)");
            ImGui::EndTooltip();
        }

        auto FMVolumeDamp = static_cast<float>(parameters->PFMVolumeDamp - 64);
        if (ImGui::SliderFloat("F.Damp", &FMVolumeDamp, -64, 63))
        {
            parameters->PFMVolumeDamp = static_cast<unsigned char>(FMVolumeDamp) + 64;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Modulator Damp at Higher frequency");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        Envelope("Modulation Amplitude Envelope", parameters->FMAmpEnvelope);

        ImGui::Separator();

        auto detune = static_cast<float>(parameters->PDetune) - 8192;
        if (ImGui::SliderFloat("Detune", &detune, -35, 35))
        {
            parameters->PDetune = static_cast<unsigned short int>(detune + 8192);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Fine detune (cents)");
            ImGui::EndTooltip();
        }

        auto octave = static_cast<int>(parameters->PCoarseDetune / 1024);
        if (octave >= 8)
        {
            octave -= 16;
        }
        if (ImGui::InputInt("Octave", &octave))
        {
            if (octave < -8)
            {
                octave = -8;
            }
            else if (octave > 7)
            {
                octave = 7;
            }

            if (octave < 0)
            {
                octave += 16;
            }
            parameters->PCoarseDetune = static_cast<unsigned short>(octave * 1024 + parameters->PCoarseDetune % 1024);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Octave");
            ImGui::EndTooltip();
        }

        static char const *detune_types[] = {
            "L35cents",
            "L10cents",
            "E100cents",
            "E1200cents",
        };
        static char const *current_detune_types_item = nullptr;

        auto detune_type = static_cast<int>(parameters->PDetuneType);
        current_detune_types_item = detune_types[detune_type];
        if (ImGui::BeginCombo("Detune type", current_detune_types_item))
        {
            for (int n = 0; n < 4; n++)
            {
                bool is_selected = (current_detune_types_item == detune_types[n]);
                if (ImGui::Selectable(detune_types[n], is_selected))
                {
                    current_detune_types_item = detune_types[n];
                    parameters->PDetuneType = static_cast<unsigned char>(n);
                }
            }

            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Detune type");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        Envelope("Modulation Frequency Envelope", parameters->FMFreqEnvelope);
    }
}

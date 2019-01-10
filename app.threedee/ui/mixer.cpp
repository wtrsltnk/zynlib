#include "../app.threedee.h"

#include "../imgui_addons/imgui_checkbutton.h"
#include "../imgui_addons/imgui_knob.h"
#include "../stb_image.h"

#define MIN_DB (-48)

static ImVec2 trackSize = ImVec2(150, 0);
static float sliderBaseHeight = 150.0f;
static float const largeModeTreshold = 4.5f;
static int mostInsertEffectsPerChannel = 0;

static char const *const channels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
};

static char const *instrumentCategoryIconFileNames[] = {
    "default.png",              // 0
    "Piano.png",                // 1
    "Chromatic_Percussion.png", // 2
    "Organ.png",                // 3
    "Guitar.png",               // 4
    "Bass.png",                 // 5
    "Solo_Strings.png",         // 6
    "Ensemble.png",             // 7
    "Brass.png",                // 8
    "Reed.png",                 // 9
    "Pipe.png",                 // 10
    "Synth_Lead.png",           // 11
    "Synth_Pad.png",            // 12
    "Synth_Effects.png",        // 13
    "Ethnic.png",               // 14
    "Percussive.png",           // 15
    "Sound_Effects.png",        // 16
};

static char const *instrumentCategoryNames[] = {
    "Default",              // 0
    "Piano",                // 1
    "Chromatic Percussion", // 2
    "Organ",                // 3
    "Guitar",               // 4
    "Bass",                 // 5
    "Solo Strings",         // 6
    "Ensemble",             // 7
    "Brass",                // 8
    "Reed",                 // 9
    "Pipe",                 // 10
    "Synth Lead",           // 11
    "Synth Pad",            // 12
    "Synth Effects",        // 13
    "Ethnic",               // 14
    "Percussive",           // 15
    "Sound Effects",        // 16
};

void AppThreeDee::LoadInstrumentIcons()
{
    std::string rootDir = "./icons/Instruments/";

    _iconImagesAreLoaded = false;

    for (int i = 0; i < int(InstrumentCategories::COUNT); i++)
    {
        GLuint my_opengl_texture;
        glGenTextures(1, &my_opengl_texture);

        auto filename = rootDir + instrumentCategoryIconFileNames[i];
        int x, y, n;
        unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, 0);
        if (data == nullptr)
        {
            std::cout << "Failed to load instrument category " << i << " from file " << filename << std::endl;
            _iconImages[i] = 0;
            continue;
        }
        _iconImages[i] = my_opengl_texture;
        _iconImagesAreLoaded = true;

        glBindTexture(GL_TEXTURE_2D, my_opengl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, n, x, y, 0, n == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
}

std::vector<char const *> toCharVector(std::set<std::string> const &strings)
{
    std::vector<char const *> result;
    for (auto &s : strings)
    {
        result.push_back(s.c_str());
    }

    return result;
}

void AppThreeDee::ImGuiMixer()
{
    if (!_showMixer)
    {
        return;
    }

    ImGui::Begin("Mixer", &_showMixer, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 10));

    int c[NUM_MIXER_CHANNELS] = {0};
    mostInsertEffectsPerChannel = 0;
    for (int i = 0; i < NUM_INS_EFX; i++)
    {
        if (_mixer->Pinsparts[i] == -1)
        {
            continue;
        }
        c[_mixer->Pinsparts[i]] = c[_mixer->Pinsparts[i]] + 1;
        if (c[_mixer->Pinsparts[i]] > mostInsertEffectsPerChannel)
        {
            mostInsertEffectsPerChannel = c[_mixer->Pinsparts[i]];
        }
    }

    for (int track = 0; track <= NUM_MIXER_CHANNELS; track++)
    {
        auto highlightTrack = _sequencer.ActiveInstrument() == track;
        ImGui::PushID(track);
        ImGuiTrack(track, highlightTrack);
        ImGui::SameLine();
        ImGui::PopID();
    }

    ImGui::PopStyleVar();
    ImGui::End();
}

void AppThreeDee::ImGuiInspector()
{
    if (!_showInspector)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 10));
    ImGui::Begin("Inspector", &_showInspector, ImVec2(trackSize.x * 2, 0), -1.0f, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    {
        ImGuiMasterTrack();
        ImGui::SameLine();
        ImGuiTrack(_sequencer.ActiveInstrument(), false);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

unsigned char indexOf(std::vector<char const *> const &values, std::string const &selectedValue)
{
    for (size_t i = 0; i < values.size(); i++)
    {
        if (std::string(values[i]) == selectedValue)
            return static_cast<unsigned char>(i);
    }
    return 0;
}

void AppThreeDee::ImGuiMasterTrack()
{
    auto io = ImGui::GetStyle();

    ImGui::BeginChild("Master Track", trackSize, true);
    {
        auto availableRegion = ImGui::GetContentRegionAvail();
        auto width = availableRegion.x;

        // Output devices
        auto sinks = toCharVector(Nio::GetSinks());
        auto selectedSink = indexOf(sinks, Nio::GetSelectedSink());
        ImGui::PushItemWidth(width);
        if (ImGui::DropDown("##Sinks", selectedSink, &sinks[0], static_cast<int>(sinks.size()), "Ouput device"))
        {
            Nio::SelectSink(sinks[static_cast<size_t>(selectedSink)]);
        }

        // Input devices
        auto sources = toCharVector(Nio::GetSources());
        auto selectedSource = indexOf(sources, Nio::GetSelectedSource());
        ImGui::PushItemWidth(width);
        if (ImGui::DropDown("##Sources", selectedSource, &sources[0], static_cast<int>(sources.size()), "Midi device"))
        {
            Nio::SelectSource(sources[static_cast<size_t>(selectedSource)]);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 2));

        // Enable/disable NRPN
        auto nrpn = _mixer->ctl.NRPN.receive == 1;
        if (ImGui::Checkbox("NRPN", &nrpn))
        {
            _mixer->ctl.NRPN.receive = nrpn ? 1 : 0;
        }
        ImGui::ShowTooltipOnHover("Receive NRPNs");

        // Enable/disable Portamento
        auto portamento = _mixer->ctl.portamento.portamento == 1;
        if (ImGui::Checkbox("Portamento", &portamento))
        {
            _mixer->ctl.portamento.portamento = portamento ? 1 : 0;
        }
        ImGui::ShowTooltipOnHover("Enable/Disable the portamento");

        ImGui::PopStyleVar();

        ImGui::Spacing();

        ImGui::Separator();

        // System effects
        if (ImGui::CollapsingHeader("Sys FX"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
            for (int fx = 0; fx < NUM_SYS_EFX; fx++)
            {
                ImGui::PushID(fx);
                ImGui::PushStyleColor(ImGuiCol_Button, _mixer->sysefx[fx].geteffect() == 0 ? ImVec4(0.5f, 0.5f, 0.5f, 0.2f) : io.Colors[ImGuiCol_Button]);
                if (ImGui::Button(effectNames[_mixer->sysefx[fx].geteffect()], ImVec2(width - (_mixer->sysefx[fx].geteffect() == 0 ? 0 : 22), 20)))
                {
                    _currentSystemEffect = fx;
                    _showSystemEffectsEditor = true;
                    ImGui::SetWindowFocus(SystemFxEditorID);
                }
                if (_mixer->sysefx[fx].geteffect() != 0)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("x", ImVec2(20, 20)))
                    {
                        _currentSystemEffect = fx;
                        _showSystemEffectsEditor = true;
                        _mixer->sysefx[fx].changeeffect(0);
                    }
                    ImGui::ShowTooltipOnHover("Remove system effect");
                }
                ImGui::PopStyleColor(1);
                ImGui::PopID();
            }

            ImGui::PopStyleVar();

            ImGui::Spacing();
        }

        // Fine detune
        auto fineDetune = _mixer->microtonal.Pglobalfinedetune;
        if (ImGui::KnobUchar("fine detune", &fineDetune, 0, 127, ImVec2(width, 40), "Global fine detune"))
        {
            _mixer->microtonal.Pglobalfinedetune = fineDetune;
        }

        auto faderHeight = ImGui::GetWindowContentRegionMax().y - ImGui::GetCursorPos().y - io.ItemSpacing.y - 20;

        // Master volume
        if (faderHeight < (40 + ImGui::GetTextLineHeight()))
        {
            auto v = _mixer->Pvolume;
            if (ImGui::KnobUchar("volume", &v, 0, 127, ImVec2(width, 40), "Master Volume"))
            {
                _mixer->setPvolume(v);
            }
        }
        else
        {
            static float olddbl = 1.0e-12f;
            static float olddbr = 1.0e-12f;
            static float oldrmsdbl = 1.0e-12f;
            static float oldrmsdbr = 1.0e-12f;

            vuData data = _mixer->GetMeter()->GetVuData();

            float dbl = rap2dB(data.outpeakl);
            float dbr = rap2dB(data.outpeakr);
            float rmsdbl = rap2dB(data.rmspeakl);
            float rmsdbr = rap2dB(data.rmspeakr);

            dbl = (MIN_DB - dbl) / MIN_DB;
            if (dbl < 0.0f)
                dbl = 0.0f;
            else if (dbl > 1.0f)
                dbl = 1.0f;

            dbr = (MIN_DB - dbr) / MIN_DB;
            if (dbr < 0.0f)
                dbr = 0.0f;
            else if (dbr > 1.0f)
                dbr = 1.0f;

            dbl = dbl * 0.4f + olddbl * 0.6f;
            dbr = dbr * 0.4f + olddbr * 0.6f;

            olddbl = dbl;
            olddbr = dbr;

            //compute RMS - start
            rmsdbl = (MIN_DB - rmsdbl) / MIN_DB;
            if (rmsdbl < 0.0f)
                rmsdbl = 0.0f;
            else if (rmsdbl > 1.0f)
                rmsdbl = 1.0f;

            rmsdbr = (MIN_DB - rmsdbr) / MIN_DB;
            if (rmsdbr < 0.0f)
                rmsdbr = 0.0f;
            else if (rmsdbr > 1.0f)
                rmsdbr = 1.0f;

            rmsdbl = rmsdbl * 0.4f + oldrmsdbl * 0.6f;
            rmsdbr = rmsdbr * 0.4f + oldrmsdbr * 0.6f;

            oldrmsdbl = rmsdbl;
            oldrmsdbr = rmsdbr;

            rmsdbl *= faderHeight;
            rmsdbr *= faderHeight;

            int irmsdbl = static_cast<int>(rmsdbl);
            int irmsdbr = static_cast<int>(rmsdbr);
            //compute RMS - end

            ImGui::Spacing();
            ImGui::SameLine(0.0f, (width - (60.0f + (4 * io.ItemSpacing.x))) / 2.0f);

            ImGui::UvMeter("##mastervoll", ImVec2(20, faderHeight), &irmsdbl, 0, static_cast<int>(faderHeight));
            ImGui::SameLine();

            ImGui::Spacing();
            ImGui::SameLine();
            int v = static_cast<int>(_mixer->Pvolume);
            if (ImGui::VSliderInt("##mastervol", ImVec2(20, faderHeight), &v, 0, 127))
            {
                _mixer->setPvolume(static_cast<unsigned char>(v));
            }
            ImGui::SameLine();
            ImGui::Spacing();

            ImGui::SameLine();
            ImGui::UvMeter("##mastervolr", ImVec2(20, faderHeight), &irmsdbr, 0, static_cast<int>(faderHeight));
        }
        ImGui::ShowTooltipOnHover("Master volume");

        ImGui::TextCentered(ImVec2(width, 20), "master");
    }
    ImGui::EndChild();
}

void AppThreeDee::ImGuiTrack(int track, bool highlightTrack)
{
    if (track < 0 || track >= NUM_MIXER_CHANNELS)
    {
        return;
    }

    auto channel = _mixer->GetChannel(track);

    if (channel == nullptr)
    {
        return;
    }

    auto io = ImGui::GetStyle();

    auto hue = track * 0.05f;
    if (highlightTrack)
    {
        ImGui::PushStyleColor(ImGuiCol_Border, static_cast<ImVec4>(ImColor::HSV(hue, 0.8f, 0.8f)));
    }

    std::stringstream trackTooltip;
    trackTooltip << "[" << instrumentCategoryNames[channel->info.Ptype] << "]";
    if (channel->Pname[0])
    {
        trackTooltip << " " << channel->Pname;
    }
    if (channel->info.Pauthor[0])
    {
        trackTooltip << " by " << channel->info.Pauthor;
    }
    if (channel->info.Pcomments[0])
    {
        trackTooltip << "\n---\n"
                     << channel->info.Pcomments;
    }

    ImGui::BeginChild("Track", trackSize, true);
    {
        auto availableRegion = ImGui::GetContentRegionAvail();
        auto width = availableRegion.x;
        auto useLargeMode = availableRegion.y > sliderBaseHeight * largeModeTreshold;

        auto trackEnabled = channel->Penabled == 1;

        ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor::HSV(1.0f, 0.0f, 1.0f)));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, static_cast<ImVec4>(ImColor::HSV(1.0f, 0.0f, 1.0f)));
        ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(hue, 0.6f, 0.6f)));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, static_cast<ImVec4>(ImColor::HSV(hue, 0.6f, 0.6f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(hue, 0.7f, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, static_cast<ImVec4>(ImColor::HSV(hue, 0.7f, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(hue, 0.8f, 0.8f)));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, static_cast<ImVec4>(ImColor::HSV(hue, 0.8f, 0.8f)));

        // Enable/disable channel
        if (ImGui::Checkbox("##trackEnabled", &trackEnabled))
        {
            _sequencer.ActiveInstrument(track);
            channel->Penabled = trackEnabled ? 1 : 0;
        }
        ImGui::ShowTooltipOnHover(trackEnabled ? "This track is enabled" : "This track is disabled");

        ImGui::SameLine();

        // Change channel presets
        auto name = std::string(reinterpret_cast<char *>(channel->Pname));
        if (ImGui::Button(name.size() == 0 ? "default" : name.c_str(), ImVec2(width - 20 - io.ItemSpacing.x, 0)))
        {
            _sequencer.ActiveInstrument(track);
            _openSelectInstrument = track;
        }
        ImGui::ShowTooltipOnHover("Change instrument preset");

        ImGui::PopStyleColor(8);

        // Select midi channel
        if (ImGui::CollapsingHeader("MIDI channel"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
            ImGui::PushItemWidth(width);
            if (ImGui::DropDown("##KeyboardChannel", channel->Prcvchn, channels, NUM_MIXER_CHANNELS, "Midi channel"))
            {
                _sequencer.ActiveInstrument(track);
            }

            if (ImGui::IsItemClicked())
            {
                _sequencer.ActiveInstrument(track);
            }

            ImGui::PopStyleVar();

            ImGui::Spacing();
        }
        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        // AD synth enable/disable + edit button
        auto adEnabled = channel->instruments[0].Padenabled == 1;
        if (ImGui::Checkbox("##adEnabled", &adEnabled))
        {
            channel->instruments[0].Padenabled = adEnabled ? 1 : 0;
            _sequencer.ActiveInstrument(track);
            if (adEnabled)
            {
                _showADNoteEditor = true;
            }
        }
        ImGui::ShowTooltipOnHover(adEnabled ? "The AD synth is enabled" : "The AD synth is disabled");
        ImGui::SameLine();
        if (ImGui::Button("AD", ImVec2(width - 21, 19)))
        {
            _showADNoteEditor = true;
            ImGui::SetWindowFocus(ADeditorID);
            _sequencer.ActiveInstrument(track);
        }
        ImGui::ShowTooltipOnHover("Edit the AD synth");

        // SUB synth enable/disable + edit button
        auto subEnabled = channel->instruments[0].Psubenabled == 1;
        if (ImGui::Checkbox("##subEnabled", &subEnabled))
        {
            channel->instruments[0].Psubenabled = subEnabled ? 1 : 0;
            _sequencer.ActiveInstrument(track);
            if (subEnabled)
            {
                _showSUBNoteEditor = true;
            }
        }
        ImGui::ShowTooltipOnHover(adEnabled ? "The SUB synth is enabled" : "The AD synth is disabled");
        ImGui::SameLine();
        if (ImGui::Button("SUB", ImVec2(width - 21, 19)))
        {
            _showSUBNoteEditor = true;
            ImGui::SetWindowFocus(SUBeditorID);
            _sequencer.ActiveInstrument(track);
        }
        ImGui::ShowTooltipOnHover("Edit the SUB synth");

        // PAD synth enable/disable + edit button
        auto padEnabled = channel->instruments[0].Ppadenabled == 1;
        if (ImGui::Checkbox("##padEnabled", &padEnabled))
        {
            channel->instruments[0].Ppadenabled = padEnabled ? 1 : 0;
            _sequencer.ActiveInstrument(track);
            if (padEnabled)
            {
                _showPADNoteEditor = true;
            }
        }
        ImGui::ShowTooltipOnHover(adEnabled ? "The PAD synth is enabled" : "The AD synth is disabled");
        ImGui::SameLine();
        if (ImGui::Button("PAD", ImVec2(width - 21, 19)))
        {
            _showPADNoteEditor = true;
            ImGui::SetWindowFocus(PADeditorID);
            _sequencer.ActiveInstrument(track);
        }
        ImGui::ShowTooltipOnHover("Edit the PAD synth");

        ImGui::PopStyleVar();

        ImGui::Spacing();

        // System effect sends
        if (ImGui::CollapsingHeader("Sys FX sends"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

            for (int fx = 0; fx < NUM_SYS_EFX; fx++)
            {
                ImGui::PushID(fx);
                ImGui::PushStyleColor(ImGuiCol_Button, _mixer->sysefx[fx].geteffect() == 0 ? ImVec4(0.5f, 0.5f, 0.5f, 0.2f) : io.Colors[ImGuiCol_Button]);
                if (ImGui::Button(effectNames[_mixer->sysefx[fx].geteffect()], ImVec2(width - 21, 20)))
                {
                    _currentSystemEffect = fx;
                    _showSystemEffectsEditor = true;
                    ImGui::SetWindowFocus(SystemFxEditorID);
                }
                if (ImGui::IsItemClicked())
                {
                    _sequencer.ActiveInstrument(track);
                }

                ImGui::SameLine();

                char label[64] = {'\0'};
                sprintf(label, "##send_%d", fx);
                char tooltip[64] = {'\0'};
                sprintf(tooltip, "Volume for send to system effect %d", (fx + 1));
                if (ImGui::KnobUchar(label, &(_mixer->Psysefxvol[fx][track]), 0, 127, ImVec2(20, 20), tooltip))
                {
                    _mixer->setPsysefxvol(track, fx, _mixer->Psysefxvol[fx][track]);
                }
                if (ImGui::IsItemClicked())
                {
                    _sequencer.ActiveInstrument(track);
                }

                ImGui::PopStyleColor(1);
                ImGui::PopID();
            }

            ImGui::PopStyleVar(1);

            ImGui::Spacing();
        }
        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }

        // Insertion effects
        if (ImGui::CollapsingHeader("Insert FX"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

            int fillCount = mostInsertEffectsPerChannel;
            for (int fx = 0; fx < NUM_INS_EFX; fx++)
            {
                ImGui::PushID(100 + fx);
                if (_mixer->Pinsparts[fx] == track)
                {
                    if (ImGui::Button(effectNames[_mixer->insefx[fx].geteffect()], ImVec2(width - 22, 20)))
                    {
                        _currentInsertEffect = fx;
                        _sequencer.ActiveInstrument(track);
                        _showInsertEffectsEditor = true;
                        ImGui::SetWindowFocus(InsertionFxEditorID);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("x", ImVec2(20, 20)))
                    {
                        RemoveInsertFxFromTrack(fx);
                        _sequencer.ActiveInstrument(track);
                    }
                    ImGui::ShowTooltipOnHover("Remove insert effect from track");
                    fillCount--;
                }
                ImGui::PopID();
            }
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            for (int i = 0; i < fillCount; i++)
            {
                ImGui::PushID(i);
                ImGui::Button("##empty", ImVec2(width, 20));
                ImGui::PopID();
            }
            ImGui::PopStyleColor(3);

            if (ImGui::Button("+", ImVec2(width, 20)))
            {
                AddInsertFx(track);
                _sequencer.ActiveInstrument(track);
            }
            ImGui::ShowTooltipOnHover("Add insert effect to track");

            ImGui::PopStyleVar(1);

            ImGui::Spacing();
        }
        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }

        // Channel effects
        if (ImGui::CollapsingHeader("Audio FX"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

            for (int fx = 0; fx < NUM_CHANNEL_EFX; fx++)
            {
                ImGui::PushID(200 + fx);
                ImGui::PushStyleColor(ImGuiCol_Button, channel->partefx[fx]->geteffect() == 0 ? ImVec4(0.5f, 0.5f, 0.5f, 0.2f) : io.Colors[ImGuiCol_Button]);
                if (ImGui::Button(effectNames[channel->partefx[fx]->geteffect()], ImVec2(width - (channel->partefx[fx]->geteffect() == 0 ? 0 : 22), 20)))
                {
                    _sequencer.ActiveInstrument(track);
                    _currentInstrumentEffect = fx;
                    _showInstrumentEffectsEditor = true;
                    ImGui::SetWindowFocus(InstrumentFxEditorID);
                }
                if (channel->partefx[fx]->geteffect() != 0)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("x", ImVec2(20, 20)))
                    {
                        _sequencer.ActiveInstrument(track);
                        _currentInstrumentEffect = fx;
                        channel->partefx[fx]->changeeffect(0);
                    }
                    ImGui::ShowTooltipOnHover("Remove effect from track");
                }
                ImGui::PopStyleColor(1);
                ImGui::PopID();
            }
            ImGui::PopStyleVar(1);

            ImGui::Spacing();
        }
        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }

        if (ImGui::CollapsingHeader("Velocity"))
        {
            if (ImGui::KnobUchar("vel.sns.", &channel->Pvelsns, 0, 127, ImVec2(width / 2, 30), "Velocity Sensing Function"))
            {
                _sequencer.ActiveInstrument(track);
            }

            ImGui::SameLine();

            if (ImGui::KnobUchar("vel.ofs.", &channel->Pveloffs, 0, 127, ImVec2(width / 2, 30), "Velocity Offset"))
            {
                _sequencer.ActiveInstrument(track);
            }
        }
        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }

        if (useLargeMode && _iconImagesAreLoaded)
        {
            ImGui::Spacing();

            ImGui::SameLine(0.0f, (width - 64 - io.ItemSpacing.x) / 2);
            if (ImGui::ImageButton(reinterpret_cast<void *>(_iconImages[channel->info.Ptype]), ImVec2(64, 64)))
            {
                _openChangeInstrumentType = track;
                _sequencer.ActiveInstrument(track);
            }
            ImGui::ShowTooltipOnHover(trackTooltip.str().c_str());
        }

        auto panning = channel->Ppanning;
        if (ImGui::KnobUchar("panning", &panning, 0, 127, ImVec2(width, 40), "Track panning"))
        {
            channel->setPpanning(panning);
            _sequencer.ActiveInstrument(track);
        }

        auto start = ImGui::GetCursorPos();
        auto faderHeight = ImGui::GetWindowContentRegionMax().y - start.y - io.ItemSpacing.y - 20;

        if (faderHeight < (40 + ImGui::GetTextLineHeight()))
        {
            auto v = channel->Pvolume;
            if (ImGui::KnobUchar("volume", &v, 0, 127, ImVec2(width, 40), "Track volume"))
            {
                channel->setPvolume(v);
                _sequencer.ActiveInstrument(track);
            }
        }
        else
        {
            float db = rap2dB(_mixer->GetMeter()->GetOutPeak(track));

            db = (MIN_DB - db) / MIN_DB;
            if (db < 0.0f)
                db = 0.0f;
            else if (db > 1.0f)
                db = 1.0f;

            db *= faderHeight;

            int idb = static_cast<int>(db);

            ImGui::Spacing();
            ImGui::SameLine(0.0f, (width - (40.0f + (3 * io.ItemSpacing.x))) / 2.0f);

            int v = static_cast<int>(channel->Pvolume);
            if (ImGui::VSliderInt("##vol", ImVec2(20, faderHeight), &v, 0, 127))
            {
                channel->setPvolume(static_cast<unsigned char>(v));
                _sequencer.ActiveInstrument(track);
            }
            ImGui::ShowTooltipOnHover("Track volume");
            ImGui::SameLine();
            ImGui::Spacing();

            ImGui::SameLine();
            ImGui::UvMeter("##instrument_uvr", ImVec2(20, faderHeight), &idb, 0, static_cast<int>(faderHeight));
        }

        char tmp[32] = {0};
        sprintf(tmp, "track %d", track + 1);
        ImGui::TextCentered(ImVec2(width, 20), tmp);
        ImGui::ShowTooltipOnHover(trackTooltip.str().c_str());

        if (ImGui::IsItemClicked())
        {
            _sequencer.ActiveInstrument(track);
        }
    }
    ImGui::EndChild();
    if (ImGui::IsItemClicked())
    {
        _sequencer.ActiveInstrument(track);
    }

    if (highlightTrack)
    {
        ImGui::PopStyleColor();
    }
}

void AppThreeDee::ImGuiSelectInstrumentPopup()
{
    if (_openSelectInstrument < 0)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(900, 850));
    if (ImGui::Begin("Select Instrument"))
    {
        static bool autoClose = false;
        ImGui::SameLine();
        ImGui::Checkbox("Auto close", &autoClose);

        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            _openSelectInstrument = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 200);
        ImGui::SetColumnWidth(1, 700);
        auto count = _mixer->GetBankManager()->GetBankCount();
        auto const &bankNames = _mixer->GetBankManager()->GetBankNames();
        for (int i = 0; i < count; i++)
        {
            bool selected = _currentBank == i;
            if (ImGui::Selectable(bankNames[static_cast<size_t>(i)], &selected))
            {
                _currentBank = i;
                _mixer->GetBankManager()->LoadBank(_currentBank);
            }
        }
        ImGui::NextColumn();

        ImGui::BeginChild("banks", ImVec2(0, -20));
        ImGui::Columns(5);
        if (_currentBank >= 0)
        {
            for (unsigned int i = 0; i < BANK_SIZE; i++)
            {
                auto instrumentName = _mixer->GetBankManager()->GetName(i);

                if (ImGui::Button(instrumentName.c_str(), ImVec2(120, 20)))
                {
                    auto const &instrument = _mixer->GetChannel(_sequencer.ActiveInstrument());
                    instrument->Lock();
                    _mixer->GetBankManager()->LoadFromSlot(i, instrument);
                    instrument->Unlock();
                    instrument->ApplyParameters();
                    if (autoClose)
                    {
                        _openSelectInstrument = -1;
                        ImGui::CloseCurrentPopup();
                    }
                }
                if ((i + 1) % 32 == 0)
                {
                    ImGui::NextColumn();
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }
}

void AppThreeDee::ImGuiChangeInstrumentTypePopup()
{
    if (!_iconImagesAreLoaded)
    {
        return;
    }

    if (_openChangeInstrumentType < 0)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(570, 500));
    if (ImGui::Begin("Select Instrument Type"))
    {
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            _openChangeInstrumentType = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::BeginChild("types", ImVec2(0, -20));

        for (int i = 0; i < int(InstrumentCategories::COUNT); i++)
        {
            if (i % 5 != 0)
            {
                ImGui::SameLine();
            }
            if (ImGui::ImageButton(reinterpret_cast<void *>(_iconImages[i]), ImVec2(96, 96)))
            {
                auto channel = _mixer->GetChannel(_openChangeInstrumentType);
                if (channel != nullptr)
                {
                    channel->info.Ptype = static_cast<unsigned char>(i);
                    _openChangeInstrumentType = -1;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::ShowTooltipOnHover(instrumentCategoryNames[i]);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}

void AppThreeDee::AddInsertFx(int track)
{
    for (int i = 0; i < NUM_INS_EFX; i++)
    {
        if (_mixer->Pinsparts[i] == -1)
        {
            _mixer->Pinsparts[i] = static_cast<short>(track);
            return;
        }
    }
}

void AppThreeDee::RemoveInsertFxFromTrack(int fx)
{
    _mixer->Pinsparts[fx] = -1;
}

#include "app.threedee.h"

#include <zyn.mixer/Track.h>
#include <zyn.nio/MidiInputManager.h>
#include <zyn.seq/ArpModes.h>
#include <zyn.seq/Chords.h>
#include <zyn.seq/NotesGenerator.h>
#include <zyn.synth/ADnoteParams.h>

#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "imgui_addons/imgui_Timeline.h"
#include "imgui_addons/imgui_checkbutton.h"
#include "imgui_addons/imgui_knob.h"
#include "stb_image.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <map>

char const *const NoteNames[] = {
    "B",
    "A#",
    "A",
    "G#",
    "G",
    "F#",
    "F",
    "E",
    "D#",
    "D",
    "C#",
    "C",
};

unsigned int NoteNameCount = 12;

char const *const SnappingModes[] = {
    "Bar",
    "Beat",
    "Division",
};

unsigned int SnappingModeCount = 3;

timestep SnappingModeValues[] = {
    1024,
    1024 / 4,
    (1024 / 4) / 4,
};

static ImVec4 clear_color = ImColor(90, 90, 100);

AppThreeDee::AppThreeDee(GLFWwindow *window, Mixer *mixer, IBankManager *banks)
    : _state(mixer, banks), _adNoteUI(&_state), _effectUi(&_state), _libraryUi(&_state),
      _mixerUi(&_state), _padNoteUi(&_state), _subNoteUi(&_state),
      _window(window),
      _toolbarIconsAreLoaded(false),
      _display_w(800), _display_h(600)
{
    glfwSetWindowUserPointer(this->_window, static_cast<void *>(this));
}

AppThreeDee::~AppThreeDee()
{
    glfwSetWindowUserPointer(this->_window, nullptr);
}

void AppThreeDee::KeyActionCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto app = static_cast<AppThreeDee *>(glfwGetWindowUserPointer(window));

    if (app != nullptr)
    {
        app->onKeyAction(key, scancode, action, mods);
        ImGui_ImplGlfw_KeyCallback(app->_window, key, scancode, action, mods);
    }
}

void AppThreeDee::ResizeCallback(GLFWwindow *window, int width, int height)
{
    auto app = static_cast<AppThreeDee *>(glfwGetWindowUserPointer(window));

    if (app != nullptr) app->onResize(width, height);
}

void AppThreeDee::onResize(int width, int height)
{
    this->_display_w = width;
    this->_display_h = height;

    glViewport(0, 0, width, height);
}

bool AppThreeDee::Setup()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigDockingWithShift = false;

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGui::StyleColorsDark();
    ImGui::GetStyle().TabRounding = 2.0f;
    ImGui::GetStyle().FrameRounding = 2.0f;

    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0.71f, 0.7f, 0.7f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] = ImVec4(0.51f, 0.5f, 0.5f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.49f, 0.48f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive] = ImGui::GetStyle().Colors[ImGuiCol_TabActive];
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocused] = ImGui::GetStyle().Colors[ImGuiCol_Tab];
    ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.24f, 0.27f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.14f, 0.14f, 1.0f);

    io.Fonts->Clear();
    ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    if (font != nullptr)
    {
        io.FontDefault = font;
    }
    else
    {
        io.Fonts->AddFontDefault();
    }
    io.Fonts->Build();

    _state._banks->RescanForBanks();
    _state._banks->LoadBank(_state._currentBank);

    _state._currentTrack = 0;

    _mixerUi.Setup();
    _libraryUi.Setup();
    _adNoteUI.Setup();
    _subNoteUi.Setup();
    _padNoteUi.Setup();
    _effectUi.Setup();

    LoadToolbarIcons();

    _state._playTime = 0.0f;

    return true;
}

void AppThreeDee::TickRegion(TrackRegion &region, unsigned char trackIndex, float prevPlayTime, float currentPlayTime, int repeat)
{
    auto track = _state._mixer->GetTrack(trackIndex);
    auto regionSize = region.startAndEnd[1] - region.startAndEnd[0];
    auto regionStart = region.startAndEnd[0] + repeat * regionSize;
    auto regionEnd = region.startAndEnd[1] + repeat * regionSize;

    if (regionStart < prevPlayTime && regionEnd < prevPlayTime)
    {
        return;
    }
    if (regionStart > currentPlayTime && regionEnd > currentPlayTime)
    {
        return;
    }

    MidiEvent ev;
    ev.type = MidiEventTypes::M_NOTE;
    ev.channel = track->Prcvchn;

    for (unsigned char noteIndex = 0; noteIndex < NUM_MIDI_NOTES; noteIndex++)
    {
        ev.num = noteIndex;
        for (auto &event : region.eventsByNote[noteIndex])
        {
            auto start = (regionStart + event.values[0]);
            auto end = (regionStart + event.values[1]);
            if (start >= prevPlayTime && start < currentPlayTime)
            {
                ev.value = 100;
                MidiInputManager::Instance().PutEvent(ev);
            }
            if (end >= prevPlayTime && end < currentPlayTime)
            {
                ev.value = 0;
                MidiInputManager::Instance().PutEvent(ev);
            }
        }
    }
}

void AppThreeDee::TempNoteOn(unsigned int channel, unsigned int note, unsigned int length)
{
    MidiEvent ev;
    ev.type = MidiEventTypes::M_NOTE;
    ev.channel = channel;
    ev.value = 100;
    ev.num = note;

    MidiInputManager::Instance().PutEvent(ev);

    for (auto &tn : _state._tempnotes)
    {
        if (tn.note == note && tn.channel == channel)
        {
            tn.done = false;
            tn.playUntil = _lastSequencerTimeInMs + length;
            return;
        }
    }

    tempnote n;
    n.playUntil = _lastSequencerTimeInMs + length;
    n.note = note;
    n.channel = channel;
    n.done = false;
    _state._tempnotes.push_back(n);
}

void AppThreeDee::Tick()
{
    std::chrono::milliseconds::rep currentTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    auto deltaTime = currentTime - _lastSequencerTimeInMs;

    for (auto &tn : _state._tempnotes)
    {
        if (tn.done) continue;
        if (tn.playUntil < currentTime)
        {
            tn.done = true;
            MidiEvent ev;
            ev.type = MidiEventTypes::M_NOTE;
            ev.channel = tn.channel;
            ev.value = 0;
            ev.num = tn.note;

            MidiInputManager::Instance().PutEvent(ev);
        }
    }

    _lastSequencerTimeInMs = currentTime;

    if (_state._isPlaying)
    {
        float bpmValue = float(_state._bpm) / 60.0f;
        deltaTime *= bpmValue;

        auto prevPlayTime = (_state._playTime);
        _state._playTime += deltaTime;
        auto currentPlayTime = _state._playTime;

        for (unsigned char trackIndex = 0; trackIndex < NUM_MIXER_TRACKS; trackIndex++)
        {
            auto *track = _state._mixer->GetTrack(trackIndex);
            if (!track->Penabled)
            {
                continue;
            }
            for (auto &region : _state._regions.GetRegionsByTrack(trackIndex))
            {
                for (int i = 0; i <= region.repeat; i++)
                {
                    TickRegion(region, trackIndex, prevPlayTime, currentPlayTime, i);
                }
            }
        }

        if (_state._playTime >= _state._maxPlayTime)
        {
            _state._playTime = 0;
        }
    }
}

void AppThreeDee::PianoRollEditor()
{
    static struct TrackRegionEvent *selectedEvent = nullptr;
    if (ImGui::Begin("Pianoroll editor"))
    {
        if (_state._currentTrack < 0 || _state._currentTrack >= NUM_MIXER_TRACKS)
        {
            ImGui::End();
            return;
        }

        auto track = _state._mixer->GetTrack(_state._currentTrack);

        if (!_state._regions.DoesRegionExist(_state._currentTrack, _state._currentPattern))
        {
            ImGui::End();
            return;
        }

        auto &region = _state._regions.GetRegion(_state._currentTrack, _state._currentPattern);
        auto maxvalue = region.startAndEnd[1] - region.startAndEnd[0];

        ImGui::Text("Zoom");
        ImGui::SameLine();
        ImGui::PushItemWidth(120);
        ImGui::SliderInt("##horizontalZoom", &(_state._pianoRollEditorHorizontalZoom), 100, 300, "horizontal %d");

        ImGui::SameLine();

        static unsigned int current_snapping_mode = 2;

        ImGui::PushItemWidth(100);
        if (ImGui::BeginCombo("Snapping mode", SnappingModes[current_snapping_mode]))
        {
            for (unsigned int n = 0; n < SnappingModeCount; n++)
            {
                bool is_selected = (current_snapping_mode == n);
                if (ImGui::Selectable(SnappingModes[n], is_selected))
                {
                    current_snapping_mode = n;
                }
            }

            ImGui::EndCombo();
        }
        ImGui::ShowTooltipOnHover("Set the Snap value for the Piano Roll Editor");

        timestep elapsedTime = (static_cast<unsigned>(_state._playTime)) - region.startAndEnd[0];

        static unsigned int _baseNote = 65;

        if (ImGui::BeginChild("##timelinechild", ImVec2(0, -30)))
        {
            auto hue = _state._currentTrack * 0.05f;
            auto tintColor = ImColor::HSV(hue, 0.6f, 0.6f);

            bool regionIsModified = false;
            if (ImGui::BeginTimelines("MyTimeline", &maxvalue, 20, _state._pianoRollEditorHorizontalZoom, 88, SnappingModeValues[current_snapping_mode]))
            {
                for (unsigned int c = NUM_MIDI_NOTES - 1; c > 0; c--)
                {
                    char id[32];
                    sprintf(id, "%4s%d", NoteNames[(107 - c) % NoteNameCount], (107 - c) / NoteNameCount - 1);
                    ImGui::TimelineStart(id);
                    if (ImGui::IsItemClicked())
                    {
                        TempNoteOn(track->Prcvchn, c, 400);
                    }

                    for (size_t i = 0; i < region.eventsByNote[c].size(); i++)
                    {
                        bool selected = (&(region.eventsByNote[c][i]) == selectedEvent);
                        if (ImGui::TimelineEvent(region.eventsByNote[c][i].values, 0, tintColor, &selected))
                        {
                            regionIsModified = true;
                            selectedEvent = &(region.eventsByNote[c][i]);
                            _baseNote = c;
                        }
                    }
                    timestep new_values[2];
                    if (ImGui::TimelineEnd(new_values))
                    {
                        regionIsModified = true;
                        TrackRegionEvent e{
                            {
                                std::min(new_values[0], new_values[1]),
                                std::max(new_values[0], new_values[1]),
                            },
                            static_cast<unsigned char>(c),
                            100,
                        };

                        region.eventsByNote[c].push_back(e);
                        selectedEvent = &(region.eventsByNote[c].back());
                        _baseNote = c;
                    }
                }
            }
            ImGui::EndTimelines(&elapsedTime);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete)) && selectedEvent != nullptr)
            {
                if (_state._regions.DoesRegionExist(_state._currentTrack, _state._currentPattern))
                {
                    auto &events = _state._regions.GetRegion(_state._currentTrack, _state._currentPattern).eventsByNote[selectedEvent->note];
                    auto itr = events.begin();
                    while (&(*itr) != selectedEvent && itr != events.end())
                    {
                        itr++;
                    }
                    if (itr != events.end())
                    {
                        events.erase(itr);
                        regionIsModified = true;
                    }
                }
            }

            if (regionIsModified)
            {
                UpdatePreviewImage(region);
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Clear all notes"))
        {
            _state._regions.ClearAllNotesInRegion(region);
        }

        ImGui::SameLine();

        ImGui::VerticalSeparator();

        ImGui::SameLine();

        static unsigned char selectedArpMode = 0;
        static unsigned char selectedChord = 0;
        static int space = 1;
        if (ImGui::Button("Generate Notes from selection"))
        {
            NotesGeneratorOptions options = {
                ArpModes::ToEnum(selectedArpMode),
                Chords::ToEnum(selectedChord),
                space,
            };
            NotesGenerator generator(options);
            generator.Generate(&(_state._regions), _state._currentTrack, _state._currentPattern, *selectedEvent);
        }

        ImGui::SameLine();

        ImGui::PushItemWidth(200);
        ImGui::DropDown("##ArpMode", selectedArpMode, &(ArpModes::Names[0]), ArpModes::Enum::Count, "Arpeggio Mode");

        ImGui::SameLine();

        ImGui::PushItemWidth(200);
        ImGui::DropDown("##Chord", selectedChord, &(Chords::Names[0]), (Chords::Enum::Count), "Chord");

        ImGui::SameLine();

        ImGui::PushItemWidth(200);
        ImGui::SliderInt("Space", &space, 0, 16);
    }
    ImGui::End();
}

void AppThreeDee::RegionEditor()
{
    if (ImGui::Begin("Region editor"))
    {
        ImGui::Text("Zoom");
        ImGui::SameLine();
        ImGui::PushItemWidth(120);
        ImGui::SliderInt("##horizontalZoom", &(_state._sequencerHorizontalZoom), 10, 100, "horizontal %d");
        ImGui::SameLine();
        ImGui::PushItemWidth(120);
        ImGui::SliderInt("##verticalZoom", &(_state._sequencerVerticalZoom), 30, 100, "vertical %d");
        ImGui::SameLine();
        ImGui::PushItemWidth(220);
        int maxPlayTime = int(_state._maxPlayTime / 1024);
        if (ImGui::SliderInt("##maxPlayTime", &maxPlayTime, 4, 200, "song length %d"))
        {
            _state._maxPlayTime = maxPlayTime * 1024;
        }

        timestep elapsedTimeSequencer = _state._playTime % _state._maxPlayTime;

        if (ImGui::BeginChild("##timeline2child", ImVec2(0, -30)))
        {
            timestep maxValue = _state._maxPlayTime;
            if (ImGui::BeginTimelines("MyTimeline2", &maxValue, _state._sequencerVerticalZoom, _state._sequencerHorizontalZoom, NUM_MIXER_TRACKS, 1024))
            {
                ImGui::TimelineSetVar(ImGui::TimelineVars::ShowAddRemoveButtons, 1);
                for (int trackIndex = 0; trackIndex < NUM_MIXER_TRACKS; trackIndex++)
                {
                    auto hue = trackIndex * 0.05f;
                    auto tintColor = ImColor::HSV(hue, 0.6f, 0.6f);

                    char id[32];
                    sprintf(id, "Track %d", trackIndex);
                    ImGui::TimelineStart(id);
                    if (ImGui::IsItemClicked())
                    {
                        _state._currentTrack = trackIndex;
                    }

                    auto &regions = _state._regions.GetRegionsByTrack(trackIndex);
                    for (size_t i = 0; i < regions.size(); i++)
                    {
                        bool selected = (trackIndex == _state._currentTrack && int(i) == _state._currentPattern);
                        if (ImGui::TimelineEvent(regions[i].startAndEnd, regions[i].previewImage, tintColor, &selected))
                        {
                            _state._currentTrack = trackIndex;
                            _state._currentPattern = int(i);
                            UpdatePreviewImage(regions[i]);
                        }
                        auto x = regions[i].startAndEnd[1] - regions[i].startAndEnd[0];
                        for (int j = 1; j <= regions[i].repeat; j++)
                        {
                            timestep repeat_values[2]{regions[i].startAndEnd[0] + (x * j), regions[i].startAndEnd[1] + (x * j)};
                            ImGui::TimelineReadOnlyEvent(repeat_values, regions[i].previewImage, tintColor);
                        }
                    }

                    TrackRegion newRegion;
                    if (ImGui::TimelineEnd(newRegion.startAndEnd))
                    {
                        _state._currentTrack = trackIndex;
                        _state._currentPattern = int(_state._regions.GetRegionsByTrack(trackIndex).size());

                        UpdatePreviewImage(newRegion);
                        _state._regions.AddRegion(trackIndex, newRegion);
                    }
                }
            }
            if (ImGui::EndTimelines(&elapsedTimeSequencer))
            {
                _state._maxPlayTime = maxValue;
            }
            _state._playTime = elapsedTimeSequencer;

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete)))
            {
                _state._regions.RemoveRegion(_state._currentTrack, _state._currentPattern);
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void AppThreeDee::Render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO &io = ImGui::GetIO();

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("TestDockspace", nullptr, window_flags);
    {
        ImGuiID dockspace_id = ImGui::GetID("ZynDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    ImGuiPlayback();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 10));
    _effectUi.Render();
    _libraryUi.Render();
    _mixerUi.Render();

    if (_state._showSmartControls)
    {
        _adNoteUI.Render();
        _padNoteUi.Render();
        _subNoteUi.Render();
    }
    ImGui::PopStyleVar();

    PianoRollEditor();
    RegionEditor();

    ImGui::Render();

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    int display_w, display_h;
    glfwMakeContextCurrent(_window);
    glfwGetFramebufferSize(_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void AppThreeDee::onKeyAction(int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/)
{
}

enum class ToolbarTools
{
    Library,
    Inspector,
    SmartControls,
    Mixer,
    Editor,
    QuickHelp,
    Rewind,
    FastForward,
    Stop,
    Play,
    Record,
    COUNT,
};

static char const *const toolbarIconFileNames[] = {
    "library.png",
    "inspector.png",
    "smart-controls.png",
    "mixer.png",
    "editor.png",
    "quick-help.png",
    "rewind.png",
    "fast-forward.png",
    "stop.png",
    "play.png",
    "record.png",
};

void AppThreeDee::LoadToolbarIcons()
{
    std::string rootDir = "./icons/";

    _toolbarIconsAreLoaded = false;

    _toolbarIcons.reserve(static_cast<size_t>(ToolbarTools::COUNT));

    for (size_t i = 0; i < static_cast<size_t>(ToolbarTools::COUNT); i++)
    {
        GLuint my_opengl_texture;
        glGenTextures(1, &my_opengl_texture);

        auto filename = rootDir + toolbarIconFileNames[i];
        int x, y, n;
        unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, 0);
        if (data == nullptr)
        {
            std::cout << "Failed to load instrument category " << i << " from file " << filename << std::endl;
            _toolbarIcons[i] = 0;
            continue;
        }
        _toolbarIcons[i] = my_opengl_texture;
        _toolbarIconsAreLoaded = true;

        glBindTexture(GL_TEXTURE_2D, my_opengl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, n, x, y, 0, n == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
}

void AppThreeDee::ImGuiPlayback()
{
    ImGui::Begin("Playback");
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 5));

        ImGui::ImageToggleButton("toolbar_library", &_state._showLibrary, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Library)]), ImVec2(32, 32));

        ImGui::SameLine();

        ImGui::ImageToggleButton("toolbar_inspector", &_state._showInspector, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Inspector)]), ImVec2(32, 32));

        ImGui::SameLine();

        ImGui::ImageToggleButton("toolbar_quick_help", &_state._showQuickHelp, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::QuickHelp)]), ImVec2(32, 32));

        ImGui::SameLine();

        ImGui::ImageToggleButton("toolbar_smart_controls", &_state._showSmartControls, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::SmartControls)]), ImVec2(32, 32));

        ImGui::SameLine();

        ImGui::ImageToggleButton("toolbar_mixer", &_state._showMixer, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Mixer)]), ImVec2(32, 32));

        ImGui::SameLine();

        ImGui::ImageToggleButton("toolbar_editor", &_state._showEditor, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Editor)]), ImVec2(32, 32));

        ImGui::PopStyleVar();

        ImGui::SameLine();

        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 5));

        ImGui::SameLine();

        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Rewind)]), ImVec2(32, 32)))
        {
            _state._playTime = 0;
            _state._mixer->ShutUp();
        }

        ImGui::SameLine();

        ImGui::ImageButton(reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::FastForward)]), ImVec2(32, 32));

        ImGui::SameLine();

        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Stop)]), ImVec2(32, 32)))
        {
            _state._playTime = 0;
            _state._mixer->ShutUp();
            _state._isPlaying = false;
        }

        ImGui::SameLine();

        ImGui::SameLine();

        bool isPlaying = _state._isPlaying;
        if (ImGui::ImageToggleButton("toolbar_play", &isPlaying, reinterpret_cast<ImTextureID>(_toolbarIcons[int(ToolbarTools::Play)]), ImVec2(32, 32)))
        {
            _state._isPlaying = !_state._isPlaying;
        }

        ImGui::SameLine();

        ImGui::PopStyleVar();

        ImGui::Spacing();

        ImGui::SameLine();

        ImGui::PushItemWidth(100);
        ImGui::InputInt("##bpm", &(_state._bpm));

        ImGui::SameLine();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0 / static_cast<double>(ImGui::GetIO().Framerate), static_cast<double>(ImGui::GetIO().Framerate));

        ImGui::End();
    }
}

void AppThreeDee::Cleanup()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

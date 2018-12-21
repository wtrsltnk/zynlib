#include "app.threedee.h"

#include <zyn.mixer/Channel.h>
#include <zyn.synth/ADnoteParams.h>

#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "imgui_addons/imgui_checkbutton.h"
#include "imgui_addons/imgui_knob.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>

AppThreeDee::AppThreeDee(GLFWwindow *window, Mixer *mixer)
    : _mixer(mixer), _window(window), _display_w(800), _display_h(600),
      showAddSynthEditor(false)
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

std::chrono::milliseconds::rep calculateStepTime(int bpm)
{
    auto beatsPerSecond = static_cast<double>(bpm * 4) / 60.0;

    return static_cast<std::chrono::milliseconds::rep>(1000 / beatsPerSecond);
}

bool AppThreeDee::SetUp()
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigDockingWithShift = false;

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Setup style
    ImGui::StyleColorsDark();

    _mixer->GetBankManager()->RescanForBanks();

    _lastSequencerTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    _playerTimeInMs = 0;
    _currentStep = 0;
    _bpm = 120;
    _stepTimeInMs = calculateStepTime(_bpm);

    return true;
}

static ImVec4 clear_color = ImColor(114, 144, 154);
static std::map<int, TrackPattern> tracksOfPatterns[NUM_MIXER_CHANNELS];
static int activeInstrument = 0;
static int activePattern = -1;

static bool showInstrumentEditor = false;
static bool showPatternEditor = false;
static int keyboardChannel = 0;
static bool showADNoteEditor = true;
static bool isPlaying = false;
static int openSelectInstrument = -1;

int AppThreeDee::CountSongLength()
{
    int maxPattern = 0;
    for (int trackIndex = 0; trackIndex < NUM_MIXER_CHANNELS; trackIndex++)
    {
        auto &pattern = tracksOfPatterns[trackIndex];
        if (pattern.empty())
        {
            continue;
        }
        if (pattern.rbegin()->first >= maxPattern)
        {
            maxPattern = pattern.rbegin()->first;
        }
    }

    return maxPattern + 1;
}

void AppThreeDee::AddPattern(int trackIndex, int patternIndex, char const *label)
{
    static int n = std::rand() % 255;
    float hue = n * 0.05f;
    activeInstrument = trackIndex;
    activePattern = patternIndex;
    tracksOfPatterns[trackIndex].insert(std::make_pair(patternIndex, TrackPattern(label, hue)));
    n = (n + std::rand()) % 255;
}

void AppThreeDee::RemoveActivePattern()
{
    if (activeInstrument < 0 || activePattern < 0)
    {
        return;
    }

    tracksOfPatterns[activeInstrument].erase(activePattern);
    activeInstrument = -1;
    activePattern = -1;
}

void AppThreeDee::MovePatternLeftIfPossible()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    auto currentKey = ap->first;

    if (currentKey == 0)
    {
        return;
    }

    auto currentValue = ap->second;
    auto newKey = currentKey - 1;

    if (tracksOfPatterns[activeInstrument].find(newKey) == tracksOfPatterns[activeInstrument].end())
    {
        tracksOfPatterns[activeInstrument].insert(std::make_pair(newKey, currentValue));
        tracksOfPatterns[activeInstrument].erase(currentKey);
        activePattern = newKey;
    }
}

void AppThreeDee::MovePatternLeftForced()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    if (tracksOfPatterns[activeInstrument].begin()->first == 0)
    {
        return;
    }

    for (int i = tracksOfPatterns[activeInstrument].begin()->first; i <= ap->first; i++)
    {
        auto itr = tracksOfPatterns[activeInstrument].find(i);
        if (itr == tracksOfPatterns[activeInstrument].end())
        {
            continue;
        }
        tracksOfPatterns[activeInstrument].insert(std::make_pair(i - 1, itr->second));
        tracksOfPatterns[activeInstrument].erase(i);
    }

    activePattern = activePattern - 1;
}

void AppThreeDee::SwitchPatternLeft()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    auto currentKey = ap->first;

    if (currentKey <= 0)
    {
        return;
    }

    auto currentValue = ap->second;
    auto newKey = currentKey - 1;

    tracksOfPatterns->erase(currentKey);

    if (tracksOfPatterns[activeInstrument].find(newKey) != tracksOfPatterns[activeInstrument].end())
    {
        auto tmpValue = tracksOfPatterns[activeInstrument].find(newKey)->second;
        tracksOfPatterns[activeInstrument].erase(newKey);
        tracksOfPatterns[activeInstrument].insert(std::make_pair(currentKey, tmpValue));
    }

    tracksOfPatterns[activeInstrument].insert(std::make_pair(newKey, currentValue));

    activePattern = newKey;
}

void AppThreeDee::MovePatternRightIfPossible()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    auto currentKey = ap->first;
    auto currentValue = ap->second;
    auto newKey = currentKey + 1;

    ap++;
    auto nextKey = ap->first;
    if (ap == tracksOfPatterns[activeInstrument].end() || newKey < nextKey)
    {
        tracksOfPatterns[activeInstrument].insert(std::make_pair(newKey, currentValue));
        tracksOfPatterns[activeInstrument].erase(currentKey);
        activePattern = newKey;
    }
}

void AppThreeDee::MovePatternRightForced()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    for (int i = tracksOfPatterns[activeInstrument].rbegin()->first; i >= ap->first; i--)
    {
        auto itr = tracksOfPatterns[activeInstrument].find(i);
        if (itr == tracksOfPatterns[activeInstrument].end())
        {
            continue;
        }
        tracksOfPatterns[activeInstrument].insert(std::make_pair(i + 1, itr->second));
        tracksOfPatterns[activeInstrument].erase(i);
    }

    activePattern = activePattern + 1;
}

void AppThreeDee::SwitchPatternRight()
{
    auto ap = tracksOfPatterns[activeInstrument].find(activePattern);

    if (activeInstrument < 0 || activePattern < 0 || ap == tracksOfPatterns[activeInstrument].end())
    {
        return;
    }

    auto currentKey = ap->first;
    auto currentValue = ap->second;
    auto newKey = currentKey + 1;

    tracksOfPatterns->erase(currentKey);

    if (tracksOfPatterns[activeInstrument].find(newKey) != tracksOfPatterns[activeInstrument].end())
    {
        auto tmpValue = tracksOfPatterns[activeInstrument].find(newKey)->second;
        tracksOfPatterns[activeInstrument].erase(newKey);
        tracksOfPatterns[activeInstrument].insert(std::make_pair(currentKey, tmpValue));
    }

    tracksOfPatterns[activeInstrument].insert(std::make_pair(newKey, currentValue));

    activePattern = newKey;
}

void AppThreeDee::SelectFirstPatternInTrack()
{
    if (activeInstrument < 0)
    {
        return;
    }

    activePattern = tracksOfPatterns[activeInstrument].begin()->first;
}

void AppThreeDee::SelectLastPatternInTrack()
{
    if (activeInstrument < 0)
    {
        return;
    }

    activePattern = tracksOfPatterns[activeInstrument].rbegin()->first;
}

void AppThreeDee::EditSelectedPattern()
{
    showPatternEditor = true;
}

void AppThreeDee::SelectPreviousPattern()
{
    if (activeInstrument < 0)
    {
        return;
    }

    if (activePattern <= 0)
    {
        return;
    }

    int newIndex = activePattern - 1;
    while (newIndex >= 0)
    {
        if (DoesPatternExistAtIndex(activeInstrument, newIndex))
        {
            activePattern = newIndex;
            break;
        }
        newIndex--;
    }
}

void AppThreeDee::SelectNextPattern()
{
    if (activeInstrument < 0)
    {
        return;
    }

    auto lastIndex = LastPatternIndex(activeInstrument);
    if (activePattern == lastIndex)
    {
        return;
    }

    int newIndex = activePattern + 1;
    while (newIndex <= lastIndex)
    {
        if (DoesPatternExistAtIndex(activeInstrument, newIndex))
        {
            activePattern = newIndex;
            break;
        }
        newIndex++;
    }
}

int AppThreeDee::LastPatternIndex(int trackIndex)
{
    return tracksOfPatterns[trackIndex].empty() ? -1 : tracksOfPatterns[trackIndex].rbegin()->first;
}

bool AppThreeDee::DoesPatternExistAtIndex(int trackIndex, int patternIndex)
{
    if (trackIndex < 0 || trackIndex >= NUM_MIXER_CHANNELS)
    {
        return false;
    }

    return tracksOfPatterns[trackIndex].find(patternIndex) != tracksOfPatterns[trackIndex].end();
}

TrackPattern &AppThreeDee::GetPattern(int trackIndex, int patternIndex)
{
    return tracksOfPatterns[trackIndex][patternIndex];
}

int const maxNotes = 255;
static std::chrono::milliseconds::rep activeNotes[NUM_MIXER_CHANNELS][maxNotes] = {{0}};

void AppThreeDee::HitNote(int trackIndex, int note, int durationInMs)
{
    activeNotes[trackIndex][note] = durationInMs;
    _mixer->NoteOn(static_cast<unsigned char>(trackIndex), static_cast<unsigned char>(note), 100);
}

void AppThreeDee::SequencerTick()
{
    std::chrono::milliseconds::rep currentTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    auto deltaTime = currentTime - _lastSequencerTimeInMs;
    _lastSequencerTimeInMs = currentTime;

    for (int trackIndex = 0; trackIndex < NUM_MIXER_CHANNELS; trackIndex++)
    {
        for (int note = 0; note < maxNotes; note++)
        {
            if (activeNotes[trackIndex][note] <= 0)
            {
                continue;
            }

            activeNotes[trackIndex][note] -= deltaTime;
            if (activeNotes[trackIndex][note] <= 0)
            {
                _mixer->NoteOff(static_cast<unsigned char>(trackIndex), static_cast<unsigned char>(note));
            }
        }
    }
    if (!IsPlaying())
    {
        return;
    }

    _playerTimeInMs += deltaTime;

    if (_playerTimeInMs > _stepTimeInMs)
    {
        _currentStep++;
        if (_currentStep >= (CountSongLength() * 16))
        {
            _currentStep = 0;
        }
        Step(_currentStep);
        _playerTimeInMs -= _stepTimeInMs;
    }
}

void AppThreeDee::Step(int step)
{
    auto patternIndex = step / 16;
    auto stepIndex = step % 16;

    for (int trackIndex = 0; trackIndex < NUM_MIXER_CHANNELS; trackIndex++)
    {
        auto track = tracksOfPatterns[trackIndex];
        if (track.find(patternIndex) != track.end())
        {
            auto pattern = track[patternIndex];
            for (auto note : pattern._notes)
            {
                if (note._step == stepIndex)
                {
                    HitNote(trackIndex, note._note, 200);
                }
            }
        }
    }
}

bool AppThreeDee::IsPlaying()
{
    return isPlaying;
}

void AppThreeDee::Stop()
{
    isPlaying = false;
    _playerTimeInMs = 0;
}

void AppThreeDee::PlayPause()
{
    isPlaying = !isPlaying;

    if (isPlaying)
    {
        Step(_currentStep);
    }
}

void AppThreeDee::ImGuiSelectedTrack()
{
    if (activeInstrument < 0 || activeInstrument >= NUM_MIXER_CHANNELS)
    {
        activeInstrument = -1;
        return;
    }

    auto io = ImGui::GetStyle();

    ImGui::Begin("Selected Track");
    {
        auto lh = ImGui::GetItemsLineHeightWithSpacing();
        auto width = ImGui::GetContentRegionAvail().x;
        float const sliderHeight = 200.0f;

        auto sliderPanelHeight =
            sliderHeight                                   /*Fader height*/
            + lh                                           /*Instrument button*/
            + ((width / 2) + lh + (io.ItemSpacing.y * 2)); /*panning knob*/

        auto spaceLeft =
            ImGui::GetContentRegionAvail().y /*Available height*/
            - sliderPanelHeight              /*Volume fader*/
            - lh                             /*Settings*/
            - lh;                            /*Input channel*/

        ImGui::BeginChild("item view", ImVec2(0, -sliderPanelHeight));
        {
            if (ImGui::Button("Settings", ImVec2(width, 0)))
            {
                showAddSynthEditor = true;
            }

            const char *channels[] = {
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
            int channel = static_cast<int>(_mixer->GetChannel(activeInstrument)->Prcvchn);
            ImGui::PushItemWidth(width);
            if (ImGui::Combo("##KeyboardChannel", &channel, channels, NUM_MIXER_CHANNELS))
            {
                _mixer->GetChannel(activeInstrument)->Prcvchn = static_cast<unsigned char>(channel);
            }

            auto fxButtonCount = spaceLeft / ImGui::GetItemsLineHeightWithSpacing() - 1;
            for (int fx = 0; fx < fxButtonCount; fx++)
            {
                char fxButton[32] = {0};
                sprintf(fxButton, "fx %d", fx + 1);
                ImGui::Button(fxButton, ImVec2(width, 0));
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::SameLine(0.0f, width / 4);
        auto panning = _mixer->GetChannel(activeInstrument)->Ppanning;
        if (ImGui::KnobUchar("panning", &panning, 0, 128, ImVec2(width / 2, width / 2)))
        {
            _mixer->GetChannel(activeInstrument)->setPpanning(panning);
        }

        float peakl, peakr;
        _mixer->GetChannel(activeInstrument)->ComputePeakLeftAndRight(_mixer->GetChannel(activeInstrument)->Pvolume, peakl, peakr);

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SameLine(0.0f, (width - 20) / 2);
        int v = static_cast<int>(_mixer->GetChannel(activeInstrument)->Pvolume);
        if (ImGui::VSliderInt("##vol", ImVec2(20, sliderHeight - io.ItemSpacing.y), &v, 0, 128))
        {
            _mixer->GetChannel(activeInstrument)->setPvolume(static_cast<unsigned char>(v));
        }

        auto hue = activeInstrument * 0.05f;
        ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(hue, 0.6f, 0.6f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(hue, 0.7f, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(hue, 0.8f, 0.8f)));

        auto name = std::string(reinterpret_cast<char *>(_mixer->GetChannel(activeInstrument)->Pname));
        if (ImGui::Button(name.size() == 0 ? "default" : name.c_str(), ImVec2(width, 0)))
        {
            openSelectInstrument = activeInstrument;
        }

        ImGui::PopStyleColor(3);
    }
    ImGui::End();

    if (openSelectInstrument >= 0)
    {
        ImGui::OpenPopup("Select Instrument");
    }

    ImGui::SetNextWindowSize(ImVec2(700, 800));
    if (ImGui::BeginPopupModal("Select Instrument"))
    {
        auto count = _mixer->GetBankManager()->GetBankCount();
        std::vector<const char *> bankNames;
        bankNames.push_back("");
        for (int i = 0; i < count; i++)
        {
            bankNames.push_back(_mixer->GetBankManager()->GetBank(i).name.c_str());
        }
        static int currentBank = 0;
        if (ImGui::Combo("Bank", &currentBank, &(bankNames[0]), int(count)))
        {
            _mixer->GetBankManager()->LoadBank(currentBank - 1);
        }

        static bool autoClose = false;
        ImGui::SameLine();
        ImGui::Checkbox("Auto close", &autoClose);

        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            openSelectInstrument = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::BeginChild("banks", ImVec2(0, -20));
        ImGui::Columns(5);
        if (currentBank > 0)
        {
            for (unsigned int i = 0; i < BANK_SIZE; i++)
            {
                auto instrumentName = _mixer->GetBankManager()->GetName(i);

                if (ImGui::Button(instrumentName.c_str(), ImVec2(120, 20)))
                {
                    auto const &instrument = _mixer->GetChannel(activeInstrument);
                    instrument->Lock();
                    _mixer->GetBankManager()->LoadFromSlot(i, instrument);
                    instrument->Unlock();
                    instrument->ApplyParameters();
                    openSelectInstrument = -1;
                    if (autoClose)
                    {
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
        ImGui::EndPopup();
    }
}

void AppThreeDee::ImGuiSequencer()
{
    ImGui::Begin("Sequencer");
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

        static int trackHeight = 30;
        ImGui::BeginChild("scrolling", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (int trackIndex = 0; trackIndex < NUM_MIXER_CHANNELS; trackIndex++)
        {
            ImGui::PushID(trackIndex * 1100);
            auto trackEnabled = _mixer->GetChannel(trackIndex)->Penabled == 1;
            if (ImGui::Checkbox("##trackEnabled", &trackEnabled))
            {
                _mixer->GetChannel(trackIndex)->Penabled = trackEnabled ? 1 : 0;
            }
            ImGui::SameLine();

            float hue = trackIndex * 0.05f;
            char trackLabel[32] = {'\0'};
            sprintf(trackLabel, "%02d", trackIndex + 1);
            bool highLight = trackIndex == activeInstrument;
            if (highLight)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(hue, 0.6f, 0.6f)));
            }
            if (ImGui::Button(trackLabel, ImVec2(trackHeight, trackHeight)))
            {
                activeInstrument = trackIndex;
            }
            if (highLight)
            {
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(hue, 0.6f, 0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(hue, 0.7f, 0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(hue, 0.8f, 0.8f)));

            //if (_mixer->GetChannel(trackIndex)->Pkitmode != 0)
            {
                ImGuiStepSequencer(trackIndex, trackHeight);
            }
            //else
            {
                ImGuiPianoRollSequencer(trackIndex, trackHeight);
            }

            ImGui::PopStyleColor(3);
        }
        float scroll_y = ImGui::GetScrollY();
        ImGui::EndChild();

        ImGui::PopStyleVar(2);

        float scroll_x_delta = 0.0f;
        ImGui::SmallButton("<<");
        if (ImGui::IsItemActive()) scroll_x_delta = -ImGui::GetIO().DeltaTime * 1000.0f;
        ImGui::SameLine();
        ImGui::Text("Scroll from code");
        ImGui::SameLine();
        ImGui::SmallButton(">>");
        if (ImGui::IsItemActive()) scroll_x_delta = +ImGui::GetIO().DeltaTime * 1000.0f;
        if (scroll_x_delta != 0.0f)
        {
            ImGui::BeginChild("scrolling");
            ImGui::SetScrollX(ImGui::GetScrollX() + scroll_x_delta);
            ImGui::End();
        }

        ImGui::BeginChild("info");
        ImGui::SetScrollY(scroll_y);
        ImGui::EndChild();

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            //if (activeInstrument >= 0 && _mixer->GetChannel(activeInstrument)->Pkitmode != 0)
            {
                ImGuiStepSequencerEventHandling();
            }
            //else
            {
                ImGuiPianoRollSequencerEventHandling();
            }
        }
    }
    ImGui::End();
}

void AppThreeDee::ImGuiStepSequencer(int trackIndex, float trackHeight)
{
    auto lastIndex = LastPatternIndex(trackIndex);
    for (int patternIndex = 0; patternIndex <= lastIndex; patternIndex++)
    {
        ImGui::SameLine();
        ImGui::PushID(patternIndex + trackIndex * 1000);
        bool isActive = trackIndex == activeInstrument && patternIndex == activePattern;
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        }

        if (DoesPatternExistAtIndex(trackIndex, patternIndex))
        {
            auto &pattern = GetPattern(trackIndex, patternIndex);
            if (ImGui::Button(pattern._name.c_str(), ImVec2(120.0f, trackHeight)))
            {
                activeInstrument = trackIndex;
                activePattern = patternIndex;
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    EditSelectedPattern();
                }
            }
        }
        else if (_mixer->GetChannel(trackIndex)->Penabled)
        {
            if (ImGui::Button("+", ImVec2(120.0f, trackHeight)))
            {
                AddPattern(trackIndex, patternIndex, "");
            }
        }
        if (isActive)
        {
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
        }

        ImGui::PopID();
    }
    if (_mixer->GetChannel(trackIndex)->Penabled)
    {
        ImGui::SameLine();
        ImGui::PushID((100 + trackIndex) * 2010);
        if (ImGui::Button("+", ImVec2(120.0f, trackHeight)))
        {
            AddPattern(trackIndex, lastIndex + 1, "");
        }
        ImGui::PopID();
    }
}

void AppThreeDee::ImGuiStepSequencerEventHandling()
{
    ImGuiIO &io = ImGui::GetIO();

    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete)))
    {
        RemoveActivePattern();
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
    {
        if (io.KeyShift && !io.KeyCtrl)
        {
            MovePatternLeftForced();
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            SwitchPatternLeft();
        }
        else
        {
            MovePatternLeftIfPossible();
        }
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
    {
        if (io.KeyShift && !io.KeyCtrl)
        {
            MovePatternRightForced();
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            SwitchPatternRight();
        }
        else
        {
            MovePatternRightIfPossible();
        }
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Home)))
    {
        SelectFirstPatternInTrack();
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_End)))
    {
        SelectLastPatternInTrack();
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Enter)))
    {
        EditSelectedPattern();
    }
    if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Tab)))
    {
        if (io.KeyShift)
        {
            SelectPreviousPattern();
        }
        else
        {
            SelectNextPattern();
        }
    }
}

void AppThreeDee::ImGuiPianoRollSequencer(int trackIndex, float trackHeight)
{
}

void AppThreeDee::ImGuiPianoRollSequencerEventHandling()
{
}

static const char *notes[] = {
    "A",
    "A#",
    "B",
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
};

void AppThreeDee::ImGuiPatternEditorWindow()
{
    const float noteLabelWidth = 50.0f;
    const float rowHeight = 20.0f;
    if (showPatternEditor)
    {
        if (!DoesPatternExistAtIndex(activeInstrument, activePattern))
        {
            activePattern = -1;
            return;
        }

        auto &style = ImGui::GetStyle();
        auto &selectedPattern = GetPattern(activeInstrument, activePattern);

        ImGui::Begin("Pattern editor", &showPatternEditor);
        auto width = ImGui::GetWindowWidth() - noteLabelWidth - (style.ItemSpacing.x * 2) - style.ScrollbarSize;
        auto itemWidth = (width / 16) - (style.ItemSpacing.x);
        for (int i = 0; i < 88; i++)
        {
            if (i % 12 == 0)
            {
                ImGui::Separator();
            }
            ImGui::PushID(i);
            if (ImGui::Button(notes[i % 12], ImVec2(noteLabelWidth, rowHeight)))
            {
                HitNote(activeInstrument, i, 200);
            }
            for (int j = 0; j < 16; j++)
            {
                ImGui::SameLine();
                ImGui::PushID(j);
                auto found = selectedPattern._notes.find(TrackPatternNote(i, j));
                bool s = found != selectedPattern._notes.end();
                if (j % 4 == 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_FrameBg]);
                }
                if (ImGui::CheckButton("##note", &s, ImVec2(itemWidth, rowHeight)))
                {
                    if (!s)
                    {
                        selectedPattern._notes.erase(TrackPatternNote(i, j));
                    }
                    else
                    {
                        selectedPattern._notes.insert(TrackPatternNote(i, j));
                    }
                    HitNote(activeInstrument, i, 200);
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::End();
    }
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
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
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

    ImGuiSequencer();
    ImGuiPatternEditorWindow();
    ImGuiSelectedTrack();

    // 3. Show another simple window.
    if (showADNoteEditor)
    {
        ImGui::Begin("AD note editor", &showADNoteEditor);
        if (ImGui::Button("Close Me"))
        {
            showADNoteEditor = false;
        }

        if (ImGui::BeginTabBar("blah"))
        {
            if (ImGui::BeginTabItem("Global"))
            {
                if (activeInstrument >= 0)
                {
                    ADNoteEditor(_mixer->GetChannel(activeInstrument)->_instruments[0].adpars);
                }
                ImGui::EndTabItem();
            }
            static char voiceIds[][64]{
                "Voice 1",
                "Voice 2",
                "Voice 3",
                "Voice 4",
                "Voice 5",
                "Voice 6",
                "Voice 7",
                "Voice 8",
            };
            for (int i = 0; i < NUM_VOICES; i++)
            {
                if (activeInstrument >= 0)
                {
                    auto parameters = &_mixer->GetChannel(activeInstrument)->_instruments[0].adpars->VoicePar[i];
                    if (ImGui::BeginTabItem(voiceIds[i]))
                    {
                        ADNoteVoiceEditor(parameters);

                        ImGui::EndTabItem();
                    }
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    ImGuiPlayback();

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

static std::map<int, unsigned char> mappedNotes{
    {int('Z'), static_cast<unsigned char>(65)},
    {int('X'), static_cast<unsigned char>(66)},
    {int('C'), static_cast<unsigned char>(67)},
    {int('V'), static_cast<unsigned char>(68)},
    {int('B'), static_cast<unsigned char>(69)},
    {int('N'), static_cast<unsigned char>(70)},
    {int('M'), static_cast<unsigned char>(71)},
};

void AppThreeDee::onKeyAction(int key, int /*scancode*/, int action, int /*mods*/)
{
    auto found = mappedNotes.find(key);

    if (found != mappedNotes.end())
    {
        if (action == 1)
        {
            _mixer->NoteOn(static_cast<unsigned char>(keyboardChannel), found->second, 128);
        }
        else if (action == 0)
        {
            _mixer->NoteOff(static_cast<unsigned char>(keyboardChannel), found->second);
        }
    }
}

void AppThreeDee::ImGuiPlayback()
{
    ImGui::Begin("Playback");
    {
        if (ImGui::Button("Stop"))
        {
            Stop();
        }

        ImGui::SameLine();

        if (isPlaying)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
        }

        if (ImGui::Button("Play/Pause"))
        {
            PlayPause();
        }

        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        ImGui::PushItemWidth(200);
        if (ImGui::SliderInt("##BPM", &_bpm, 10, 200, "BPM %d"))
        {
            _stepTimeInMs = calculateStepTime(_bpm);
        }

        ImGui::SameLine();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();
    }
}

void AppThreeDee::CleanUp()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

#include <application.h>
#include <imgui.h>
#include <iostream>
#include <zyn.common/Config.h>
#include <zyn.common/IAudioGenerator.h>
#include <zyn.common/IMidiEventHandler.h>
#include <zyn.common/globals.h>
#include <zyn.mixer/Mixer.h>
#include <zyn.nio/MidiInputManager.h>
#include <zyn.nio/Nio.h>

ImVec2 operator+(ImVec2 const &a, ImVec2 const &b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(ImVec2 const &a, ImVec2 const &b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}
class Application :
    public IApplication
{
    Mixer *_mixer;

    ImFont *_monofont;

public:
    virtual bool Setup()
    {
        ImGuiIO &io = ImGui::GetIO();
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
        _monofont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\SourceCodePro-Bold.ttf", 12.0f);
        io.Fonts->Build();

        Config::init();

        SystemSettings::Instance().samplerate = Config::Current().cfg.SampleRate;
        SystemSettings::Instance().buffersize = Config::Current().cfg.SoundBufferSize;
        SystemSettings::Instance().oscilsize = Config::Current().cfg.OscilSize;
        SystemSettings::Instance().alias();

        _mixer = new Mixer();

        _mixer->Init();

        Nio::preferedSampleRate(SystemSettings::Instance().samplerate);

        if (!Nio::Start(_mixer, _mixer))
        {
            return false;
        }

        Nio::SelectSink("PA");
        Nio::SelectSource("RT");

        return true;
    }

    virtual void Render3d()
    {}

    const int numTracks = 8;
    const int numRows = 64;
    int currentRow = 0;

    virtual void Render2d()
    {
        //show Main Window
        ImGui::ShowDemoWindow();

        ImGui::Begin("TracksContainer", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        auto tracksPos = ImGui::GetWindowContentRegionMin();
        auto tracksMax = ImGui::GetWindowContentRegionMax();

        ImGui::PushFont(_monofont);

        auto lineHeight = ImGui::GetTextLineHeightWithSpacing();
        auto selectionRowMin = ImVec2(tracksPos.x, tracksPos.y - 2 + currentRow * lineHeight);
        auto selectionRowMax = ImVec2(tracksMax.x, tracksPos.y + (currentRow + 1) * lineHeight);

        auto drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            ImGui::GetWindowPos() + selectionRowMin,
            ImGui::GetWindowPos() + selectionRowMax,
            ImColor(20, 220, 20, 55));

        ImGui::SetScrollY((currentRow - ((tracksMax.y - tracksPos.y) / lineHeight) / 2 + 1) * lineHeight);

        auto columnWidth = ImGui::CalcTextSize("--- -- --");
        ImGui::Columns(numTracks + 1);
        ImGui::SetColumnWidth(0, 50);
        for (int i = 0; i < numTracks; i++)
        {
            ImGui::SetColumnWidth(i + 1, columnWidth.x + 15);
        }

        for (int r = 0; r < 64; r++)
        {
            ImGui::Text("%02d", r);
            ImGui::NextColumn();

            for (int i = 0; i < numTracks; i++)
            {
                ImGui::Text("--- -- --");
                ImGui::NextColumn();
            }
        }

        ImGui::PopFont();
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
        {
            currentRow--;
            if (currentRow < 0) currentRow = numRows - 1;
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
        {
            currentRow++;
            if (currentRow >= numRows) currentRow = 0;
        }

        ImGui::End();
    }

    virtual void Cleanup()
    {
        //ensure that everything has stopped with the mutex wait
        _mixer->Lock();
        _mixer->Unlock();

        Nio::Stop();

        delete _mixer;
        _mixer = nullptr;
    }
};

extern IApplication *CreateApplication();

IApplication *CreateApplication()
{
    return new Application();
}

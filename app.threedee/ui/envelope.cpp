#include "../app.threedee.h"

#include "../imgui_addons/imgui_knob.h"
#include <imgui.h>
#include <zyn.synth/EnvelopeParams.h>

void AppThreeDee::Envelope(char const *label, EnvelopeParams *envelope)
{
    ImGui::Text("%s", label);

    if (envelope->Envmode >= 3)
    {
        if (ImGui::KnobUchar("A.val", &(envelope->PA_val), 0, 127, ImVec2(40, 40)))
        {
        }
        ImGui::ShowTooltipOnHover("Starting value");

        ImGui::SameLine();
    }

    if (ImGui::KnobUchar("A.dt", &(envelope->PA_dt), 0, 127, ImVec2(40, 40)))
    {
    }
    ImGui::ShowTooltipOnHover("Attack time");

    ImGui::SameLine();

    if (envelope->Envmode == 4)
    {
        if (ImGui::KnobUchar("D.val", &(envelope->PD_val), 0, 127, ImVec2(40, 40)))
        {
        }
        ImGui::ShowTooltipOnHover("Decay value");

        ImGui::SameLine();
    }

    if (envelope->Envmode != 3 && envelope->Envmode != 5)
    {
        if (ImGui::KnobUchar("D.dt", &(envelope->PD_dt), 0, 127, ImVec2(40, 40)))
        {
        }
        ImGui::ShowTooltipOnHover("Decay time");

        ImGui::SameLine();
    }

    if (envelope->Envmode < 3)
    {
        if (ImGui::KnobUchar("S.val", &(envelope->PS_val), 0, 127, ImVec2(40, 40)))
        {
        }
        ImGui::ShowTooltipOnHover("Sustain value");

        ImGui::SameLine();
    }

    if (ImGui::KnobUchar("R.dt", &(envelope->PR_dt), 0, 127, ImVec2(40, 40)))
    {
    }
    ImGui::ShowTooltipOnHover("Release time");

    ImGui::SameLine();

    if (envelope->Envmode >= 3)
    {
        if (ImGui::KnobUchar("R.val", &(envelope->PR_val), 0, 127, ImVec2(40, 40)))
        {
        }
        ImGui::ShowTooltipOnHover("Release value");

        ImGui::SameLine();
    }

    if (ImGui::KnobUchar("Str.", &(envelope->Penvstretch), 0, 127, ImVec2(40, 40)))
    {
    }
    ImGui::ShowTooltipOnHover("Envelope stretch (on lower notes makes the envelope longer)");

    ImGui::SameLine();

    auto forcedRelease = envelope->Pforcedrelease != 0;
    if (ImGui::Checkbox("##forcedrelease", &forcedRelease))
    {
        envelope->Pforcedrelease = forcedRelease ? 1 : 0;
    }
    ImGui::ShowTooltipOnHover("Forced Release");

    ImGui::SameLine();

    ImGui::Text("frcR.");
}

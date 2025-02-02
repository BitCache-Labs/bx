#include <editor/profiler.hpp>

BX_EDITOR_MENUITEM_REGISTRATION("Window/Profiler", ProfilerEditor)
{
    Editor::Get().AddWindow<ProfilerEditor>();
}

ProfilerEditor::ProfilerEditor()
{
    SetTitle("Profiler");
    SetExclusive(true);
    SetPresistent(false);
}

void ProfilerEditor::OnGui(EditorApplication& app)
{
}

/*
#include <engine/profiler.hpp>
#include <engine/time.hpp>
#include <engine/list.hpp>
#include <engine/math.hpp>
#include <engine/data.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <IconsFontAwesome5.h>

#include <algorithm>

//#include <rttr/registration.h>
//RTTR_REGISTRATION
//{
//    rttr::registration::class_<ProfilerEditor>("ProfilerEditor")
//    .constructor();
//}

bool ProfilerEditor::Initialize()
{
    //m_shown = Data::GetBool("Toolbar Show Profiler", false, DataTarget::EDITOR);
    return true;
}

void ProfilerEditor::Shutdown()
{
    //Data::SetBool("Toolbar Show Profiler", isShown, DataTarget::EDITOR);
}

void ProfilerEditor::Reload()
{
}

//void ProfilerEditor::OnToolbar()
//{
//    ImGui::SameLine();
//    if (ImGui::Button(ICON_FA_CHART_PIE))
//    {
//        isShown = !isShown;
//    }
//    //Tooltip("Profiler");
//}

const char* ProfilerEditor::GetTitle() const
{
    return ICON_FA_STOPWATCH"  Profiler";
}

void ProfilerEditor::Present(const char* title, bool& isOpen)
{
    //Old icon: ICON_FA_CHART_PIE
    ImGui::Begin(title, &isOpen, ImGuiWindowFlags_NoCollapse);
    
    // TODO: Fix moving frame number
    //m_frame++;
    m_frames++;
    m_time += Time::GetDeltaTime();
    if (m_time >= 1.f)
    {
        m_fps = (int)(ceil(m_frames / m_time));
        m_time = std::fmod(m_time, 1.f);
        m_frames = 0;
    }
    ImGui::Text("FPS: %i", m_fps);

    const auto& data = Profiler::GetData();
    float height = (float)Math::Max(200, (int)(20 * data.size()));
    if (ImPlot::BeginPlot("Profiler", ImVec2(-1, height)))
    {
        ImPlot::SetupAxes("Samples", "Milliseconds");
        ImPlot::SetupAxesLimits(m_frame, m_frame + 100, 0, 50, ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside);
        for (const auto& itr : data)
        {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(itr.first.c_str(), itr.second.frames.data(), (int)itr.second.frames.size());
            ImPlot::PopStyleVar();
            ImPlot::PlotLine(itr.first.c_str(), itr.second.frames.data(), (int)itr.second.frames.size());
        }
        ImPlot::EndPlot();
    }
    
    for (auto& itr : data)
        ImGui::LabelText(itr.first.c_str(), "%f ms", itr.second.avg);

    ImGui::End();
}
*/
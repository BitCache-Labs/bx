#include "bx/editor/views/data_view.hpp"

#include <bx/engine/core/data.hpp>
#include <bx/engine/core/file.hpp>
#include <bx/engine/containers/list.hpp>
#include <bx/engine/containers/hash_map.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>
#include <algorithm>

//std::vector<DataMap> history;
//int historyStackPointer;

HashMap<DataTarget, bool> g_edited;

static inline float saturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline uint32_t f32_to_uint8(float val) { return (uint32_t)(saturate(val) * 255.0f + 0.5f); }

static uint32_t EncodeColor(ImVec4 color)
{
	ImU32 out;
	out = (f32_to_uint8(color.x)) << 24;
	out |= (f32_to_uint8(color.y)) << 16;
	out |= (f32_to_uint8(color.z)) << 8;
	out |= (f32_to_uint8(color.w)) << 0;
	return out;
}

static ImVec4 DecodeColor(uint32_t color)
{
	float s = 1.0f / 255.0f;
	return ImVec4(
		((color >> 24) & 0xFF) * s,
		((color >> 16) & 0xFF) * s,
		((color >> 8) & 0xFF) * s,
		((color >> 0) & 0xFF) * s);
}

namespace Widget
{
	bool Edit(const char* label, bool& val)
	{
		bool edited = ImGui::Checkbox(label, &val);
		return edited;
	}

	bool Edit(const char* label, int& val)
	{
		bool edited = ImGui::DragInt(label, &val);
		return edited;
		return false;
	}

	bool Edit(const char* label, unsigned& val)
	{
		ImVec4 vec = DecodeColor(val);
		bool edited = ImGui::ColorEdit4(label, &vec.x);
		val = EncodeColor(vec);
		return edited;
	}

	bool Edit(const char* label, float& val)
	{
		bool edited = ImGui::DragFloat(label, &val, 0.01f);
		return edited;
	}

	bool Edit(const char* label, double& val)
	{
		float flt = (float)val;
		bool edited = ImGui::DragFloat(label, &flt, 0.01f);
		val = flt;
		return edited;
	}

	struct InputTextCallback_UserData
	{
		String* Str;
		ImGuiInputTextCallback  ChainCallback;
		void* ChainCallbackUserData;
	};

	bool Edit(const char* label, String& val)
	{
		ImGui::PushItemWidth(0);
		bool changed = ImGui::InputText(label, &val);
		ImGui::PopItemWidth();
		return changed;
	}
}

static bool HasChages()
{
	return g_edited[DataTarget::DEBUG] || g_edited[DataTarget::GAME] || g_edited[DataTarget::SYSTEM];
}

static void Undo()
{
	//if (historyStackPointer < history.size() - 1)
	//	historyStackPointer++;
	//
	//size_t idx = history.size() - 1 - historyStackPointer;
	//if (idx < history.size() && idx >= 0) {
	//	const DataMap& state = history[idx];
	//	Data::SetDataState(state);
	//}
}

static void Redo()
{
	//if (historyStackPointer > 0)
	//	historyStackPointer--;
	//
	//std::size_t idx = history.size() - 1 - historyStackPointer;
	//if (idx < history.size() && idx >= 0) {
	//	const DataMap& state = history[idx];
	//	Data::SetDataState(state);
	//}
}

static void Tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.6f)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip("%s", tooltip);
		ImGui::EndTooltip();
	}
}

template <typename T>
static bool InspectStorage(HashMap<String, T>& storage)
{
	bool changed = false;
	for (auto& data : storage)
	{
		if (Widget::Edit(data.first.c_str(), data.second))
		{
			// TODO: Notify change
			changed = true;
		}
	}
	return changed;
}

static bool InspectDatabase(DataTarget target)
{
	bool changed = false;

	auto& database = Data::GetDatabase(target);
	changed |= InspectStorage(database.GetStorage<bool>());
	changed |= InspectStorage(database.GetStorage<int>());
	changed |= InspectStorage(database.GetStorage<unsigned>());
	changed |= InspectStorage(database.GetStorage<float>());
	changed |= InspectStorage(database.GetStorage<double>());
	changed |= InspectStorage(database.GetStorage<String>());
	
	return changed;
}

static void Save(DataTarget target)
{
	Data::Save(target);
	g_edited[target] = false;
}

static void InspectTarget(const String& type_name, ImGuiTextFilter& filter, DataTarget target)
{
	if (g_edited[target])
		ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFF5FB9);

	if (ImGui::CollapsingHeader(type_name.c_str()))
	{
		if (g_edited[target])
			ImGui::PopStyleColor();

		bool& ed = g_edited[target];
		{
			bool ted = ed;
			if (ted) ImGui::PushStyleColor(ImGuiCol_Button, 0xFF773049);
			if (ImGui::Button("Save")) Save(target);
			if (ted) ImGui::PopStyleColor();
			Tooltip("Save to a file");
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text("%s", File::GetPath(Data::GetFilepath(target)).c_str());
			ImGui::PopStyleColor();
		}

		// ImGui::BeginChild("Child", ImVec2(0, -100)); // TODO:Find out how to get the size
		ImGui::BeginChild("Child");

		//std::vector<DataRecord> sorted;
		//const auto& records = Data::GetRecords();
		//for (auto& record : records)
		//{
		//	if (filter.PassFilter(record.name.c_str()) && record.target == target)
		//		sorted.push_back(record);
		//}
		//std::sort(sorted.begin(), sorted.end());

		//for (const auto& record : sorted)
		//	ed |= InspectEntry(record);

		ed |= InspectDatabase(target);

		ImGui::EndChild();
	}
	else
	{
		if (g_edited[target])
			ImGui::PopStyleColor();
	}
}

void DataView::Present(bool& show)
{
	//if (history.empty())
	//{
	//	DataMap state = Data::GetDataState();
	//	history.push_back(state);
	//}

	ImGui::Begin(ICON_FA_DATABASE"  Data", &show, ImGuiWindowFlags_NoCollapse);

	ImGui::BeginDisabled();// !(historyStackPointer < history.size() - 1));
	if (ImGui::Button(ICON_FA_UNDO))
	{
		Undo();
	}
	Tooltip("Undo");
	ImGui::EndDisabled();
	ImGui::SameLine();
	
	ImGui::BeginDisabled();// historyStackPointer == 0);
	if (ImGui::Button(ICON_FA_REDO))
	{
		Redo();
	}
	Tooltip("Redo");
	ImGui::EndDisabled();
	ImGui::SameLine();

	static ImGuiTextFilter filter;
	filter.Draw(ICON_FA_SEARCH);
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_TIMES))
	{
		filter.Clear();
	}

	//ImGui::BeginChild("Child");
	ImGui::PushItemWidth(80);
	InspectTarget(ICON_FA_COG"  System", filter, DataTarget::SYSTEM);
	InspectTarget(ICON_FA_GAMEPAD"  Game", filter, DataTarget::GAME);
	InspectTarget(ICON_FA_CODE"  Editor", filter, DataTarget::EDITOR);
	InspectTarget(ICON_FA_USER"  Player", filter, DataTarget::PLAYER);
	InspectTarget(ICON_FA_BUG"  Debug", filter, DataTarget::DEBUG);
	ImGui::PopItemWidth();
	//ImGui::EndChild();

	ImGui::End();
}
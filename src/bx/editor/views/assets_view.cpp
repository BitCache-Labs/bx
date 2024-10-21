#include <bx/editor/views/assets_view.hpp>

#include <bx/core/thread.hpp>
#include <bx/core/file.hpp>
#include <bx/core/macros.hpp>
#include <bx/core/time.hpp>
#include <bx/containers/hash_map.hpp>
#include <bx/containers/list.hpp>
#include <bx/platform/graphics.hpp>
#include <bx/engine/data.hpp>
#include <bx/engine/resource.hpp>

#include <cstring>
#include <fstream>
#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<AssetEditor>("AssetEditor")
	.property("m_assetEditor", &AssetEditor::m_assetEditor);
	
	rttr::registration::class_<AssetsView>("AssetsView")
	.constructor();
}

bool AssetsView::Initialize()
{
	ValidateSelectedFolder();
	return true;
}

void AssetsView::Shutdown()
{
}

void AssetsView::Reload()
{
}

void AssetsView::ValidateSelectedFolder()
{
	if (m_selectedFolder == INVALID_TREENODE_ID ||
		!AssetsManager::Get().GetAssetTree().IsValid(m_selectedFolder))
	{
		m_selectedFolder = AssetsManager::Get().GetAssetRootId();
	}
}

void AssetsView::ShowFileTree(TreeNodeId nodeId)
{
	const auto& node = AssetsManager::Get().GetAssetTree().GetNode(nodeId);

	if (!node.data.isDirectory)
		return;

	bool isParent = false;
	for (auto childIndex : node.children)
	{
		const auto& child = AssetsManager::Get().GetAssetTree().GetNode(childIndex);
		if (child.data.isDirectory)
		{
			isParent = true;
			break;
		}
	}

	bool isSelected = m_selectedFolder == nodeId;

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth
		| (!isParent ? ImGuiTreeNodeFlags_Leaf : 0)
		| (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

	ImGui::PushID(node.data.path.c_str());
	if (ImGui::TreeNodeEx(node.data.name.c_str(), flags))
	{
		if (ImGui::IsItemClicked())
		{
			m_selectedFolder = nodeId;
		}

		for (auto childIndex : node.children)
		{
			ShowFileTree(childIndex);
		}

		ImGui::TreePop();
	}
	ImGui::PopID();
}

static List<SizeType> GetSortedNodes(const Tree<Asset>& assetTree, const List<TreeNodeId>& nodes)
{
	List<SizeType> indices(nodes.size());
	for (SizeType i = 0; i < nodes.size(); ++i)
	{
		indices[i] = i;
	}

	std::sort(indices.begin(), indices.end(),
		[&](SizeType a, SizeType b)
		{
			const auto& na = assetTree.GetNode(nodes[a]);
			const auto& nb = assetTree.GetNode(nodes[b]);
			return na.data.extension < nb.data.extension;
		});

	return indices;
}

void AssetsView::ProcessFolderImport(TreeNodeId id, const Asset& asset)
{
	const auto& node = AssetsManager::Get().GetAssetTree().GetNode(id);
	for (auto childId : node.children)
	{
		const auto& child = AssetsManager::Get().GetAssetTree().GetNode(childId);
		const auto& childAsset = child.data;

		AssetsManager::Get().Import(childAsset.extension.c_str(), childAsset.path.c_str());

		//bool isModel =
		//	childAsset.extension == "glb" ||
		//	childAsset.extension == "fbx" ||
		//	childAsset.extension == "obj";
		//
		//if (isModel)
		//{
		//	AssetImporter::ImportModel(childAsset.path);
		//}
		//
		//bool isTexture = childAsset.extension == "png";
		//if (isTexture)
		//{
		//	AssetImporter::ImportTexture(childAsset.path);
		//}
	}
}

void AssetsView::ProcessAssetContextMenu(TreeNodeId id, const Asset& asset)
{
	if (asset.isDirectory)
	{
		if (ImGui::MenuItem("Import"))
		{
			ProcessFolderImport(id, asset);
		}
	}
	else
	{
		AssetsManager::Get().Import(asset.extension.c_str(), asset.path.c_str());

		//bool isModel =
		//	asset.extension == "glb" ||
		//	asset.extension == "fbx" ||
		//	asset.extension == "obj";
		//
		//if (isModel && ImGui::MenuItem("Import"))
		//{
		//	AssetImporter::ImportModel(asset.path);
		//}
		//
		//bool isTexture = asset.extension == "png";
		//if (isTexture && ImGui::MenuItem("Import"))
		//{
		//	AssetImporter::ImportTexture(asset.path);
		//}
	}
}

static const char* GetAssetIcon(bool isDirectory, const String& extension)
{
	if (isDirectory)
	{
		return ICON_FA_FOLDER;
	}
	else
	{
		static Hash<String> hashFn;
		struct HashToIcon
		{
			SizeType hash = 0;
			const char* icon = nullptr;
		};
		static HashToIcon s_icons[] =
		{
			{ hashFn("mesh"),		ICON_FA_SHAPES },
			{ hashFn("material"),	ICON_FA_BOWLING_BALL },
			{ hashFn("skeleton"),	ICON_FA_SKULL_CROSSBONES },
			{ hashFn("animation"),	ICON_FA_RUNNING },
			{ hashFn("scene"),		ICON_FA_LIST },
			{ hashFn("gameobject"),	ICON_FA_CUBES },
			{ hashFn("wren"),		ICON_FA_CODE },
			{ hashFn("texture"),	ICON_FA_IMAGE },
			{ hashFn("shader"),		ICON_FA_YIN_YANG },
			{ hashFn("audio"),		ICON_FA_MUSIC },
			{ hashFn("terrain"),	ICON_FA_MOUNTAIN },
		};
		static SizeType s_iconsCount = BX_ARRAYSIZE(s_icons);

		SizeType hash = hashFn(extension);
		for (SizeType i = 0; i < s_iconsCount; ++i)
		{
			if (hash == s_icons[i].hash)
				return s_icons[i].icon;
		}
		return ICON_FA_FILE;
	}
}

void AssetsView::ShowAssetIcon(f32 iconScale, f32 cellSize, TreeNodeId id, const Asset& asset)
{
	ObjectRef selected = m_selection.GetSelected();

	bool isSelected = false;
	if (selected.Is<TreeNodeId>())
	{
		auto selectedNodeId = *selected.As<TreeNodeId>();
		if (AssetsManager::Get().GetAssetTree().IsValid(selectedNodeId))
		{
			const auto& selectedNode = AssetsManager::Get().GetAssetTree().GetNode(selectedNodeId);
			isSelected = asset.path == selectedNode.data.path;
		}
		else
		{
			m_selection.ClearSelection();
		}
	}

	if (isSelected)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.00f)); // Light blue color

	ImGui::SetWindowFontScale(iconScale);
	if (ImGui::Button(GetAssetIcon(asset.isDirectory, asset.extension), ImVec2(cellSize, cellSize)))
	{
		if (!asset.isDirectory)
			m_selection.SetSelected(Object<TreeNodeId>(id));
		else
			m_selectedFolder = id;
	}
	ImGui::SetWindowFontScale(1.0f);

	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		AssetsManager::Get().OpenEditor(0);

		//bool consumed = false;
		//for (const auto& callback : g_callbacks)
		//{
		//	consumed |= callback.OnAssetDoubleClick(asset, consumed);

			//if (asset.extension == "scene")
			//{
			//	const String& scene = Data::SetString("Current Scene", asset.path, DataTarget::EDITOR);
			//	Scene::Load(scene);
			//}
		//}
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("AssetContextMenu");
	}

	if (ImGui::BeginPopup("AssetContextMenu"))
	{
		ProcessAssetContextMenu(id, asset);

		if (ImGui::MenuItem("Delete"))
		{
			File::Delete(asset.path);
		}
		ImGui::EndPopup();
	}

	ImGuiID fileSourceId = ImGui::GetCurrentWindow()->GetID("file");
	if (!asset.isDirectory)
	{
		//ImGui::PushID(fileSourceId);
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload(asset.extension.c_str(), asset.path.c_str(), asset.path.size() + 1, ImGuiCond_Once);
			ImGui::Text(asset.name.c_str());
			ImGui::EndDragDropSource();
		}
		//ImGui::PopID();
	}
	else
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(0, ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				if (payload->IsDelivery() && payload->SourceId == fileSourceId)
				{
					//payload->SourceId
					auto path = (const char*)payload->Data;
					String newPath = asset.path.c_str();
					//File::Move(path, newPath);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	if (isSelected)
		ImGui::PopStyleColor();
}

void AssetsView::ShowAssetLabel(TreeNodeId id, const Asset& asset)
{
	if (m_fileEditNode == id)
	{
		if (!m_fileEditFocused)
		{
			m_fileEditFocused = true;
			ImGui::SetKeyboardFocusHere();
		}

		// Show input text box for renaming
		if (ImGui::InputText("##EditFilePath", &m_fileEditName, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)
			|| ImGui::IsItemDeactivated())
		{
			// Apply changes
			String newPath = asset.path.substr(0, asset.path.find_last_of("\\/")) + "\\" + m_fileEditName;
			if (File::Move(asset.path, newPath))
			{
				// TODO: Handle failed rename?
			}

			m_fileEditNode = INVALID_TREENODE_ID;
			m_fileEditFocused = false;
			m_fileEditName.clear();
		}
	}
	else
	{
		// Show file name text
		ImGui::TextWrapped(asset.name.c_str());

		// Show tooltip of full path when hovering with delay
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		{
			ImGui::BeginTooltip();
			ImGui::Text(asset.path.c_str());
			ImGui::EndTooltip();
		}

		// Check for double click and enter edit mode
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			m_fileEditNode = id;
			m_fileEditName = asset.name;
		}
	}
}

void AssetsView::ShowGridList()
{
	if (m_selectedFolder == INVALID_TREENODE_ID)
		return;

	const float cellPadding = 10.0f;
	const float cellSize = 100.0f;
	const int columns = (int)((ImGui::GetContentRegionAvail().x + cellPadding) / (cellSize + cellPadding));
	const float iconScale = 5.0f;

	TreeNodeId deleteFile = INVALID_TREENODE_ID;

	int id = 0;
	if (columns > 0)
	{
		ImGui::Columns(columns, NULL, false);

		const auto& node = AssetsManager::Get().GetAssetTree().GetNode(m_selectedFolder);
		auto indices = GetSortedNodes(AssetsManager::Get().GetAssetTree(), node.children);

		for (auto index : indices)
		{
			auto childId = node.children[index];
			const auto& child = AssetsManager::Get().GetAssetTree().GetNode(childId);

			if (!m_filter.PassFilter(child.data.name.c_str()))
				continue;

			ImGui::PushID(child.data.path.c_str());

			ImGui::BeginGroup();

			ShowAssetIcon(iconScale, cellSize, childId, child.data);
			ShowAssetLabel(childId, child.data);
			
			ImGui::EndGroup();
			ImGui::NextColumn();

			ImGui::PopID();
		}
		ImGui::Columns(1); // Reset to single column
	}
}

static void ShowCreatePopup(bool& show)
{
	auto& io = ImGui::GetIO();
	ImVec2 size = io.DisplaySize; //ImGui::GetMainViewport()->Size;
	ImVec2 center = ImVec2(size.x * 0.5f, size.y * 0.5f); //ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(size.x * 0.25f, size.y * 0.25f));
	ImGui::SetNextWindowBgAlpha(0.8f);

	static const List<String> assets = { "Scene", "Material", "Shader" };
	static SizeType selected = 0;

	if (ImGui::BeginPopupModal("NewAssetPopup", &show, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool clickedOutside = (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow));

		f32 height = ImGui::GetCurrentWindow()->Size.y - ImGui::GetFrameHeightWithSpacing() * 2;
		ImGui::BeginChild("top_panel", ImVec2(0, height), true);

		if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild("AssetList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
			for (SizeType i = 0; i < assets.size(); ++i)
			{
				const auto& asset = assets[i];
				if (ImGui::Selectable(asset.c_str(), selected == i))
				{
					selected = i;
				}
			}
			ImGui::EndChild();
		}

		ImGui::EndChild();

		ImGui::BeginChild("bottom_panel", ImVec2(0, 0), true);

		if (ImGui::Button("Add"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel") || clickedOutside)
		{
			show = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndChild();

		ImGui::EndPopup();
	}
}

//static void SaveGameObject(EntityId entityId)
//{
//	if (g_selectedFolder == INVALID_TREENODE_ID)
//		return;
//
//	const auto& gameObj = GameObject::Find(Scene::GetCurrent(), entityId);
//	const auto& folderPath = AssetManager::GetAssetTree().GetNode(g_selectedFolder).data.path;
//	String path = folderPath + "/" + gameObj.GetName() + ".gameobject";
//	
//	GameObject::Save(gameObj, path);
//}

const char* AssetsView::GetTitle() const
{
	return ICON_FA_IMAGES"  Assets";
}

void AssetsView::Present(const char* title, bool& isOpen)
{
	ValidateSelectedFolder();

	ImGui::Begin(title, &isOpen, ImGuiWindowFlags_NoCollapse);

	ImGui::BeginChild("left_panel", ImVec2(m_leftPanelWidth, 0), true);

	if (ImGui::IsItemClicked())
	{
		m_selection.ClearSelection();
	}

	ShowFileTree(AssetsManager::Get().GetAssetRootId());

	ImGui::EndChild();
	ImGui::SameLine();

	// Adjust the splitter
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

	ImGui::Button(ICON_FA_GRIP_LINES_VERTICAL, ImVec2(10.0f, -1.0f));

	if (ImGui::IsItemActive())
	{
		m_leftPanelWidth += ImGui::GetIO().MouseDelta.x;
	}

	ImGui::PopStyleColor(3);
	ImGui::SameLine();

	// Right panel with grid list of files
	ImGui::BeginChild("right_panel", ImVec2(0, 0), true);

	ImGui::BeginChild("search_panel", ImVec2(0, ImGui::GetFrameHeightWithSpacing()), false);
	
	m_filter.Draw(ICON_FA_SEARCH);
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_TIMES))
	{
		m_filter.Clear();
	}

	ImGui::EndChild();

	ImGui::BeginChild("grid_panel", ImVec2(0, 0), true);

	if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect, ImGui::GetCurrentWindow()->ID))
	{
		//bool consumed = false;
		//for (const auto& callback : g_callbacks)
		//{
		//	consumed |= callback.OnAssetTargetDrop(consumed);
		//
		//	//if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobject"))
		//	//{
		//	//	EntityId entityId = *(const EntityId*)payload->Data;
		//	//	SaveGameObject(entityId);
		//	//}
		//}
		
		ImGui::EndDragDropTarget();
	}

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("AssetPanelContextMenu");
	}

	bool newAssetPopup = false;
	if (ImGui::BeginPopup("AssetPanelContextMenu"))
	{
		if (ImGui::MenuItem("New Folder"))
		{
			const auto& node = AssetsManager::Get().GetAssetTree().GetNode(m_selectedFolder);
			File::CreateDirectory(File::GetPath(node.data.path + "/NewFolder"));
		}

		if (ImGui::MenuItem("New Asset"))
			newAssetPopup = true;

		ImGui::EndPopup();
	}

	ShowGridList();

	ImGui::EndChild();

	ImGui::EndChild();

	if (newAssetPopup)
	{
		m_showNewAssetPopup = true;
		ImGui::OpenPopup("NewAssetPopup");
	}

	ShowCreatePopup(m_showNewAssetPopup);
	ImGui::End();
}
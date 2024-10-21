#pragma once

#include <bx/engine/imgui.hpp>
#include <bx/editor/view.hpp>
#include <bx/editor/assets.hpp>
#include <bx/editor/command.hpp>
#include <bx/editor/selection.hpp>

class BX_API AssetEditor : public View
{
	RTTR_ENABLE(View)

public:
	virtual bool Initialize() { return true; }
	virtual void Reload() {}
	virtual void Shutdown() {}

	virtual const char* GetTitle() const { return "AssetEditor"; }
	virtual void Present(const char* title, bool& isOpen) {}

private:
	RTTR_REGISTRATION_FRIEND
	bool m_assetEditor = true;
};

class BX_API AssetsView final : public View
{
	RTTR_ENABLE(View)

public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	const char* GetTitle() const override;
	void Present(const char* title, bool& isOpen) override;

private:
	void ValidateSelectedFolder();
	void ShowFileTree(TreeNodeId nodeId);
	void ProcessFolderImport(TreeNodeId id, const Asset& asset);
	void ProcessAssetContextMenu(TreeNodeId id, const Asset& asset);
	void ShowAssetIcon(f32 iconScale, f32 cellSize, TreeNodeId id, const Asset& asset);
	void ShowAssetLabel(TreeNodeId id, const Asset& asset);
	void ShowGridList();

private:
	TreeNodeId m_selectedFolder = INVALID_TREENODE_ID;

	TreeNodeId m_fileEditNode = INVALID_TREENODE_ID;
	bool m_fileEditFocused = false;
	String m_fileEditName;

	bool m_showNewAssetPopup = false;

	float m_leftPanelWidth = 200.0f;

	ImGuiTextFilter m_filter;

	Selection m_selection;
};
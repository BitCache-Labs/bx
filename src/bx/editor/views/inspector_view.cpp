#include "bx/editor/views/inspector_view.hpp"

#include "bx/editor/core/assets.hpp"
#include "bx/editor/core/selection.hpp"
#include "bx/editor/views/scene_view.hpp"
#include "bx/editor/views/assets_view.hpp"

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/inspector.hpp>

#include <bx/framework/gameobject.hpp>
#include <bx/framework/components/animator.hpp>
#include <bx/framework/components/audio_listener.hpp>
#include <bx/framework/components/audio_source.hpp>
#include <bx/framework/components/camera.hpp>
#include <bx/framework/components/character_controller.hpp>
#include <bx/framework/components/collider.hpp>
#include <bx/framework/components/light.hpp>
#include <bx/framework/components/mesh_filter.hpp>
#include <bx/framework/components/mesh_renderer.hpp>
#include <bx/framework/components/rigidbody.hpp>
#include <bx/framework/components/spline.hpp>
#include <bx/framework/components/transform.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>

static Resource<Material> g_selectedMaterial;
static String g_selectedMaterialPath;

template <>
class Inspector<GameObjectBase>
{
public:
	static void Inspect(GameObjectBase& obj)
	{
		ImGui::Text("Name");
		ImGui::SameLine();
		ImGui::InputText("##Name", &obj.m_name);
	}
};

template <>
class Inspector<Animator>
{
public:
	static void DebugDraw(const Mat4& trx, const Animator& cmp)
	{
		if (!cmp.m_skeleton)
			return;

		const auto& skelData = cmp.m_skeleton.GetData();
		const auto& boneTree = skelData.GetBoneTree();
		const auto& boneMap = skelData.GetBoneMap();

		const auto& boneMatrices = cmp.m_boneMatrices2;

		boneTree.Recurse(boneTree.GetRoot(),
			[&](TreeNodeId nodeId, const TreeNode<String>& node)
			{
				if (node.parent == INVALID_TREENODE_ID)
					return;

				auto nodeBoneIdx = boneMap.find(node.data)->second;
				auto parentBoneIdx = boneMap.find(boneTree.GetNode(node.parent).data)->second;
				const auto& nodeMatrix = trx * boneMatrices[nodeBoneIdx];
				const auto& parentMatrix = trx * boneMatrices[parentBoneIdx];

				Vec3 a(nodeMatrix.basis[3].x, nodeMatrix.basis[3].y, nodeMatrix.basis[3].z);
				Vec3 b(parentMatrix.basis[3].x, parentMatrix.basis[3].y, parentMatrix.basis[3].z);
				Graphics::DebugLine(a, b, 0xFF00FF00);
			});
	}

	static void Inspect(Animator& cmp)
	{
		if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			ImGui::Checkbox("Enabled", &cmp.m_enabled);

			auto curr = cmp.GetCurrent() == -1 ? -1 : (int)cmp.GetCurrent();
			if (ImGui::SliderInt("Current", &curr, -1, (int)cmp.GetAnimationCount() - 1))
			{
				cmp.SetCurrent((SizeType)curr);
			}
			
			if (ImGui::BeginTable("##SkeletonTable", 2, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableNextColumn();

				ImGui::Text("Skeleton");

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("skeleton"))
					{
						auto path = (const char*)payload->Data;
						cmp.SetSkeleton(Resource<Skeleton>(path));
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::TableNextColumn();
				const auto& skeleton = cmp.GetSkeleton();
				if (skeleton)
				{
					const String& filename = skeleton.GetResourceData().filename;
					ImGui::Text(filename.c_str());
					ImGui::SameLine();
				}

				if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenSkeleton"))
				{
				}

				ImGui::EndTable();
			}

			if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::BeginTable("##AnimationTable", 2, ImGuiTableFlags_SizingStretchProp))
				{
					SizeType removedAnim = -1;
					SizeType index = 0;
					for (const auto& anim : cmp.GetAnimations())
					{
						ImGui::TableNextColumn();

						ImGui::Text("Animation");

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("animation"))
							{
								auto path = (const char*)payload->Data;
								cmp.SetAnimation(index, Resource<Animation>(path));
							}

							ImGui::EndDragDropTarget();
						}

						ImGui::TableNextColumn();
						if (anim)
						{
							const String& filename = anim.GetResourceData().filename;
							ImGui::Text(filename.c_str());
							ImGui::SameLine();
						}

						if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenAnimation"))
						{
						}

						ImGui::SameLine();

						if (ImGui::Button(ICON_FA_MINUS"##RemoveMaterial"))
						{
							removedAnim = index;
						}

						index++;
					}

					if (removedAnim != -1)
					{
						cmp.RemoveAnimation(removedAnim);
					}

					ImGui::EndTable();
				}

				if (ImGui::Button(ICON_FA_PLUS"##NewAnimationLast"))
				{
					cmp.AddAnimation(Resource<Animation>());
				}

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_MINUS"##RemoveAnimationLast"))
				{
					cmp.RemoveAnimation(cmp.GetAnimationCount() - 1);
				}
			}

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<AudioListener>
{
public:
	static void Inspect(AudioListener& cmp)
	{
		if (ImGui::CollapsingHeader("Audio Listener", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			// TODO
			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<AudioSource>
{
public:
	static void Inspect(AudioSource& cmp)
	{
		if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			// TODO
			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<Camera>
{
public:
	static void Inspect(Camera& cmp)
	{
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			cmp.m_dirty |= ImGui::DragFloat("Fov", &cmp.m_fov);
			//cmp.m_dirty |= ImGui::DragFloat("Aspect", &cmp.m_aspect);
			cmp.m_dirty |= ImGui::DragFloat("Near", &cmp.m_zNear);
			cmp.m_dirty |= ImGui::DragFloat("Far", &cmp.m_zFar);

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<CharacterController>
{
public:
	static void Inspect(CharacterController& cmp)
	{
		if (ImGui::CollapsingHeader("Character Controller", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			
			cmp.m_isDirty |= ImGui::DragFloat3("Offset", &cmp.m_offset[0]);
			cmp.m_isDirty |= ImGui::DragFloat("Width", &cmp.m_width);
			cmp.m_isDirty |= ImGui::DragFloat("Height", &cmp.m_height);

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<Collider>
{
public:
	static void Inspect(Collider& cmp)
	{
		if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			
			int shapeIdx = cmp.m_shape.GetIndex();
			if (ImGui::Combo("Shape", &shapeIdx, ColliderShape::GetNames().data(), (int)ColliderShape::GetNames().size()))
			{
				cmp.m_shape = ColliderShape::GetValues()[shapeIdx];
				cmp.m_isDirty = true;
			}

			cmp.m_isDirty |= ImGui::DragFloat3("Center", &cmp.m_center[0]);

			switch (cmp.m_shape)
			{
			case ColliderShape::BOX:
			{
				cmp.m_isDirty |= ImGui::DragFloat3("Size", &cmp.m_size[0]);
				break;
			}

			case ColliderShape::SPHERE:
			{
				cmp.m_isDirty |= ImGui::DragFloat("Radius", &cmp.m_radius);
				break;
			}

			case ColliderShape::CAPSULE:
			{
				cmp.m_isDirty |= ImGui::DragFloat("Radius", &cmp.m_radius);
				cmp.m_isDirty |= ImGui::DragFloat("Height", &cmp.m_height);

				int axisIdx = cmp.m_axis.GetIndex();
				if (ImGui::Combo("Axis", &axisIdx, ColliderAxis::GetNames().data(), (int)ColliderAxis::GetNames().size()))
				{
					cmp.m_axis = ColliderShape::GetValues()[axisIdx];
					cmp.m_isDirty = true;
				}
				break;
			}

			case ColliderShape::MESH:
			{
				cmp.m_isDirty |= ImGui::DragFloat3("Scale", &cmp.m_scale[0]);
				cmp.m_isDirty |= ImGui::Checkbox("Concave", &cmp.m_isConcave);
				if (ImGui::BeginTable("##MeshTable", 2, ImGuiTableFlags_SizingStretchProp))
				{
					ImGui::TableNextColumn();

					ImGui::Text("Mesh");

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("mesh"))
						{
							auto path = (const char*)payload->Data;
							cmp.m_mesh = Resource<Mesh>(path);
							cmp.m_isDirty = true;
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::TableNextColumn();
					if (cmp.m_mesh)
					{
						const String& filename = cmp.m_mesh.GetResourceData().filename;
						ImGui::Text(filename.c_str());
						ImGui::SameLine();
					}

					if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenMesh"))
					{
					}

					ImGui::SameLine();

					if (ImGui::Button(ICON_FA_MINUS"##RemoveMesh"))
					{
					}

					ImGui::EndTable();
				}

				break;
			}
			}

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<Light>
{
public:
	static void Inspect(Light& cmp)
	{
		if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			
			ImGui::DragFloat("Intensity", &cmp.m_intensity);

			ImGui::DragFloat("Constant", &cmp.m_constant);
			ImGui::DragFloat("Linear", &cmp.m_linear);
			ImGui::DragFloat("Quadratic", &cmp.m_quadratic);

			ImGui::ColorEdit3("Color", cmp.m_color.data);

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<MeshFilter>
{
public:
	static void Inspect(MeshFilter& cmp)
	{
		if (ImGui::CollapsingHeader("Mesh Filter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			if (ImGui::BeginTable("##MeshTable", 2, ImGuiTableFlags_SizingStretchProp))
			{
				SizeType removedMesh = -1;
				SizeType index = 0;
				for (const auto& mesh : cmp.GetMeshes())
				{
					ImGui::TableNextColumn();

					ImGui::Text("Mesh");

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("mesh"))
						{
							auto path = (const char*)payload->Data;
							cmp.SetMesh(index, Resource<Mesh>(path));
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::TableNextColumn();
					if (mesh)
					{
						const String& filename = mesh.GetResourceData().filename;
						ImGui::Text(filename.c_str());
						ImGui::SameLine();
					}

					if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenMesh"))
					{
					}
					
					ImGui::SameLine();
					
					if (ImGui::Button(ICON_FA_MINUS"##RemoveMesh"))
					{
						removedMesh = index;
					}

					index++;
				}

				if (removedMesh != -1)
				{
					cmp.RemoveMesh(removedMesh);
				}
				
				ImGui::EndTable();
			}

			if (ImGui::Button(ICON_FA_PLUS"##AddMeshLast"))
			{
				cmp.AddMesh(Resource<Mesh>());
			}

			ImGui::SameLine();
			
			if (ImGui::Button(ICON_FA_MINUS"##RemoveMeshLast"))
			{
				cmp.RemoveMesh(cmp.GetMeshCount() - 1);
			}

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<MeshRenderer>
{
public:
	static void Inspect(MeshRenderer& cmp)
	{
		if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::BeginTable("##MaterialTable", 2, ImGuiTableFlags_SizingStretchProp))
				{
					SizeType removedMaterial = -1;
					SizeType index = 0;
					for (const auto& material : cmp.GetMaterials())
					{
						ImGui::TableNextColumn();

						ImGui::Text("Material");

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material"))
							{
								auto path = (const char*)payload->Data;
								cmp.SetMaterial(index, Resource<Material>(path));
							}

							ImGui::EndDragDropTarget();
						}

						ImGui::TableNextColumn();
						if (material)
						{
							const String& filename = material.GetResourceData().filename;
							ImGui::Text(filename.c_str());
							ImGui::SameLine();
						}

						if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenMaterial"))
						{
						}

						ImGui::SameLine();

						if (ImGui::Button(ICON_FA_MINUS"##RemoveMaterial"))
						{
							removedMaterial = index;
						}

						index++;
					}

					if (removedMaterial != -1)
					{
						cmp.RemoveMaterial(removedMaterial);
					}

					ImGui::EndTable();
				}

				if (ImGui::Button(ICON_FA_PLUS"##NewMaterialLast"))
				{
					cmp.AddMaterial(Resource<Material>());
				}

				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_MINUS"##RemoveMaterialLast"))
				{
					cmp.RemoveMaterial(cmp.GetMaterialCount() - 1);
				}
			}

			if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int index = cmp.m_shadowCastingMode.GetIndex();
				if (ImGui::Combo("Cast Shadows", &index, ShadowCastingMode::GetNames().data(), (int)ShadowCastingMode::GetNames().size()))
				{
					cmp.m_shadowCastingMode = ShadowCastingMode::GetValues()[index];
				}

				ImGui::Checkbox("Receive Shadows", &cmp.m_receiveShadows);
				ImGui::Checkbox("Contribute GI", &cmp.m_contributeGI);
				ImGui::Checkbox("Receive GI", &cmp.m_receiveGI);
			}

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<RigidBody>
{
public:
	static void Inspect(RigidBody& cmp)
	{
		if (ImGui::CollapsingHeader("RigidBody", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			// TODO
			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<Spline>
{
public:
	static void Inspect(Spline& cmp)
	{
		if (ImGui::CollapsingHeader("Spline", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);
			// TODO
			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

static bool DrawVec3Control(const String& name, Vec3& value, bool& hasChanged, float resetVal = 0.0f, float columnWidth = 100.f)
{
	bool isChanging = false;

	ImGui::PushID(name.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);

	ImGui::Text(name.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize(lineHeight + 3.0f, lineHeight);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 1));
	if (ImGui::Button("X", buttonSize))
	{
		value.x = resetVal;
		isChanging |= true;
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();
	isChanging |= ImGui::DragFloat("##X", &value.x, 0.1f);
	//hasChanged |= checkChanged(value);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 1, 0, 1));
	if (ImGui::Button("Y", buttonSize))
	{
		value.y = resetVal;
		isChanging |= true;
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();
	isChanging |= ImGui::DragFloat("##Y", &value.y, 0.1f);
	//hasChanged |= checkChanged(value);


	ImGui::PopItemWidth();
	ImGui::SameLine();


	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 1, 1));
	if (ImGui::Button("Z", buttonSize))
	{
		value.z = resetVal;
		isChanging |= true;
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();
	isChanging |= ImGui::DragFloat("##Z", &value.z, 0.1f);
	//hasChanged |= checkChanged(value);

	ImGui::PopItemWidth();
	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return isChanging;
}

static bool s_transformChanged = false;
static Transform s_prevTransform;

template <>
class Inspector<Transform>
{
public:
	static void Inspect(Transform& cmp)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			bool isChanging = false;
			bool hasChanged = false;

			Vec3 pos = cmp.GetPosition();
			Vec3 rot = cmp.GetRotation().EulerAngles();
			Vec3 scl = cmp.GetScale();

			isChanging |= DrawVec3Control("Position", pos, hasChanged);
			isChanging |= DrawVec3Control("Rotation", rot, hasChanged);
			isChanging |= DrawVec3Control("Scale", scl, hasChanged, 1.0f);

			if (!s_transformChanged && isChanging)
			{
				s_transformChanged = true;
				s_prevTransform = cmp;
			}

			//Check if transform has moved if so add to command history
			if (s_transformChanged && !isChanging)
			{
				s_transformChanged = false;
				//CommandHistory::Add(new TransformChanged(m_currentEntity, g_oldTransform, value));
			}

			cmp.Set(pos, Quat::Euler(rot.x, rot.y, rot.z), scl);

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
};

template <>
class Inspector<Material>
{
public:
	static bool Inspect(Material& data)
	{
		bool changed = false;

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10);

			if (ImGui::BeginTable("##ShaderTable", 2, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableNextColumn();

				ImGui::Text("Shader");
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("shader"))
					{
						auto path = (const char*)payload->Data;
						data.SetShader(Resource<Shader>(path));
						changed = true;
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::TableNextColumn();
				const auto& shader = data.GetShader();
				if (shader)
				{
					const String& filename = shader.GetResourceData().filename;
					ImGui::Text(filename.c_str());
					ImGui::SameLine();
				}

				if (ImGui::Button(ICON_FA_FOLDER_OPEN"##OpenShader"))
				{
				}

				ImGui::EndTable();
			}

			if (ImGui::BeginTable("##TexturesTable", 2, ImGuiTableFlags_SizingStretchProp))
			{
				String removedTexture;
				for (const auto& texture : data.GetTextures())
				{
					ImGui::TableNextColumn();
					ImGui::Text(texture.first.c_str());

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture"))
						{
							auto path = (const char*)payload->Data;
							auto res = Resource<Texture>(path);
							data.GetTexture(texture.first) = res;
							changed = true;
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::TableNextColumn();
					if (texture.second.IsValid() && texture.second.IsLoaded())
					{
						const String& filename = texture.second.GetResourceData().filename;
						ImGui::Text(filename.c_str());
						ImGui::SameLine();
					}

					if (ImGui::Button(ICON_FA_FOLDER_OPEN))
					{
					}

					ImGui::SameLine();
					if (ImGui::Button(ICON_FA_MINUS))
					{
						removedTexture = texture.first;
						changed = true;
					}
				}

				if (!removedTexture.empty())
					data.RemoveTexture(removedTexture);

				ImGui::EndTable();
			}

			static String textureName;
			ImGui::InputText("##TextureName", &textureName);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_PLUS))
			{
				data.GetTexture(textureName);
				textureName = "";
			}

			if (ImGui::Checkbox("Is Emissive", &data.m_isEmissive))
			{
				changed = true;
			}

			ImGui::Unindent(10);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		return changed;
	}
};

#define INSPECT_COMPONENT(entity, Component) if (entity.HasComponent<Component>()) Inspector<Component>::Inspect(entity.GetComponent<Component>());

void InspectorView::Initialize()
{
}

void InspectorView::Shutdown()
{
	g_selectedMaterial = Resource<Material>();
	g_selectedMaterialPath = "";
}

void InspectorView::Present(bool& show)
{
	ImGui::Begin(ICON_FA_EDIT"  Inspector", &show, ImGuiWindowFlags_NoCollapse);

	ObjectRef selected = Selection::GetSelected();

	if (selected.Is<Entity>())
	{
		Entity entity = *selected.As<Entity>();
		if (entity.IsValid())
		{
			GameObjectBase& gameObj = GameObject::Find(Scene::GetCurrent(), entity.GetId());

			Inspector<GameObjectBase>::Inspect(gameObj);
			if (entity.IsValid())
			{
				INSPECT_COMPONENT(entity, Transform);
				INSPECT_COMPONENT(entity, Camera);
				INSPECT_COMPONENT(entity, Light);
				INSPECT_COMPONENT(entity, AudioSource);
				INSPECT_COMPONENT(entity, AudioListener);
				INSPECT_COMPONENT(entity, MeshFilter);
				INSPECT_COMPONENT(entity, MeshRenderer);
				INSPECT_COMPONENT(entity, Animator);
				INSPECT_COMPONENT(entity, CharacterController);
				INSPECT_COMPONENT(entity, Collider);
				INSPECT_COMPONENT(entity, RigidBody);
				INSPECT_COMPONENT(entity, Spline);

				if (entity.HasComponent<Transform>() && entity.HasComponent<Animator>())
				{
					const auto& trx = entity.GetComponent<Transform>();
					const auto& anim = entity.GetComponent<Animator>();
					Inspector<Animator>::DebugDraw(trx.GetMatrix(), anim);
				}
			}
		}
		else
		{
			Selection::ClearSelection();
		}
	}
	else if (selected.Is<TreeNodeId>())
	{
		TreeNodeId nodeId = *selected.As<TreeNodeId>();
		if (AssetManager::GetAssetTree().IsValid(nodeId))
		{
			const auto& node = AssetManager::GetAssetTree().GetNode(nodeId);
			if (node.data.extension == "material")
			{
				g_selectedMaterial = Resource<Material>(node.data.path);
				g_selectedMaterialPath = node.data.path;
				if (Inspector<Material>::Inspect(g_selectedMaterial.GetData()))
				{
					g_selectedMaterial.Save(g_selectedMaterialPath);
				}
			}
			else
			{
				if (g_selectedMaterial.IsValid())
				{
					g_selectedMaterial.Save(g_selectedMaterialPath);
					g_selectedMaterial = Resource<Material>();
					g_selectedMaterialPath = "";
				}
			}
		}
		else
		{
			Selection::ClearSelection();
		}
	}

	ImGui::End();
}
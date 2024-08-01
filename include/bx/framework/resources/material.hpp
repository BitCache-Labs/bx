#pragma once

#include "bx/framework/resources/shader.hpp"
#include "bx/framework/resources/texture.hpp"

#include <bx/engine/core/math.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/string.hpp>
#include <bx/engine/containers/list.hpp>
#include <bx/engine/modules/graphics.hpp>

class Material
{
public:
	inline const Resource<Shader>& GetShader() const { return m_shader; }
	inline void SetShader(Resource<Shader> shader) { m_shader = shader; /*TODO: some dirty flag to query in renderer and rebuild pipelines*/ }

	inline const Vec4& GetColor() const { return m_color; }
	inline void SetColor(const Vec4& color) { m_color = color; }

	inline Resource<Texture>& GetTexture(const String& name) { return m_textures[name]; }
	inline void RemoveTexture(const String& name) { m_textures.erase(m_textures.find(name)); }
	inline const HashMap<String, Resource<Texture>>& GetTextures() const { return m_textures; }

	static BindGroupLayoutDescriptor GetBindGroupLayout();
	BindGroupHandle GetBindGroup(BindGroupLayoutHandle layout) const;

	// TODO: maybe auto generate? a ShaderBindable interface to inherit from can be a solution
	static constexpr u32 SHADER_BIND_GROUP = 1;

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Resource;

	template <typename T>
	friend class Inspector;

private:
	Resource<Shader> m_shader;

	mutable HashMap<BindGroupLayoutHandle, BindGroupHandle> m_bindGroupCache;

	Vec4 m_color = Vec4(1, 1, 1, 1);
	HashMap<String, Resource<Texture>> m_textures;
};
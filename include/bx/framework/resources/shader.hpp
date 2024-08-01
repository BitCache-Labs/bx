#pragma once

#include <bx/engine/core/math.hpp>
#include <bx/engine/containers/string.hpp>
#include <bx/engine/containers/list.hpp>
#include <bx/engine/modules/graphics.hpp>

class Shader
{
public:
	const String& GetSource() const { return m_source; }
	void SetSource(const String& src) { m_source = src; }

	ShaderHandle GetVertexShader() const { return m_vertexShader; }
	ShaderHandle GetFragmentShader() const { return m_fragmentShader; }

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Resource;

	List<String> m_includes;
	List<String> m_macros;
	String m_source;
	ShaderHandle m_vertexShader = ShaderHandle::null;
	ShaderHandle m_fragmentShader = ShaderHandle::null;
};
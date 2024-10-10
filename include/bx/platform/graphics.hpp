#pragma once

#include <bx/bx.hpp>
#include <bx/math/math.hpp>
#include <bx/meta/enum.hpp>
#include <bx/core/macros.hpp>

#include <rttr/rttr_enable.h>

using GraphicsHandle = u64;
constexpr GraphicsHandle INVALID_GRAPHICS_HANDLE = -1;

enum struct BX_API GraphicsClearFlags { NONE, DEPTH, STENCIL };
enum struct BX_API GraphicsValueType { UNDEFINED, INT8, INT16, INT32, UINT8, UINT16, UINT32, FLOAT16, FLOAT32 };

enum struct BX_API ShaderType { UNKNOWN, VERTEX, PIXEL, GEOMETRY, COMPUTE };

enum struct BX_API BufferType { VERTEX_BUFFER, INDEX_BUFFER, UNIFORM_BUFFER, STORAGE_BUFFER };
enum struct BX_API BufferUsage { IMMUTABLE, DEFAULT, DYNAMIC };
enum struct BX_API BufferAccess { NONE, READ, WRITE };

enum struct BX_API TextureFormat { UNKNOWN, RGB8_UNORM, RGBA8_UNORM, RG32_UINT, D24_UNORM_S8_UINT };
enum struct BX_API TextureFlags { NONE = BX_BIT(0), SHADER_RESOURCE = BX_BIT(1), RENDER_TARGET = BX_BIT(2), DEPTH_STENCIL = BX_BIT(3) };

enum struct BX_API ResourceBindingType { UNKNOWN, TEXTURE, UNIFORM_BUFFER, STORAGE_BUFFER };
enum struct BX_API ResourceBindingAccess { STATIC, MUTABLE, DYNAMIC };

enum struct BX_API PipelineTopology { UNDEFINED, POINTS, LINES, TRIANGLES };
enum struct BX_API PipelineFaceCull { NONE, CW, CCW };

struct BX_API ShaderInfo
{
	ShaderType shaderType = ShaderType::UNKNOWN;
	const char* source = nullptr;
};

struct BX_API BufferInfo
{
	u32 strideBytes = 0;
	BufferType type = BufferType::UNIFORM_BUFFER;
	BufferUsage usage = BufferUsage::DYNAMIC;
	BufferAccess access = BufferAccess::WRITE;
};

struct BX_API TextureInfo
{
	TextureFormat format = TextureFormat::UNKNOWN;
	u32 width = 0;
	u32 height = 0;
	TextureFlags flags = TextureFlags::SHADER_RESOURCE;
};

struct BX_API BufferData
{
	BufferData() {}
	BufferData(const void* pData, u32 dataSize)
		: pData(pData)
		, dataSize(dataSize) {}

	const void* pData = nullptr;
	u32 dataSize = 0;
};

struct BX_API ResourceBindingElement
{
	ResourceBindingElement() {}
	ResourceBindingElement(ShaderType shaderType, const char* name, u32 count, ResourceBindingType type, ResourceBindingAccess access)
		: shaderType(shaderType)
		, name(name)
		, count(count)
		, type(type)
		, access(access) {}

	ShaderType shaderType = ShaderType::UNKNOWN;
	const char* name = nullptr;
	u32 count = 0;
	ResourceBindingType type = ResourceBindingType::UNKNOWN;
	ResourceBindingAccess access = ResourceBindingAccess::STATIC;
};

struct BX_API ResourceBindingInfo
{
	const ResourceBindingElement* resources = nullptr;
	u32 numResources = 0;
};

struct BX_API LayoutElement
{
	LayoutElement() {}
	LayoutElement(u32 inputIndex, u32 bufferSlot, u32 numComponents, GraphicsValueType valueType, bool isNormalized, u32 relativeOffset, u32 instanceDataStepRate)
		: inputIndex(inputIndex)
		, bufferSlot(bufferSlot)
		, numComponents(numComponents)
		, valueType(valueType)
		, isNormalized(isNormalized)
		, relativeOffset(relativeOffset)
		, instanceDataStepRate(instanceDataStepRate) {}

	u32 inputIndex = 0;
	u32 bufferSlot = 0;
	u32 numComponents = 0;
	GraphicsValueType valueType = GraphicsValueType::FLOAT32;
	bool isNormalized = false;
	u32 relativeOffset = 0;
	u32 instanceDataStepRate = 0;
};

struct BX_API PipelineInfo
{
	u32 numRenderTargets = 0;
	TextureFormat renderTargetFormats[8] = { TextureFormat::UNKNOWN };
	TextureFormat depthStencilFormat = TextureFormat::UNKNOWN;

	PipelineTopology topology = PipelineTopology::UNDEFINED;
	PipelineFaceCull faceCull = PipelineFaceCull::NONE;

	bool depthEnable = true;
	bool blendEnable = false;

	GraphicsHandle vertShader = INVALID_GRAPHICS_HANDLE;
	GraphicsHandle pixelShader = INVALID_GRAPHICS_HANDLE;
	
	const LayoutElement* layoutElements = nullptr;
	u32 numElements = 0;
};

struct BX_API DrawAttribs
{
	DrawAttribs() {}
	DrawAttribs(u32 numVertices)
		: numVertices(numVertices) {}

	u32 numVertices = 0;
};

struct BX_API DrawIndexedAttribs
{
	DrawIndexedAttribs() {}
	DrawIndexedAttribs(GraphicsValueType indexType, u32 numIndices)
		: indexType(indexType), numIndices(numIndices) {}

	GraphicsValueType indexType = GraphicsValueType::UINT32;
	u32 numIndices = 0;
};

struct BX_API DebugVertex
{
	DebugVertex() {}
	DebugVertex(const Vec3& vert, u32 col)
		: vert(vert), col(col) {}

	Vec3 vert{ 0, 0, 0 };
	u32 col{ 0 };
};

struct BX_API DebugDrawAttribs
{
};

struct BX_API DebugLineData
{
	DebugLineData() {}
	DebugLineData(const Vec3& a, const Vec3& b, u32 c, f32 l)
		: a(a), b(b), color(c), lifespan(l) {}

	Vec3 a = Vec3(0, 0, 0);
	Vec3 b = Vec3(0, 0, 0);
	u32 color = 0;
	f32 lifespan = 0.0f;
};

class BX_API Graphics
{
	RTTR_ENABLE()

public:
	static Graphics& Get();

public:
	Graphics() = default;
	virtual ~Graphics() = default;

	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual void NewFrame() = 0;
	virtual void EndFrame() = 0;

	virtual TextureFormat GetColorBufferFormat() = 0;
	virtual TextureFormat GetDepthBufferFormat() = 0;

	virtual GraphicsHandle GetCurrentBackBufferRT() = 0;
	virtual GraphicsHandle GetDepthBuffer() = 0;

	virtual void SetRenderTarget(const GraphicsHandle renderTarget, const GraphicsHandle depthStencil) = 0;
	virtual void ReadPixels(u32 x, u32 y, u32 w, u32 h, void* pixelData, const GraphicsHandle renderTarget) = 0;

	virtual void SetViewport(const f32 viewport[4]) = 0;

	virtual void ClearRenderTarget(const GraphicsHandle renderTarget, const f32 clearColor[4]) = 0;
	virtual void ClearDepthStencil(const GraphicsHandle depthStencil, GraphicsClearFlags flags, f32 depth, i32 stencil) = 0;

	virtual GraphicsHandle CreateShader(const ShaderInfo& info) = 0;
	virtual void DestroyShader(const GraphicsHandle shader) = 0;

	virtual GraphicsHandle CreateTexture(const TextureInfo& info) = 0;
	virtual GraphicsHandle CreateTexture(const TextureInfo& info, const BufferData& data) = 0;
	virtual void DestroyTexture(const GraphicsHandle texture) = 0;

	virtual GraphicsHandle CreateResourceBinding(const ResourceBindingInfo& info) = 0;
	virtual void DestroyResourceBinding(const GraphicsHandle resources) = 0;
	virtual void BindResource(const GraphicsHandle resources, const char* name, GraphicsHandle resource) = 0;

	virtual GraphicsHandle CreatePipeline(const PipelineInfo& info) = 0;
	virtual void DestroyPipeline(const GraphicsHandle pipeline) = 0;
	virtual void SetPipeline(const GraphicsHandle pipeline) = 0;
	virtual void CommitResources(const GraphicsHandle pipeline, const GraphicsHandle resources) = 0;

	virtual GraphicsHandle CreateBuffer(const BufferInfo& info) = 0;
	virtual GraphicsHandle CreateBuffer(const BufferInfo& info, const BufferData& data) = 0;
	virtual void DestroyBuffer(const GraphicsHandle buffer) = 0;
	virtual void UpdateBuffer(const GraphicsHandle buffer, const BufferData& data) = 0;

	virtual void SetVertexBuffers(i32 i, i32 count, const GraphicsHandle* pBuffers, const u64* offset) = 0;
	virtual void SetIndexBuffer(const GraphicsHandle buffer, i32 i) = 0;

	virtual void Draw(const DrawAttribs& attribs) = 0;
	virtual void DrawIndexed(const DrawIndexedAttribs& attribs) = 0;

	// Debug draw utilities
	virtual void DebugLine(const Vec3& a, const Vec3& b, u32 color = 0xFFFFFFFF, f32 lifespan = 0.0f) = 0;

	virtual void UpdateDebugLines() = 0;
	virtual void DrawDebugLines(const Mat4& viewProj) = 0;
	virtual void ClearDebugLines() = 0;

	virtual void DebugDraw(const Mat4& viewProj, const DebugDrawAttribs& attribs, const List<DebugVertex>& vertices) = 0;

private:
	List<DebugLineData> m_debugLines;
	List<DebugLineData> m_debugLinesBuffer;

	List<DebugVertex> m_debugVertices;
};

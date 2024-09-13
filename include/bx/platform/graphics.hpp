#pragma once

#include "bx/engine/core/math.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/core/macros.hpp"

using GraphicsHandle = u64;
constexpr GraphicsHandle INVALID_GRAPHICS_HANDLE = -1;

ENUM(GraphicsClearFlags, NONE, DEPTH, STENCIL);
ENUM(GraphicsValueType, UNDEFINED, INT8, INT16, INT32, UINT8, UINT16, UINT32, FLOAT16, FLOAT32);

ENUM(ShaderType, UNKNOWN, VERTEX, PIXEL, GEOMETRY, COMPUTE);

ENUM(BufferType, VERTEX_BUFFER, INDEX_BUFFER, UNIFORM_BUFFER, STORAGE_BUFFER);
ENUM(BufferUsage, IMMUTABLE, DEFAULT, DYNAMIC);
ENUM(BufferAccess, NONE, READ, WRITE);

ENUM(TextureFormat, UNKNOWN, RGB8_UNORM, RGBA8_UNORM, RG32_UINT, D24_UNORM_S8_UINT);
ENUM(TextureFlags, NONE = BX_BIT(0), SHADER_RESOURCE = BX_BIT(1), RENDER_TARGET = BX_BIT(2), DEPTH_STENCIL = BX_BIT(3));

ENUM(ResourceBindingType, UNKNOWN, TEXTURE, UNIFORM_BUFFER, STORAGE_BUFFER);
ENUM(ResourceBindingAccess, STATIC, MUTABLE, DYNAMIC);

ENUM(PipelineTopology, UNDEFINED, POINTS, LINES, TRIANGLES);
ENUM(PipelineFaceCull, NONE, CW, CCW);

struct ShaderInfo
{
	ShaderType shaderType = ShaderType::UNKNOWN;
	const char* source = nullptr;
};

struct BufferInfo
{
	u32 strideBytes = 0;
	BufferType type = BufferType::UNIFORM_BUFFER;
	BufferUsage usage = BufferUsage::DYNAMIC;
	BufferAccess access = BufferAccess::WRITE;
};

struct TextureInfo
{
	TextureFormat format = TextureFormat::UNKNOWN;
	u32 width = 0;
	u32 height = 0;
	TextureFlags flags = TextureFlags::SHADER_RESOURCE;
};

struct BufferData
{
	BufferData() {}
	BufferData(const void* pData, u32 dataSize)
		: pData(pData)
		, dataSize(dataSize) {}

	const void* pData = nullptr;
	u32 dataSize = 0;
};

struct ResourceBindingElement
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

struct ResourceBindingInfo
{
	const ResourceBindingElement* resources = nullptr;
	u32 numResources = 0;
};

struct LayoutElement
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

struct PipelineInfo
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

struct DrawAttribs
{
	DrawAttribs() {}
	DrawAttribs(u32 numVertices)
		: numVertices(numVertices) {}

	u32 numVertices = 0;
};

struct DrawIndexedAttribs
{
	DrawIndexedAttribs() {}
	DrawIndexedAttribs(GraphicsValueType indexType, u32 numIndices)
		: indexType(indexType), numIndices(numIndices) {}

	GraphicsValueType indexType = GraphicsValueType::UINT32;
	u32 numIndices = 0;
};

struct DebugVertex
{
	DebugVertex() {}
	DebugVertex(const Vec3& vert, u32 col)
		: vert(vert), col(col) {}

	Vec3 vert{ 0, 0, 0 };
	u32 col{ 0 };
};

struct DebugDrawAttribs
{
};

class Graphics
{
public:
	static TextureFormat GetColorBufferFormat();
	static TextureFormat GetDepthBufferFormat();

	static GraphicsHandle GetCurrentBackBufferRT();
	static GraphicsHandle GetDepthBuffer();

	static void SetRenderTarget(const GraphicsHandle renderTarget, const GraphicsHandle depthStencil);
	static void ReadPixels(u32 x, u32 y, u32 w, u32 h, void* pixelData, const GraphicsHandle renderTarget);

	static void SetViewport(const f32 viewport[4]);

	static void ClearRenderTarget(const GraphicsHandle renderTarget, const f32 clearColor[4]);
	static void ClearDepthStencil(const GraphicsHandle depthStencil, GraphicsClearFlags flags, f32 depth, i32 stencil);

	static GraphicsHandle CreateShader(const ShaderInfo& info);
	static void DestroyShader(const GraphicsHandle shader);

	static GraphicsHandle CreateTexture(const TextureInfo& info);
	static GraphicsHandle CreateTexture(const TextureInfo& info, const BufferData& data);
	static void DestroyTexture(const GraphicsHandle texture);

	static GraphicsHandle CreateResourceBinding(const ResourceBindingInfo& info);
	static void DestroyResourceBinding(const GraphicsHandle resources);
	static void BindResource(const GraphicsHandle resources, const char* name, GraphicsHandle resource);

	static GraphicsHandle CreatePipeline(const PipelineInfo& info);
	static void DestroyPipeline(const GraphicsHandle pipeline);
	static void SetPipeline(const GraphicsHandle pipeline);
	static void CommitResources(const GraphicsHandle pipeline, const GraphicsHandle resources);

	static GraphicsHandle CreateBuffer(const BufferInfo& info);
	static GraphicsHandle CreateBuffer(const BufferInfo& info, const BufferData& data);
	static void DestroyBuffer(const GraphicsHandle buffer);
	static void UpdateBuffer(const GraphicsHandle buffer, const BufferData& data);

	static void SetVertexBuffers(i32 i, i32 count, const GraphicsHandle* pBuffers, const u64* offset);
	static void SetIndexBuffer(const GraphicsHandle buffer, i32 i);

	static void Draw(const DrawAttribs& attribs);
	static void DrawIndexed(const DrawIndexedAttribs& attribs);

	// Debug draw utilities
	static void DebugLine(const Vec3& a, const Vec3& b, u32 color = 0xFFFFFFFF, f32 lifespan = 0.0f);

	static void UpdateDebugLines();
	static void DrawDebugLines(const Mat4& viewProj);
	static void ClearDebugLines();

	static void DebugDraw(const Mat4& viewProj, const DebugDrawAttribs& attribs, const List<DebugVertex>& vertices);

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static void NewFrame();
	static void EndFrame();
};
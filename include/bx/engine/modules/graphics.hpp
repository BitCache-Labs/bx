#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/modules/graphics/type.hpp"

class Graphics
{
public:
	// TODO: capabilities()

	// Buffer with 1 byte set to zero, can only be used for read-only purposes (BufferUsageFlags::COPY_SRC | BufferUsageFlags::INDEX | BufferUsageFlags::VERTEX | BufferUsageFlags::UNIFORM | BufferUsageFlags::STORAGE)
	static const BufferHandle& EmptyBuffer();
	// 2D texture with 1 pixel set to zero, can only be used for read-only purposes (TextureUsageFlags::COPY_SRC | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING)
	static const TextureHandle& EmptyTexture();

	// Swapchain
	// TODO: rework api a bit so it matches the old func calls better
	static TextureHandle GetSwapchainColorTarget();
	static TextureViewHandle GetSwapchainColorTargetView();

	// Resource creation and destruction
	static const TextureCreateInfo& GetTextureCreateInfo(TextureHandle texture);
	static TextureHandle CreateTexture(const TextureCreateInfo& createInfo);
	static TextureHandle CreateTexture(const TextureCreateInfo& createInfo, const void* data);
	static void DestroyTexture(TextureHandle& texture);

	static TextureViewHandle CreateTextureView(TextureHandle texture);
	static void DestroyTextureView(TextureViewHandle& textureView);

	static const SamplerCreateInfo& GetSamplerCreateInfo(SamplerHandle sampler);
	static SamplerHandle CreateSampler(const SamplerCreateInfo& create);
	static void DestroySampler(SamplerHandle& sampler);

	static const BufferCreateInfo& GetBufferCreateInfo(BufferHandle buffer);
	static BufferHandle CreateBuffer(const BufferCreateInfo& createInfo);
	static BufferHandle CreateBuffer(const BufferCreateInfo& createInfo, const void* data);
	static void DestroyBuffer(BufferHandle& buffer);

	static const ShaderCreateInfo& GetShaderCreateInfo(ShaderHandle shader);
	static ShaderHandle CreateShader(const ShaderCreateInfo& createInfo);
	static void DestroyShader(ShaderHandle& shader);
	
	static const GraphicsPipelineCreateInfo& GetGraphicsPipelineCreateInfo(GraphicsPipelineHandle graphicsPipeline);
	static GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
	static void DestroyGraphicsPipeline(GraphicsPipelineHandle& graphicsPipeline);
	static const ComputePipelineCreateInfo& GetComputePipelineCreateInfo(ComputePipelineHandle computePipeline);
	static ComputePipelineHandle CreateComputePipeline(const ComputePipelineCreateInfo& createInfo);
	static void DestroyComputePipeline(ComputePipelineHandle& computePipeline);

	static BindGroupLayoutHandle GetBindGroupLayout(GraphicsPipelineHandle graphicsPipeline, u32 bindGroup);
	static BindGroupLayoutHandle GetBindGroupLayout(ComputePipelineHandle computePipeline, u32 bindGroup);
	static u32 GetBindGroupLayoutBindGroup(BindGroupLayoutHandle bindGroupLayout);
	static b8 IsBindGroupLayoutGraphics(BindGroupLayoutHandle bindGroupLayout);
	static u64 GetBindGroupLayoutPipeline(BindGroupLayoutHandle bindGroupLayout);

	static const BindGroupCreateInfo& GetBindGroupCreateInfo(BindGroupHandle bindGroup);
	static BindGroupHandle CreateBindGroup(const BindGroupCreateInfo& createInfo);
	static void DestroyBindGroup(BindGroupHandle& bindGroup);

	// Cmds
	static const RenderPassDescriptor& GetRenderPassDescriptor(RenderPassHandle renderPass);
	static RenderPassHandle BeginRenderPass(const RenderPassDescriptor& descriptor);
	// Set a graphics pipeline, will unbind any vertex and index buffers previously bound
	static void SetGraphicsPipeline(GraphicsPipelineHandle graphicsPipeline);
	static void SetVertexBuffer(u32 slot, const BufferSlice& bufferSlice);
	static void SetIndexBuffer(const BufferSlice& bufferSlice, IndexFormat format);
	static void SetBindGroup(u32 index, BindGroupHandle bindGroup);
	static void Draw(u32 vertexCount, u32 firstVertex = 0, u32 instanceCount = 1);
	static void DrawIndexed(u32 indexCount, u32 instanceCount = 1);
	static void EndRenderPass(RenderPassHandle& renderPass);

	static ComputePassHandle BeginComputePass(const ComputePassDescriptor& descriptor);
	static void SetComputePipeline(ComputePipelineHandle computePipeline);
	static void DispatchWorkgroups(u32 x, u32 y, u32 z);
	static void EndComputePass(ComputePassHandle& computePass);

	static void WriteBuffer(BufferHandle buffer, u64 offset, const void* data);
	static void WriteBuffer(BufferHandle buffer, u64 offset, const void* data, SizeType size);

	static void WriteTexture(TextureHandle texture, const void* data, const Extend3D& offset, const Extend3D& size);
	static void ReadTexture(TextureHandle texture, void* data,  const Extend3D& offset, const Extend3D& size);

	// Debug draw utilities
	static void DebugLine(const Vec3& a, const Vec3& b, u32 color = 0xFFFFFFFF, f32 lifespan = 0.0f);

	static void UpdateDebugLines();
	static void DrawDebugLines(const Mat4& viewProj);
	static void ClearDebugLines();

	static void DebugDraw(const Mat4& viewProj, const DebugDrawAttribs& attribs, const List<DebugVertex>& vertices);

private:
	friend class Runtime;
	friend class Module;

	// Internal use only, query `Capabilities` for accurate limits.
	static constexpr size_t MAX_BIND_GROUPS = 64;

	struct CreateInfoCache : NoCopy
	{
		HashMap<BufferHandle, BufferCreateInfo> bufferCreateInfos;
		HashMap<SamplerHandle, SamplerCreateInfo> samplerCreateInfos;
		HashMap<TextureHandle, TextureCreateInfo> textureCreateInfos;
		HashMap<ShaderHandle, ShaderCreateInfo> shaderCreateInfos;
		HashMap<GraphicsPipelineHandle, GraphicsPipelineCreateInfo> graphicsPipelineCreateInfos;
		HashMap<ComputePipelineHandle, ComputePipelineCreateInfo> computePipelineCreateInfos;
		HashMap<BindGroupHandle, BindGroupCreateInfo> bindGroupCreateInfos;
		HashMap<RenderPassHandle, RenderPassDescriptor> renderPassCreateInfos;
	};
	static std::unique_ptr<CreateInfoCache> s_createInfoCache;

	static b8 Initialize();
	static void Reload();
	static void Shutdown();

	static void NewFrame();
	static void EndFrame();
};
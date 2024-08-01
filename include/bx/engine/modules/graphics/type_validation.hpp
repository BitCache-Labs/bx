#pragma once

#include "type.hpp"

b8 ValidateBufferCreateInfo(const BufferCreateInfo& createInfo);
b8 ValidateSamplerCreateInfo(const SamplerCreateInfo& createInfo);
b8 ValidateTextureCreateInfo(const TextureCreateInfo& createInfo);
b8 ValidateShaderCreateInfo(const ShaderCreateInfo& createInfo);
b8 ValidateBindGroupCreateInfo(const BindGroupCreateInfo& createInfo);
b8 ValidateGraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo& createInfo);
b8 ValidateComputePipelineCreateInfo(const ComputePipelineCreateInfo& createInfo);
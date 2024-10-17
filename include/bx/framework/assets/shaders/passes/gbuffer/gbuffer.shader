#ifndef GBUFFER_H
#define GBUFFER_H

#include "[engine]/shaders/math.shader"

struct GBufferData
{
	vec3 normal;
	float distance;
	vec2 texCoord;
	bool frontFace;
	uint blasInstanceIdx;
};

GBufferData GBufferData_loadAll(texture2D gbufferTexture, sampler s, ivec2 pixel, uvec2 resolution)
{
	vec2 uv = pixelToUv(pixel, resolution);
    vec4 gbufferData = texture(sampler2D(gbufferTexture, s), uv);

	GBufferData result;
    result.normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    result.distance = (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r;
	result.texCoord = unpackHalf2x16(floatBitsToUint(gbufferData.b));
    uint blasInstanceIdxAndFrontFacing = floatBitsToUint(gbufferData.a);
    result.blasInstanceIdx = (blasInstanceIdxAndFrontFacing << 1) >> 1;
    result.frontFace = (blasInstanceIdxAndFrontFacing >> 31) == 1;
	return result;
}

GBufferData GBufferData_loadDepth(texture2D gbufferTexture, sampler s, ivec2 pixel, uvec2 resolution)
{
	vec2 uv = pixelToUv(pixel, resolution);
    vec4 gbufferData = texture(sampler2D(gbufferTexture, s), uv);

	GBufferData result;
    result.normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
	return result;
}

GBufferData GBufferData_loadNormalDepth(texture2D gbufferTexture, sampler s, ivec2 pixel, uvec2 resolution)
{
	vec2 uv = pixelToUv(pixel, resolution);
    vec4 gbufferData = texture(sampler2D(gbufferTexture, s), uv);

	GBufferData result;
    result.normal = unpackNormalizedXyz10(PackedNormalizedXyz10(floatBitsToUint(gbufferData.g)), 0);
    result.distance = (gbufferData.r == 0.0) ? 0.0 : 1.0 / gbufferData.r;
	return result;
}

bool GBufferData_isSky(GBufferData self)
{
	return self.distance == 0.0;
}

void _depthTestMin(texture2D gbufferTexture, sampler s, ivec2 pixel, uvec2 resolution, inout float minDepth, inout ivec2 minPixel)
{
	float depth = GBufferData_loadDepth(gbufferTexture, s, pixel, resolution).distance;

	if (depth < minDepth)
	{
		minDepth = depth;
		minPixel = pixel;
	}
}

vec2 getVelocityDepthDilated(texture2D velocityTexture, texture2D gbufferTexture, sampler s, ivec2 pixel, uvec2 resolution)
{
	float minDepth = GBufferData_loadDepth(gbufferTexture, s, pixel, resolution).distance;
	ivec2 minPixel = pixel;

	#pragma unroll
	for	(int x = -1; x <= 1; x++)
	{
		#pragma unroll
		for (int y = -1; y <= 1; y++)
		{
			_depthTestMin(gbufferTexture, s, pixel + ivec2(x, y), resolution, minDepth, minPixel);
		}
	}

	vec2 uv = pixelToUv(minPixel, resolution);
	vec2 velocity = texture(sampler2D(velocityTexture, s), uv).xy;
	return velocity;
}

vec2 getVelocity(texture2D velocityTexture, sampler s, ivec2 pixel, uvec2 resolution)
{
	vec2 uv = pixelToUv(pixel, resolution);
	vec2 velocity = texture(sampler2D(velocityTexture, s), uv).xy;
	return velocity;
}

bool GBufferData_isDisoccluded(GBufferData current, GBufferData history)
{
	if (GBufferData_isSky(history))
	{
		return true;
	}

    if (abs(current.distance - history.distance) > 0.8)
    {
        return true;
    }

    if (dot(current.normal, history.normal) < 0.9)
    {
        return true;
    }

	return false;
}

bool GBufferData_isDisoccludedStrict(GBufferData current, GBufferData history)
{
	if (GBufferData_isSky(history))
	{
		return true;
	}

	if (current.blasInstanceIdx != history.blasInstanceIdx)
    {
        return true;
    }

    if (abs(current.distance - history.distance) > 0.8)
    {
        return true;
    }

    if (dot(current.normal, history.normal) < 0.9)
    {
        return true;
    }

	return false;
}

#endif // GBUFFER_H
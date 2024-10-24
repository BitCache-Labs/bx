#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/math.shader"

#include "[engine]/shaders/passes/gbuffer/gbuffer.shader"

const float ACCEPTANCE_THRESHOLD = 0.01;

layout (BINDING(0, 0), std140) uniform _Constants
{
    mat4 clipToView;
    mat4 viewToClip;
    mat4 clipToPrevClip;
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1)) uniform texture2D velocity;
layout (BINDING(0, 2)) uniform texture2D depth;
layout (BINDING(0, 3), rg32f) uniform image2D reprojection;

layout (BINDING(0, 4)) uniform sampler nearestClampSampler;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    vec2 uv = pixelToUv(pixel, constants.resolution);

    float depth = texture(sampler2D(depth, nearestClampSampler), uv).r;
    
    vec4 positionCs = vec4(uvToClip(uv), depth, 1.0);

    vec4 positionVs = constants.clipToView * positionCs;
    vec4 prevPositionVs;
    
    bool isSky = depth == 1.0;
    if (isSky)
    {
        prevPositionVs = positionVs;
    }
    else
    {
        vec3 velocityVs = texture(sampler2D(velocity, nearestClampSampler), uv).rgb;
        prevPositionVs = (positionVs / positionVs.w) + vec4(velocityVs, 0.0);
    }

    vec4 prevPositionCs = constants.viewToClip * prevPositionVs;
    vec4 prevPositionPrevFrameCs = constants.clipToPrevClip * prevPositionCs;
    vec4 prevPositionPrevFrameNdc = prevPositionPrevFrameCs / prevPositionPrevFrameCs.w;

    vec2 prevFrameUv = clipToUv(prevPositionPrevFrameNdc.xy);
    vec2 prevToCurrentFrameUvOffset = uv - prevFrameUv;

    imageStore(reprojection, pixel, vec4(prevToCurrentFrameUvOffset, 1.0, 1.0));
}
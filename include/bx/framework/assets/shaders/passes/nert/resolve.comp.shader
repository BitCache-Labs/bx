#include "[engine]/shaders/Language.shader"

#define MATERIAL_BINDINGS
#define BLAS_DATA_BINDINGS
#define SKY_BINDINGS

#include "[engine]/shaders/ray_tracing/material.shader"
#include "[engine]/shaders/ray_tracing/blas_data.shader"
#include "[engine]/shaders/ray_tracing/sky.shader"

#include "[engine]/shaders/packing.shader"
#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uvec2 resolution;
    uint _PADDING0;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), rgba32f) uniform image2D ambientEmissiveBaseColor;
layout (BINDING(0, 2)) uniform texture2D denoisedIllumination;
layout (BINDING(0, 3), rgba32f) uniform image2D outImage;

layout (BINDING(0, 4), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 5)) uniform sampler linearRepeatSampler;

layout (BINDING(0, 6), r32f) uniform image2D throughputs;

layout (BINDING(0, 7), std430) readonly buffer _Rays
{
    PackedRay rays[];
};

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    uint id = uint(pixel.y * constants.resolution.x + pixel.x);
    if (pixel.x >= constants.resolution.x || pixel.y >= constants.resolution.y) return;

    Intersection intersection = intersections[id];
    Ray ray = unpackRay(rays[id]);

    vec3 throughput = unpackRgb9e5(PackedRgb9e5(floatBitsToUint(imageLoad(throughputs, pixel).r)));

    vec3 ambientContribution = vec3(0.0);
    vec3 emissiveContribution = vec3(0.0);
    vec3 lightingContribution = vec3(0.0);
    vec3 baseColorFactor = vec3(0.0);

    vec3 ambientFactor = 1.0 * vec3(0.15, 0.15, 0.17);

    if (intersection.t != T_MISS)
    {
        BlasInstance blasInstance = blasInstances[intersection.blasInstanceIdx];
        BlasAccessor blasAccessor = blasAccessors[blasInstance.blasIdx];

        Triangle triangle = blasTriangles[intersection.primitiveIdx + blasAccessor.triangleOffset];
        PackedVertex vertex0 = blasVertices[triangle.i0 + blasAccessor.vertexOffset];
        PackedVertex vertex1 = blasVertices[triangle.i1 + blasAccessor.vertexOffset];
        PackedVertex vertex2 = blasVertices[triangle.i2 + blasAccessor.vertexOffset];

        vec2 texCoord0 = unpackHalf2x16(vertex0.texCoord);
        vec2 texCoord1 = unpackHalf2x16(vertex1.texCoord);
        vec2 texCoord2 = unpackHalf2x16(vertex2.texCoord);

        vec3 barycentrics = barycentricsFromUv(intersection.uv);
        vec2 texCoord = texCoord0 * barycentrics.x
            + texCoord1 * barycentrics.y
            + texCoord2 * barycentrics.z;
        
        SampledMaterial material = sampleMaterial(materialDescriptors[blasInstance.materialIdx], texCoord);
        baseColorFactor = diffuseBsdfEval(material.baseColorFactor);

        if (dot(material.emissiveFactor, material.emissiveFactor) > 0.01)
        {
            emissiveContribution += throughput * material.emissiveFactor;
        }

        ambientContribution += throughput * baseColorFactor * ambientFactor;

        vec2 uv = vec2(pixel) / vec2(constants.resolution);
        lightingContribution = texture(sampler2D(denoisedIllumination, linearRepeatSampler), uv).rgb;
        lightingContribution *= throughput * baseColorFactor;
    }
    else
    {
        emissiveContribution += shadeSky(ray.direction);
    }

    vec3 resolved = lightingContribution + ambientContribution + emissiveContribution;
    imageStore(outImage, pixel, vec4(resolved, 1.0));

    vec4 packedAmbientEmissiveBaseColor = vec4(
        uintBitsToFloat(packRgb9e5(ambientContribution).data),
        uintBitsToFloat(packRgb9e5(emissiveContribution).data),
        uintBitsToFloat(packRgb9e5(baseColorFactor).data),
        0.0
    );
    imageStore(ambientEmissiveBaseColor, pixel, packedAmbientEmissiveBaseColor);
}
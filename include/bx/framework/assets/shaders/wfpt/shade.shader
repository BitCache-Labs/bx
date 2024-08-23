#include "[engine]/shaders/Language.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"
#include "[engine]/shaders/wfpt/payload.shader"
#include "[engine]/shaders/random.shader"
#include "[engine]/shaders/sampling.shader"

#define BLAS_DATA_BINDINGS
#include "[engine]/shaders/ray_tracing/blas_data.shader"

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint width;
    uint height;
    uint bounce;
    uint seed;
} constants;

layout (BINDING(0, 1), std430) readonly buffer _Rays
{
    Ray rays[];
};
layout (BINDING(0, 2), std430) writeonly buffer _OutRays
{
    Ray outRays[];
};

layout(BINDING(0, 3), std430) readonly buffer _RayCount
{
    uint rayCount;
};
layout(BINDING(0, 4), std430) buffer _OutRayCount
{
    uint outRayCount;
};

layout (BINDING(0, 5), std430) readonly buffer _PixelMapping
{
    uint pixelMapping[];
};
layout (BINDING(0, 6), std430) writeonly buffer _OutPixelMapping
{
    uint outPixelMapping[];
};

layout (BINDING(0, 7), std430) buffer _Payloads
{
    Payload payloads[];
};

layout (BINDING(0, 8), std430) readonly buffer _Intersections
{
    Intersection intersections[];
};

layout (BINDING(0, 9), std430) writeonly buffer _ShadowRays
{
    Ray shadowRays[];
};
layout(BINDING(0, 10), std430) writeonly buffer _ShadowRayDistances
{
    float shadowRayDistances[];
};
layout(BINDING(0, 11), std430) buffer _ShadowRayCount
{
    uint shadowRayCount;
};
layout(BINDING(0, 12), std430) writeonly buffer _ShadowPixelMapping
{
    uint shadowPixelMapping[];
};

void shootRay(vec3 origin, vec3 direction, uint pid)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;

    uint rayIdx = atomicAdd(outRayCount, 1u);
    outRays[rayIdx] = ray;
    outPixelMapping[rayIdx] = pid;
}

void shootShadowRay(vec3 origin, vec3 direction, float tMax, uint pid)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;

    uint rayIdx = atomicAdd(shadowRayCount, 1u);
    shadowRays[rayIdx] = ray;
    shadowRayDistances[rayIdx] = tMax;
    shadowPixelMapping[rayIdx] = pid;
}

vec3 shadeSky(vec3 direction, vec3 throughput)
{
    float a = (direction.y + 1.0) * 0.5;
    vec3 color = (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);

    return color * throughput;
}

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= rayCount) return;
    uint pid = pixelMapping[id];

    Intersection intersection = intersections[pid];
    Payload payload = payloads[pid];
    Ray ray = rays[id];

    vec3 throughput = payload.throughput;
    vec3 accumulated = payload.accumulated;
    if (constants.bounce == 0)
    {
        accumulated = vec3(0.0);
        throughput = vec3(1.0);
        payload.rngState = pcgHash(pid ^ xorShiftU32(constants.seed));
    }

    if (intersection.t != T_MISS)
    {
        BlasInstance blasInstance = blasInstances[intersection.blasInstanceIdx];
        BlasAccessor blasAccessor = blasAccessors[blasInstance.blasIdx];

        Triangle triangle = blasTriangles[intersection.primitiveIdx + blasAccessor.triangleOffset];
        Vertex vertex0 = blasVertices[triangle.i0 + blasAccessor.vertexOffset];
        Vertex vertex1 = blasVertices[triangle.i1 + blasAccessor.vertexOffset];
        Vertex vertex2 = blasVertices[triangle.i2 + blasAccessor.vertexOffset];

        vec3 barycentrics = barycentricsFromUv(intersection.uv);
        vec3 normal = normalize(vertex0.normal * barycentrics.x
            + vertex1.normal * barycentrics.y
            + vertex2.normal * barycentrics.z);

        mat4 invTransTransform = transpose(blasInstance.invTransform);
        normal = normalize((invTransTransform * vec4(normal, 1.0)).xyz);
        if (!intersection.frontFace)
        {
            normal = -normal;
        }

        vec3 color = normal * 0.5 + 0.5;

        throughput *= color;

        vec3 intersectionPos = ray.origin + ray.direction * intersection.t;

        { // Direct illumination
            vec3 L = normalize(vec3(0.3, 1.0, 0.2));

            vec3 origin = intersectionPos;
            vec3 direction = L;
            origin += direction * RT_EPSILON;

            shootShadowRay(origin, direction, 1000.0, pid);
        }

        { // Indirect illumination
            vec3 origin = intersectionPos;
            vec3 direction = getUniformSphereSample(vec2(
                randomUniformFloat(payload.rngState),
                randomUniformFloat(payload.rngState)
            ));
            origin += direction * RT_EPSILON;
            shootRay(origin, direction, pid);
        }
    }
    else
    {
        accumulated += shadeSky(ray.direction, throughput);
    }

    payload.throughput = throughput;
    payload.accumulated = accumulated;
    payloads[pid] = payload;
}
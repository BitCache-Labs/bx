#include "[engine]/shaders/Language.shader"
#include "[engine]/shaders/extensions/ray_tracing_ext.shader"

#include "[engine]/shaders/ray_tracing/ray.shader"

layout (BINDING(0, 0), std140) uniform Constants
{
    mat4 invView;
    mat4 invProj;
    uint width;
    uint height;
};
layout(BINDING(0, 1)) uniform accelerationStructureEXT Scene;
layout (BINDING(0, 2)), rgba8) uniform image2D OutImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dispatchSize = ivec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    if (id.x >= Constants.width || id.y >= Constants.height) return;

    vec2 pixelCenter = vec2(id.x + 0.5, id.y + 0.5);
    vec2 uv = (pixelCenter / vec2(Constants.width, Constants.height)) * 2.0 - 1.0;
    uv.y = -uv.y; // TODO: required??
    vec4 origin = Constants.invView * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 target = Constants.invProj * vec4(uv, 1.0, 1.0);
    vec4 direction = Constants.invView * vec4(normalize(target.xyz), 0.0);

    Ray ray;
    ray.origin = origin.xyz;
    ray.direction = direction.xyz;

    rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, Scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ray.origin, 0.01, ray.direction, 1000.0);
	rayQueryProceedEXT(rayQuery);

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
		imageStore(OutImage, id, vec4(1.0, 0.0, 1.0, 1.0));
	}
    else
    {
        imageStore(OutImage, id, vec4(0.0, 0.0, 1.0, 1.0));
    }
}
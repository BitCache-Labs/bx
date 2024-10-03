#include "[engine]/shaders/Language.shader"

#define BLAS_DATA_BINDINGS

#include "[engine]/shaders/ray_tracing/blas_data.shader"

const uint MAX_BONES = 100;

layout (BINDING(0, 0), std140) uniform _Constants
{
    uint vertexCount;
    uint blasIdx;
    uint originalBlasIdx;
    uint _PADDING1;
} constants;

layout (BINDING(0, 1), std140) uniform _Bones
{
    mat4 bones[MAX_BONES];
};

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint id = uint(gl_GlobalInvocationID.x);
    if (id >= constants.vertexCount) return;

    uint inVertexId = blasAccessors[constants.originalBlasIdx].vertexOffset + id;
    uint outVertexId = blasAccessors[constants.blasIdx].vertexOffset + id;

    PackedVertex packedVertex = blasVertices[inVertexId];
    uvec4 boneIndices = unpack4xU8(packedVertex.bones);
    vec4 weights = packedVertex.weights;
    
    mat4 matrix = mat4(0);
    for (uint i = 0; i < 4; i++)
    {
        matrix += weights[i] * bones[boneIndices[i]];
    }
    packedVertex.position = (matrix * vec4(packedVertex.position, 1.0)).xyz;
    
    blasVertices[outVertexId] = packedVertex;
}
#include "bx/editor/core/asset_importer.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/list.hpp>
#include <bx/engine/containers/hash_map.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <bx/framework/resources/animation.hpp>
#include <bx/framework/resources/material.hpp>
#include <bx/framework/resources/mesh.hpp>
#include <bx/framework/resources/shader.hpp>
#include <bx/framework/resources/skeleton.hpp>
#include <bx/framework/resources/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstring>
#include <fstream>
#include <sstream>

template <typename T>
static Resource<T> ImportResource(const String& filename, const T& data)
{
    auto res = Resource<T>(Resource<T>::MakeHandle(filename), data);
    res.Save(filename);
    return res;
}

bool AssetImporter::ImportTexture(const String& filename)
{
    Texture texture;

    stbi_set_flip_vertically_on_load(true);
    auto pData = stbi_load(File::GetPath(filename).c_str(), &texture.width, &texture.height, &texture.channels, 4);
    if (pData == nullptr)
        return false;

    texture.channels = 4;
    texture.depth = 1;

    texture.pixels.resize((i64)texture.width * (i64)texture.height * 4);
    memcpy(texture.pixels.data(), pData, texture.pixels.size());
    stbi_image_free(pData);

    auto res = ImportResource(File::RemoveExt(filename) + ".texture", texture);
    return res.IsValid();
}

template <typename TData>
struct ModelDataWrapper
{
    String name;
    TData data;
};

struct ModelData
{
    List<ModelDataWrapper<Mesh>> meshes;
    List<ModelDataWrapper<Texture>> textures;
    List<ModelDataWrapper<Material>> materials;
    List<ModelDataWrapper<Skeleton>> skeletons;
    List<ModelDataWrapper<Animation>> animations;
};

static Vec3 AssimpVec3(const aiVector3D& v)
{
    return Vec3(v.x, v.y, v.z);
}

static Quat AssimpQuat(const aiQuaternion& q)
{
    return Quat(q.x, q.y, q.z, q.w);
}

static Mat4 AssimpMat4(const aiMatrix4x4& m)
{
    Mat4 mm = Mat4::Identity();
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < 4; ++j)
            mm(i, j) = m[j][i];
    return mm;
}

static void AssimpLoadTexture(const aiTexture* pTexture, ModelData& data)
{
    Texture tex;

    stbi_set_flip_vertically_on_load(true);
    auto pData = stbi_load_from_memory((stbi_uc*)pTexture->pcData, pTexture->mWidth * sizeof(aiTexel), &tex.width, &tex.height, &tex.channels, 4);
    if (pData == nullptr)
        return;

    tex.channels = 4;
    tex.pixels.resize((i64)tex.width * (i64)tex.height * 4);
    memcpy(tex.pixels.data(), pData, tex.pixels.size());
    stbi_image_free(pData);

    ModelDataWrapper<Texture> entry;
    entry.name = String("_") + pTexture->mFilename.C_Str();
    entry.data = tex;
    data.textures.emplace_back(entry);
}

static void AssimpLoadMaterial(const aiMaterial* pMaterial, ModelData& data)
{
    for (SizeType j = 0; j < pMaterial->mNumProperties; ++j)
    {
        const auto pProperty = pMaterial->mProperties[j];
        pProperty->mType;
    }

    Material mat;

    ModelDataWrapper<Material> entry;
    entry.name = "_" + String(pMaterial->GetName().C_Str());
    entry.data = mat;
    data.materials.emplace_back(entry);
}

static void AssimpReadHeirarchy(
    HashMap<String, SizeType>& boneMap,
    List<Skeleton::Bone>& bones,
    Tree<String>& boneTree, TreeNodeId nodeId,
    const aiNode* pNode)
{
    String boneName = pNode->mName.C_Str();
    auto it = boneMap.find(boneName);
    if (it == boneMap.end())
    {
        Skeleton::Bone bone;
        bone.local = AssimpMat4(pNode->mTransformation);

        bones.emplace_back(bone);
        boneMap[boneName] = bones.size() - 1;
    }
    else
    {
        auto& bone = bones[it->second];
        bone.local = AssimpMat4(pNode->mTransformation);
    }

    for (SizeType c = 0; c < pNode->mNumChildren; ++c)
    {
        const auto pChild = pNode->mChildren[c];
        
        TreeNodeId childId = boneTree.CreateNode(pChild->mName.C_Str());
        boneTree.AddChild(nodeId, childId);
        AssimpReadHeirarchy(boneMap, bones, boneTree, childId, pChild);
    }
}

static void AssimpLoadSkeleton(const aiScene* pScene, ModelData& data)
{
    List<Skeleton::Bone> bones;
    HashMap<String, SizeType> boneMap;
    Tree<String> boneTree;

    for (u32 m = 0; m < pScene->mNumMeshes; ++m)
    {
        auto pMesh = pScene->mMeshes[m];

        // Extract bone data
        if (pMesh->HasBones())
        {
            for (u32 b = 0; b < pMesh->mNumBones; ++b)
            {
                auto pBone = pMesh->mBones[b];

                String boneName = pBone->mName.C_Str();
                if (boneMap.find(boneName) == boneMap.end())
                {
                    Skeleton::Bone bone;
                    bone.offset = AssimpMat4(pBone->mOffsetMatrix);

                    bones.emplace_back(bone);
                    boneMap[boneName] = bones.size() - 1;
                }
            }
        }
    }

    if (bones.empty())
        return;

    auto pRoot = pScene->mRootNode;
    auto rootId = boneTree.CreateNode(pRoot->mName.C_Str());
    AssimpReadHeirarchy(boneMap, bones, boneTree, rootId, pRoot);

    Skeleton skel(bones, boneMap, boneTree);

    ModelDataWrapper<Skeleton> entry;
    entry.name = "";
    entry.data = skel;
    data.skeletons.emplace_back(entry);
}

static void AssimpLoadMesh(const aiScene* pScene, const aiNode* pNode, ModelData& data)
{
    for (SizeType m = 0; m < pNode->mNumMeshes; ++m)
    {
        auto pMesh = pScene->mMeshes[pNode->mMeshes[m]];

        List<Vec3> vertices;
        List<Vec4> colors;
        List<Vec3> normals;
        List<Vec3> tangents;
        List<Vec2> uvs;
        List<Vec4i> bones;
        List<Vec4> weights;

        List<u32> triangles;

        // Extract vertex data
        vertices.resize(pMesh->mNumVertices);
        colors.resize(pMesh->mNumVertices);
        normals.resize(pMesh->mNumVertices);
        tangents.resize(pMesh->mNumVertices);
        uvs.resize(pMesh->mNumVertices);
        bones.resize(pMesh->mNumVertices);
        weights.resize(pMesh->mNumVertices);

        for (SizeType v = 0; v < pMesh->mNumVertices; ++v)
        {
            auto& vert = vertices[v];

            const auto& p = pMesh->mVertices[v];
            vert = Vec3(p.x, p.y, p.z);

            auto& norm = normals[v];
            norm = Vec3(0, 0, 0);
            if (pMesh->HasNormals())
            {
                const auto& n = pMesh->mNormals[v];
                norm = Vec3(n.x, n.y, n.z);
            }

            auto& tang = tangents[v];
            tang = Vec3(0, 0, 0);
            if (pMesh->HasTangentsAndBitangents())
            {
                const auto& t = pMesh->mTangents[v];
                tang = Vec3(t.x, t.y, t.z);
            }

            auto& uv = uvs[v];
            uv = Vec2(0, 0);
            if (pMesh->HasTextureCoords(0))
            {
                const auto& tc = pMesh->mTextureCoords[0][v];
                uv = Vec2(tc.x, tc.y);
            }

            auto& color = colors[v];
            color = Vec4(1, 1, 1, 1);
            if (pMesh->HasVertexColors(0))
            {
                const auto& c = pMesh->mColors[0][v];
                color = Vec4(c.r, c.g, c.b, c.a);
            }

            auto& bone = bones[v];
            bone = Vec4i(-1, -1, -1, -1);

            auto& weight = weights[v];
            weight = Vec4(0, 0, 0, 0);
        }

        // Extract face data
        triangles.resize(pMesh->mNumFaces * 3LL);

        for (SizeType f = 0; f < pMesh->mNumFaces; ++f)
        {
            const auto& face = pMesh->mFaces[f];
            BX_ASSERT(face.mNumIndices == 3, "Only triangles are supported!");

            SizeType idx = f * 3;
            triangles[idx + 0] = face.mIndices[0];
            triangles[idx + 1] = face.mIndices[1];
            triangles[idx + 2] = face.mIndices[2];
        }

        // Extract bone data
        if (pMesh->HasBones())
        {
            // TODO: Handle models with more than one skeleton
            BX_ENSURE(data.skeletons.size() > 0);
            auto& skeleton = data.skeletons[0].data;

            u32 boneCount = 0;
            for (u32 b = 0; b < pMesh->mNumBones; ++b)
            {
                const auto pBone = pMesh->mBones[b];

                String boneName = pBone->mName.C_Str();
                auto it = skeleton.GetBoneMap().find(boneName);
                BX_ENSURE(it != skeleton.GetBoneMap().end());

                u32 boneIdx = (u32)it->second;
                for (u32 w = 0; w < pBone->mNumWeights; ++w)
                {
                    auto pWeight = pBone->mWeights[w];
                    u32 vertexId = pWeight.mVertexId;

                    BX_ENSURE(vertexId <= vertices.size());

                    auto& bone = bones[vertexId];
                    auto& weight = weights[vertexId];
                    u8 minIdx = 0;
                    f32 minWeight = weight[minIdx];

                    for (u8 i = 1; i < SKELETON_MAX_BONES; ++i)
                    {
                        if (weight[i] < minWeight)
                        {
                            minWeight = weight[i];
                            minIdx = i;
                        }
                    }

                    if (pWeight.mWeight > minWeight)
                    {
                        weight[minIdx] = pWeight.mWeight;
                        bone[minIdx] = boneIdx;
                    }
                }
            }
        }

        Mat4 transform = AssimpMat4(pNode->mTransformation);
        Mesh mesh(transform, vertices, colors, normals, tangents, uvs, bones, weights, triangles);

        ModelDataWrapper<Mesh> entry;
        entry.name = String("_") + pNode->mName.C_Str();
        entry.data = mesh;
        data.meshes.emplace_back(entry);
    }

    for (SizeType c = 0; c < pNode->mNumChildren; ++c)
    {
        const auto pChild = pNode->mChildren[c];
        AssimpLoadMesh(pScene, pChild, data);
    }
}

static void AssimpLoadAnimation(const aiAnimation* pAnimation, ModelData& data)
{
    String animationName = pAnimation->mName.C_Str();

    HashMap<String, Animation::Keyframes> channels;
    for (SizeType c = 0; c < pAnimation->mNumChannels; ++c)
    {
        auto pChannel = pAnimation->mChannels[c];
        String channelName = pChannel->mNodeName.C_Str();

        Animation::Keyframes keyframes;
        
        for (SizeType p = 0; p < pChannel->mNumPositionKeys; ++p)
        {
            const auto& k = pChannel->mPositionKeys[p];
            Animation::PositionKey key;
            key.position = AssimpVec3(k.mValue);
            key.timeStamp = (f32)k.mTime;
            keyframes.positionKeys.emplace_back(key);
        }

        for (SizeType r = 0; r < pChannel->mNumRotationKeys; ++r)
        {
            const auto& k = pChannel->mRotationKeys[r];
            Animation::RotationKey key;
            key.rotation = AssimpQuat(k.mValue);
            key.timeStamp = (f32)k.mTime;
            keyframes.rotationKeys.emplace_back(key);
        }

        for (SizeType s = 0; s < pChannel->mNumScalingKeys; ++s)
        {
            const auto& k = pChannel->mScalingKeys[s];
            Animation::ScaleKey key;
            key.scale = AssimpVec3(k.mValue);
            key.timeStamp = (f32)k.mTime;
            keyframes.scaleKeys.emplace_back(key);
        }

        channels.insert(std::make_pair(channelName, keyframes));
    }

    f32 duration = (f32)pAnimation->mDuration;
    f32 tps = (f32)pAnimation->mTicksPerSecond;

    Animation anim(animationName, duration, tps, channels);

    ModelDataWrapper<Animation> entry;
    entry.name = String("_") + pAnimation->mName.C_Str();
    entry.data = anim;
    data.animations.emplace_back(entry);
}

bool AssetImporter::ImportModel(const String& filename)
{
    // Create an instance of the Importer class
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65536);

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* pScene = importer.ReadFile(File::GetPath(filename),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_SplitLargeMeshes |
        //aiProcess_ValidateDataStructure |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_OptimizeMeshes |
        //aiProcess_PopulateArmatureData |
        //aiProcess_OptimizeGraph |
        aiProcess_SortByPType);

    // If the import failed, report it
    if (pScene == nullptr)
    {
        BX_LOGE("Failed to import model: {} - Message: {}", filename, importer.GetErrorString());
        return false;
    }

    ModelData data{};
    
    // Extract textures
    if (pScene->HasTextures())
    {
        data.textures.reserve(pScene->mNumTextures);
        for (SizeType i = 0; i < pScene->mNumTextures; ++i)
        {
            const auto pTexture = pScene->mTextures[i];
            AssimpLoadTexture(pTexture, data);
        }

        for (SizeType i = 0; i < data.textures.size(); ++i)
        {
            String name = File::RemoveExt(filename) + data.textures[i].name + "_" + std::to_string(i) + ".texture";
            auto res = ImportResource(name, data.textures[i].data);
        }
    }

    // Extract materials
    if (pScene->HasMaterials())
    {
        data.materials.reserve(pScene->mNumMaterials);
        for (SizeType i = 0; i < pScene->mNumMaterials; ++i)
        {
            const auto pMaterial = pScene->mMaterials[i];
            AssimpLoadMaterial(pMaterial, data);
        }

        for (SizeType i = 0; i < data.materials.size(); ++i)
        {
            String name = File::RemoveExt(filename) + data.materials[i].name + "_" + std::to_string(i) + ".material";
            auto res = ImportResource(name, data.materials[i].data);
        }
    }

    // Extract skeletons
    //if (pScene->hasSkeletons())
    {
        //data.skeletons.reserve(pScene->mNumSkeletons);
        //for (SizeType i = 0; i < pScene->mNumSkeletons; ++i)
        {
            //const auto pSkeleton = pScene->mSkeletons[i];
            AssimpLoadSkeleton(pScene, data);
        }

        for (SizeType i = 0; i < data.skeletons.size(); ++i)
        {
            String name = File::RemoveExt(filename) + data.skeletons[i].name + "_" + std::to_string(i) + ".skeleton";
            auto res = ImportResource(name, data.skeletons[i].data);
        }
    }

    // Extract meshes
    //if (pScene->HasMeshes())
    {
        //data.meshes.reserve(pScene->mNumMeshes);
        //for (SizeType i = 0; i < pScene->mNumMeshes; ++i)
        {
            //const auto pMesh = pScene->mMeshes[i];
            AssimpLoadMesh(pScene, pScene->mRootNode, data);
        }
    
        for (SizeType i = 0; i < data.meshes.size(); ++i)
        {
            String name = File::RemoveExt(filename) + data.meshes[i].name + "_" + std::to_string(i) + ".mesh";
            auto res = ImportResource(name, data.meshes[i].data);
        }
    }

    // Extract animations
    if (pScene->HasAnimations())
    {
        data.skeletons.reserve(pScene->mNumAnimations);
        for (SizeType i = 0; i < pScene->mNumAnimations; ++i)
        {
            const auto pAnimation = pScene->mAnimations[i];
            AssimpLoadAnimation(pAnimation, data);
        }

        for (SizeType i = 0; i < data.animations.size(); ++i)
        {
            String name = File::RemoveExt(filename) + data.animations[i].name + "_" + std::to_string(i) + ".animation";
            auto res = ImportResource(name, data.animations[i].data);
        }
    }

    return true;
}
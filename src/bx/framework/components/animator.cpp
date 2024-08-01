#include "bx/framework/components/animator.hpp"
#include "bx/framework/components/animator.serial.hpp"

#include <bx/engine/core/time.hpp>

#define ENABLE_INTERPOLATION

Animator::Animator()
{
    BufferCreateInfo createInfo{};
    createInfo.name = "Animator Bones Buffer";
    createInfo.size = sizeof(Mat4) * 100;
    createInfo.usageFlags = BufferUsageFlags::UNIFORM | BufferUsageFlags::STORAGE;
    m_boneBuffer = Graphics::CreateBuffer(createInfo);
}

void Animator::OnRemoved()
{
    Graphics::DestroyBuffer(m_boneBuffer);
}

static SizeType GetPositionIndex(const Animation::Keyframes& keys, f32 time)
{
    for (SizeType index = 0; index < keys.positionKeys.size() - 1; ++index)
    {
        if (time < keys.positionKeys[index + 1].timeStamp)
            return index;
    }
    return keys.positionKeys.size() - 2;
}

static SizeType GetRotationIndex(const Animation::Keyframes& keys, f32 time)
{
    for (SizeType index = 0; index < keys.rotationKeys.size() - 1; ++index)
    {
        if (time < keys.rotationKeys[index + 1].timeStamp)
            return index;
    }
    return keys.rotationKeys.size() - 2;
}

static SizeType GetScaleIndex(const Animation::Keyframes& keys, f32 time)
{
    for (SizeType index = 0; index < keys.scaleKeys.size() - 1; ++index)
    {
        if (time < keys.scaleKeys[index + 1].timeStamp)
            return index;
    }
    return keys.scaleKeys.size() - 2;
}

static f32 GetScaleFactor(f32 time, f32 lastTimeStamp, f32 nextTimeStamp)
{
    f32 scaleFactor = 0.0f;
    f32 midWayLength = time - lastTimeStamp;
    f32 framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

Mat4 InterpolatePosition(const Animation::Keyframes& keys, f32 time)
{
    if (keys.positionKeys.size() == 1)
        return Mat4::Translation(keys.positionKeys[0].position);

    SizeType p0Index = GetPositionIndex(keys, time);
    SizeType p1Index = p0Index + 1;

    f32 scaleFactor = GetScaleFactor(
        time,
        keys.positionKeys[p0Index].timeStamp,
        keys.positionKeys[p1Index].timeStamp);

#ifdef ENABLE_INTERPOLATION
    Vec3 finalPosition = Vec3::Lerp(
        keys.positionKeys[p0Index].position,
        keys.positionKeys[p1Index].position,
        scaleFactor);
#else
    Vec3 finalPosition = keys.positionKeys[p0Index].position;
#endif

    return Mat4::Translation(finalPosition);
}

Mat4 InterpolateRotation(const Animation::Keyframes& keys, f32 time)
{
    if (keys.rotationKeys.size() == 1)
        return Mat4::Rotation(keys.rotationKeys[0].rotation);

    SizeType p0Index = GetRotationIndex(keys, time);
    SizeType p1Index = p0Index + 1;
    f32 scaleFactor = GetScaleFactor(
        time,
        keys.rotationKeys[p0Index].timeStamp,
        keys.rotationKeys[p1Index].timeStamp);

#ifdef ENABLE_INTERPOLATION
    Quat finalRotation = Quat::Slerp(
        keys.rotationKeys[p0Index].rotation,
        keys.rotationKeys[p1Index].rotation,
        scaleFactor);
#else
    Quat finalRotation = keys.rotationKeys[p0Index].rotation;
#endif
    
    return Mat4::Rotation(finalRotation.Normalized());
}

Mat4 InterpolateScale(const Animation::Keyframes& keys, f32 time)
{
    if (keys.scaleKeys.size() == 1)
        return Mat4::Scale(keys.scaleKeys[0].scale);

    SizeType p0Index = GetScaleIndex(keys, time);
    SizeType p1Index = p0Index + 1;
    f32 scaleFactor = GetScaleFactor(
        time,
        keys.scaleKeys[p0Index].timeStamp,
        keys.scaleKeys[p1Index].timeStamp);

#ifdef ENABLE_INTERPOLATION
    Vec3 finalScale = Vec3::Lerp(
        keys.scaleKeys[p0Index].scale,
        keys.scaleKeys[p1Index].scale,
        scaleFactor);
#else
    Vec3 finalScale = keys.scaleKeys[p0Index].scale;
#endif

    return Mat4::Scale(finalScale);
}

static Mat4 ComputeBoneTransform(const Animation& anim, const String& boneName, const Skeleton::Bone& bone, f32 time)
{
    auto it = anim.GetChannels().find(boneName);
    if (it == anim.GetChannels().end())
        return bone.local;

    const auto& keyframes = it->second;
    Mat4 translation = InterpolatePosition(keyframes, time);
    Mat4 rotation = InterpolateRotation(keyframes, time);
    Mat4 scale = InterpolateScale(keyframes, time);
    return translation * rotation * scale;
}

void Animator::Update()
{
    // Reset to identity
    for (auto& mat : m_boneMatrices)
        mat = Mat4::Identity();
    for (auto& mat : m_boneMatrices2)
        mat = Mat4::Identity();

    // Validate state
    if (!m_skeleton || m_current == -1)
        return;

    const auto& anim = GetAnimation(m_current);
    if (!anim) return;

    // Update animation data
    const auto& animData = anim.GetData();
    m_time += m_speed * animData.GetTicksPerSecond() * Time::GetDeltaTime();
    m_time = m_looping ? std::fmod(m_time, animData.GetDuration()) : Math::Min(m_time, animData.GetDuration());
    
    if (!m_looping && m_time >= animData.GetDuration())
    {
        m_hasEnded = true;
    }

    const auto& skelData = m_skeleton.GetData();
    const auto& boneTree = skelData.GetBoneTree();
    const auto& boneMap = skelData.GetBoneMap();
    const auto& bones = skelData.GetBones();

    // Compute hierarchy and animation transforms
    boneTree.RecurseParam<Mat4>(Mat4::Identity(), boneTree.GetRoot(),
        [&](const Mat4& parentMatrix, TreeNodeId nodeId, const TreeNode<String>& node)
        {
            auto nodeBoneIdx = boneMap.find(node.data)->second;
            const auto& bone = bones[nodeBoneIdx];
            Mat4 boneMatrix = ComputeBoneTransform(animData, node.data, bone, m_time);

            Mat4 worldMatrix = parentMatrix * boneMatrix;

            Mat4 finalBoneMatrix = worldMatrix * bone.offset;
            m_boneMatrices[nodeBoneIdx] = finalBoneMatrix;
            m_boneMatrices2[nodeBoneIdx] = worldMatrix;

            return worldMatrix;
        });

    Graphics::WriteBuffer(m_boneBuffer, 0, GetBoneMatrices().data(), GetBoneMatrices().size() * sizeof(Mat4));
}
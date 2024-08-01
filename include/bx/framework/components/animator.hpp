#pragma once

#include "bx/framework/resources/animation.hpp"
#include "bx/framework/resources/skeleton.hpp"

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/modules/graphics.hpp>

class Animator : public Component<Animator>
{
public:
	Animator();

	inline void OnPostCopy() override
	{
		RefreshBones();
		RefreshCurrent();
	}

	void OnRemoved() override;
	
	inline const Resource<Skeleton>& GetSkeleton() const { return m_skeleton; }
	inline void SetSkeleton(const Resource<Skeleton>& skeleton)
	{
		m_skeleton = skeleton;
		RefreshBones();
	}

	inline const List<Resource<Animation>>& GetAnimations() const { return m_animations; }
	inline SizeType GetAnimationCount() const { return m_animations.size(); }

	inline void AddAnimation(const Resource<Animation>& anim)
	{
		m_animations.emplace_back(anim);
		RefreshCurrent();
	}

	inline const Resource<Animation>& GetAnimation(SizeType index) const
	{
		BX_ENSURE(index < m_animations.size());
		return m_animations[index];
	}

	inline void SetAnimation(SizeType index, const Resource<Animation>& anim)
	{
		BX_ENSURE(index < m_animations.size());
		m_animations[index] = anim;
		RefreshCurrent();
	}

	inline void RemoveAnimation(SizeType index)
	{
		BX_ENSURE(index < m_animations.size());
		m_animations.erase(m_animations.begin() + index);
		RefreshCurrent();
	}

	inline SizeType GetCurrent() const { return m_current; }
	inline void SetCurrent(SizeType current)
	{
		if (m_current == current)
			return;

		m_current = current;
		m_time = 0.f;
		m_hasEnded = false;
	}

	inline bool HasEnded() const { return m_hasEnded; }
	inline f32 GetDuration(SizeType index) const
	{
		const auto& anim = GetAnimation(index);
		if (!anim) return 0.f;
		const auto& animData = anim.GetData();
		return animData.GetDuration() / animData.GetTicksPerSecond();
	}

	inline f32 GetSpeed() const { return m_speed; }
	inline void SetSpeed(f32 speed) { m_speed = speed; }

	inline bool GetLooping() const { return m_looping; }
	inline void SetLooping(bool looping) { m_looping = looping; }

	inline Mat4 GetBoneMatrix(const String& name) const
	{
		const auto& skelData = m_skeleton.GetData();
		auto it = skelData.GetBoneMap().find(name);
		if (it == skelData.GetBoneMap().end())
		{
			return Mat4::Identity();
		}
		return m_boneMatrices2[it->second];
	}

	inline const List<Mat4>& GetBoneMatrices() const
	{
		return m_boneMatrices;
	}

	inline BufferHandle GetBoneBuffer() const
	{
		return m_boneBuffer;
	}

	void Update();

private:
	inline void RefreshBones()
	{
		m_boneMatrices.clear();
		m_boneMatrices2.clear();
		if (!m_skeleton)
			return;

		m_boneMatrices.resize(m_skeleton->GetBones().size());
		m_boneMatrices2.resize(m_skeleton->GetBones().size());
	}

	inline void RefreshCurrent()
	{
		if (m_current == -1 && !m_animations.empty())
		{
			m_current = 0;
			return;
		}

		if (m_current >= m_animations.size())
		{
			m_current = -1;
			return;
		}
	}

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	Resource<Skeleton> m_skeleton;
	List<Resource<Animation>> m_animations;

	f32 m_time = 0;
	SizeType m_current = -1;
	f32 m_speed = 1.0f;
	bool m_looping = true;
	bool m_hasEnded = false;

	List<Mat4> m_boneMatrices;
	List<Mat4> m_boneMatrices2;

	BufferHandle m_boneBuffer;
};
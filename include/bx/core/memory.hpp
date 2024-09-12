#pragma once

#include "bx/engine/core/byte_types.hpp"

#include <new>
#include <cstddef>
#include <cstdlib>
#include <utility>

#ifdef MEMORY_CUSTOM_CONTAINERS

class Allocator
{
public:
	virtual void* Allocate(SizeType size, SizeType align) = 0;
	virtual void Deallocate(void* ptr) = 0;
	virtual SizeType AllocatedSize(void* ptr) = 0;

	template <typename TObj, typename... Args>
	TObj* New(Args&&... args)
	{
		void* pMem = Allocate(sizeof(TObj), alignof(TObj));
		TObj* pObj = new (pMem) TObj(std::forward<Args>(args)...);
		return pObj;
	}

	template <typename TObj>
	void Delete(TObj* pObj)
	{
		if (pObj != nullptr)
		{
			Deallocate(pObj);
			pObj->~TObj();
		}
	}

	template <typename TObj>
	TObj* NewArray(SizeType count)
	{
		const SizeType allocSize = sizeof(SizeType) + sizeof(TObj) * count;
		void* pMem = Allocate(allocSize, alignof(TObj));
		
		auto& arraySize = *(reinterpret_cast<SizeType*>(pMem));
		arraySize = count;
		
		pMem = PtrUtils::Add(pMem, sizeof(SizeType));

		TObj* pObj = new (pMem) TObj[count];
		return pObj;
	}

	template <typename TObj>
	void DeleteArray(TObj* pObj)
	{
		if (pObj != nullptr)
		{
			void* pMem = PtrUtils::Subtract(pObj, sizeof(SizeType));
			
			auto& arraySize = *(reinterpret_cast<SizeType*>(pMem));
			for (SizeType i = 0; i < arraySize; ++i)
				(pObj + i)->~TObj();

			Deallocate(pMem);
		}
	}
};

class Memory
{
public:
	static void Initialize();
	static void Shutdown();

	static Allocator& DefaultAllocator();

	static void Copy(void* pOld, void* pNew, SizeType size);

	static void* Allocate(SizeType size, SizeType align)
	{
		return DefaultAllocator().Allocate(size, align);
	}

	static void Deallocate(void* ptr)
	{
		DefaultAllocator().Deallocate(ptr);
	}

	static SizeType AllocatedSize(void* ptr)
	{
		return DefaultAllocator().AllocatedSize(ptr);
	}

	template <typename TObj, typename... Args>
	TObj* New(Args&&... args)
	{
		return DefaultAllocator().New(std::forward(args)...);
	}

	template <typename TObj>
	static void Delete(TObj* pObj)
	{
		DefaultAllocator().Delete(pObj);
	}

	template <typename TObj>
	static TObj* NewArray(SizeType count)
	{
		return DefaultAllocator().NewArray(count);
	}

	template <typename TObj>
	static void DeleteArray(TObj* pObj)
	{
		DefaultAllocator().DeleteArray(pObj);
	}
};

class PtrUtils
{
public:
	static std::ptrdiff_t AlignForward(uintptr_t address, u8 alignment);
	static const void* AlignForward(const void* address, u8 alignment);
	static void* AlignBackward(void* address, u8 alignment);
	static const void* AlignBackward(const void* address, u8 alignment);
	static u8 AlignForwardAdjustment(const void* address, u8 alignment);
	static u8 AlignForwardAdjustmentWithHeader(const void* address, u8 alignment, u8 headerSize);
	static u8 AlignBackwardAdjustment(const void* address, u8 alignment);

	static void* Add(void* p, u64 x);
	static const void* Add(const void* p, u64 x);
	static void* Subtract(void* p, u64 x);
	static const void* Subtract(const void* p, u64 x);
};

class HeapAllocator : public Allocator
{
public:
	HeapAllocator();
	~HeapAllocator();

	void* Allocate(SizeType size, SizeType align) override;
	void Deallocate(void* ptr) override;
	SizeType AllocatedSize(void* ptr) override;

private:
	SizeType m_allocated = 0;
};

#endif
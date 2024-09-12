#ifdef MEMORY_CUSTOM_CONTAINERS

#include "Engine/Core/Memory.hpp"
#include "Engine/Core/Log.hpp"
#include "Engine/Core/Macros.hpp"

#include <iostream>

void* operator new(SizeType size)//, SizeType align)
{
	if (size == 0)
		return nullptr;

    return Memory::DefaultAllocator().Allocate(size, alignof(u8));
}

void operator delete(void* ptr)
{
	if (ptr == nullptr)
		return;

	Memory::DefaultAllocator().Deallocate(ptr);
}

void Memory::Initialize()
{
}

void Memory::Shutdown()
{
}

Allocator& Memory::DefaultAllocator()
{
	static HeapAllocator s_defaultAllocator;
	return s_defaultAllocator;
}

void Memory::Copy(void* pOld, void* pNew, SizeType size)
{
	std::memcpy(pNew, pOld, size);
}

std::ptrdiff_t PtrUtils::AlignForward(uintptr_t address, u8 alignment)
{
	ENGINE_ENSURE(alignment >= 1);
	ENGINE_ENSURE(alignment <= 128);
	ENGINE_ENSURE((alignment & (alignment - 1)) == 0); // Must be pwr of 2!

	// u8 mask = (alignment-1);
	// uptr misalignment = (address & mask);
	std::ptrdiff_t adjustment = alignment - (address & (alignment - 1));
	ENGINE_ENSURE(adjustment < 256);
	return adjustment;
}

const void* PtrUtils::AlignForward(const void* address, u8 alignment)
{
	return (void*)((reinterpret_cast<uintptr_t>(address) + static_cast<uintptr_t>(alignment - 1)) & static_cast<uintptr_t>(~(alignment - 1)));
}

void* PtrUtils::AlignBackward(void* address, u8 alignment)
{
	return (void*)(reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(~(alignment - 1)));
}

const void* PtrUtils::AlignBackward(const void* address, u8 alignment)
{
	return (void*)(reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(~(alignment - 1)));
}

u8 PtrUtils::AlignForwardAdjustment(const void* address, u8 alignment)
{
	u8 adjustment = alignment - (reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(alignment - 1));

	if (adjustment == alignment)
		return 0; // already aligned

	return adjustment;
}

u8 PtrUtils::AlignForwardAdjustmentWithHeader(const void* address, u8 alignment, u8 headerSize)
{
	u8 adjustment = AlignForwardAdjustment(address, alignment);

	u8 neededSpace = headerSize;

	if (adjustment < neededSpace) {
		neededSpace -= adjustment;

		// Increase adjustment to fit header
		adjustment += alignment * (neededSpace / alignment);

		if (neededSpace % alignment > 0)
			adjustment += alignment;
	}

	return adjustment;
}

u8 PtrUtils::AlignBackwardAdjustment(const void* address, u8 alignment)
{
	u8 adjustment = reinterpret_cast<uintptr_t>(address) & static_cast<uintptr_t>(alignment - 1);

	if (adjustment == alignment)
		return 0; // already aligned

	return adjustment;
}

void* PtrUtils::Add(void* p, u64 x)
{
	return (void*)(reinterpret_cast<uintptr_t>(p) + x);
}

const void* PtrUtils::Add(const void* p, u64 x)
{
	return (const void*)(reinterpret_cast<uintptr_t>(p) + x);
}

void* PtrUtils::Subtract(void* p, u64 x)
{
	return (void*)(reinterpret_cast<uintptr_t>(p) - x);
}

const void* PtrUtils::Subtract(const void* p, u64 x)
{
	return (const void*)(reinterpret_cast<uintptr_t>(p) - x);
}

HeapAllocator::HeapAllocator()
{
}

HeapAllocator::~HeapAllocator()
{
	//ENGINE_ASSERT(m_allocated == 0);
}

struct HeapHeader
{
	u32 magic = 0xFFFFFFFF;
	SizeType allocSize = 0;
	SizeType count = 0;
};

void* HeapAllocator::Allocate(SizeType size, SizeType align)
{
	ENGINE_ASSERT(size != 0, "Allocating zero bytes!");

	const SizeType allocSize = sizeof(HeapHeader) + size;
	m_allocated += allocSize;
	
	void* ptr = malloc(allocSize);
	ENGINE_ASSERT(ptr != nullptr, "Failed to allocate memory!");
	
	auto& header = *(reinterpret_cast<HeapHeader*>(ptr));
	header.allocSize = size;

	return PtrUtils::Add(ptr, sizeof(HeapHeader));
}

void HeapAllocator::Deallocate(void* ptr)
{
	ENGINE_ASSERT(ptr != nullptr, "Deleting a nullptr!");

	const SizeType allocSize = sizeof(HeapHeader) + AllocatedSize(ptr);
	ENGINE_ASSERT(m_allocated >= allocSize, "Deallocating more memory than allocated!");
	m_allocated -= allocSize;

	ptr = PtrUtils::Subtract(ptr, sizeof(HeapHeader));
	free(ptr);
}

SizeType HeapAllocator::AllocatedSize(void* ptr)
{
	ENGINE_ASSERT(ptr != nullptr, "Pointer is null!");
	
	ptr = PtrUtils::Subtract(ptr, sizeof(HeapHeader));
	const auto& header = *(reinterpret_cast<HeapHeader*>(ptr));
	return header.allocSize;
}

#endif
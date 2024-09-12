#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/containers/list.hpp"

// TODO: THIS WAS MADE FOR THE ECS IT'S OLD
// DON'T USE FOR OTHER STUFF UNTIL IT'S READY!

class IPool
{
public:
    virtual ~IPool() {}

    virtual bool IsUsed(SizeType idx) const = 0;
    virtual SizeType GetFreeIndex() = 0;
    virtual SizeType GetSize() const = 0;
    virtual void Remove(SizeType idx) = 0;
    virtual void* GetPtr(SizeType idx) const = 0;
    virtual void Clear() = 0;
};

// TODO: Add custom iterator to iterate over only valid slots
template <typename TData>
class Pool : public IPool
{
public:
    Pool() : Pool(TData{}, 1000) {}
    Pool(TData initializer, SizeType size)
        : m_initializer(initializer)
    {
        m_used.resize(size);
        m_data.resize(size);

        std::fill(std::begin(m_used), std::end(m_used), false);
    }
    virtual ~Pool() {}

    inline bool IsUsed(const TData& data) const
    {
        auto it = std::find(m_data.begin(), m_data.end(), data);
        if (it != m_data.end())
        {
            SizeType idx = it - m_data.begin();
            return IsUsed(idx);
        }

        return false;
    }

    virtual inline bool IsUsed(SizeType idx) const override
    {
        return m_used[idx];
    }

    inline TData& New()
    {
        SizeType idx = GetFreeIndex();
        m_used[idx] = true;
        return Get(idx);
    }

    inline TData& New(SizeType idx)
    {
        BX_ASSERT(m_used[idx] == false, "New called on used index!");
        m_used[idx] = true;
        return Get(idx);
    }

    inline TData& Get(SizeType idx)
    {
        BX_ASSERT(idx < GetSize(), "Index out of bounds!");
        BX_ASSERT(IsUsed(idx), "Index is not used!");

        return m_data[idx];
    }

    inline const TData& Get(SizeType idx) const
    {
        BX_ASSERT(idx < GetSize(), "Index out of bounds!");
        BX_ASSERT(IsUsed(idx), "Index is not used!");

        return m_data[idx];
    }

    virtual inline SizeType GetFreeIndex() override
    {
        for (SizeType idx = 0; idx < GetSize(); ++idx)
        {
            if (!m_used[idx])
                return idx;
        }

        Resize((GetSize() + 1) * 2);
        return GetFreeIndex();
    }

    inline TData& Set(SizeType idx)
    {
        BX_ASSERT(idx < GetSize(), "Index out of bounds!");
        BX_ASSERT(IsUsed(idx), "Setting data to unused index!");

        return m_data[idx];
    }

    inline void Set(const TData& data)
    {
        SizeType idx = GetFreeIndex();
        Set(idx, data);
    }

    inline void Set(SizeType idx, const TData& data)
    {
        BX_ASSERT(idx < GetSize(), "Index out of bounds!");
        BX_ASSERT(IsUsed(idx), "Setting data to unused index!");

        m_data[idx] = data;
    }

    inline void Remove(const TData& data)
    {
        auto it = std::find(m_data.begin(), m_data.end(), data);
        if (it != m_data.end())
        {
            SizeType idx = it - m_data.begin();
            Remove(idx);
        }
    }

    virtual inline void Remove(SizeType idx) override
    {
        BX_ASSERT(idx < GetSize(), "Index out of bounds!");
        BX_ASSERT(IsUsed(idx), "Index is not used!");

        m_used[idx] = false;

        // Reset memory
        TData& data = m_data[idx];
        data.~TData();
        new(&data) TData(m_initializer);
    }

    inline TData& operator[](SizeType idx)
    {
        return Get(idx);
    }

    inline const TData& operator[](SizeType idx) const
    {
        return Get(idx);
    }

    virtual inline SizeType GetSize() const override
    {
        return m_data.size();
    }

    virtual inline void* GetPtr(SizeType idx) const override
    {
        return (void*)&m_data[idx];
    }

    virtual inline void Clear() override
    {
        for (SizeType i = 0; i < GetSize(); ++i)
        {
            if (IsUsed(i))
                Remove(i);
        }
    }

private:
    inline void Resize(SizeType size)
    {
        // TODO: This assert is a placeholder until the ECS can handle resizing
        BX_ASSERT(false, "TODO: The ECS doesn't support resizing!");
        m_used.resize((m_used.size() + 1) * 2, false);
        m_data.resize((m_data.size() + 1) * 2, m_initializer);
    }

    TData m_initializer;
    List<bool> m_used;
    List<TData> m_data;
};
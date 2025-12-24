#ifndef BX_ARRAY
#define BX_ARRAY

#include <bx/core.hpp>
#include <bx/type_traits.hpp>

#include <new>
#include <initializer_list>
#include <utility>

namespace bx
{
    template<typename T>
    struct bx_api array
    {
        using value_type = T;
        using iterator = T*;
        using const_iterator = const T*;

        array() noexcept = default;

        explicit array(usize count) noexcept
        {
            static_assert(std::is_default_constructible<T>::value,
                "T must be default constructible to use resize or count constructor");
            resize(count);
        }

        array(std::initializer_list<T> init) noexcept
            : array(init.size())
        {
            usize i = 0;
            for (auto it = init.begin(); it != init.end() && i < init.size(); ++it, ++i)
                m_data[i] = *it;
        }

        ~array() noexcept
        {
            clear();
            ::operator delete[](m_data, std::nothrow);
        }

        array(const array&) = delete;
        array& operator=(const array&) = delete;

        array(array&& other) noexcept
            : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity)
        {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        array& operator=(array&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                ::operator delete[](m_data, std::nothrow);

                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;

                other.m_data = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            }
            return *this;
        }

        inline T& operator[](usize i) noexcept { return m_data[i]; }
        inline const T& operator[](usize i) const noexcept { return m_data[i]; }

        inline iterator begin() noexcept { return m_data; }
        inline iterator end() noexcept { return m_data + m_size; }

        inline const_iterator begin() const noexcept { return m_data; }
        inline const_iterator end() const noexcept { return m_data + m_size; }

        inline T& front() noexcept { return m_data[0]; }
        inline T& back() noexcept { return m_data[m_size - 1]; }
        inline const T& front() const noexcept { return m_data[0]; }
        inline const T& back() const noexcept { return m_data[m_size - 1]; }

        inline usize size() const noexcept { return m_size; }
        inline bool empty() const noexcept { return m_size == 0; }

        inline T* data() noexcept { return m_data; }
        inline const T* data() const noexcept { return m_data; }

        inline void push_back(const T& value) noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return; // allocation failed, silently do nothing

            new (m_data + m_size) T(value);
            ++m_size;
        }

        inline void push_back(T&& value) noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return;

            new (m_data + m_size) T(static_cast<T&&>(value));
            ++m_size;
        }

        template<typename... Args>
        void emplace_back(Args&&... args) noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return;

            new (m_data + m_size) T(std::forward<Args>(args)...);
            ++m_size;
        }

        inline void pop_back() noexcept
        {
            if (m_size > 0)
            {
                --m_size;
                m_data[m_size].~T();
            }
        }

        inline void resize(usize new_size) noexcept
        {
            static_assert(std::is_default_constructible<T>::value,
                "T must be default constructible to resize");

            if (new_size > m_capacity && !reserve(new_size))
                return;

            for (usize i = m_size; i < new_size; ++i)
                new (m_data + i) T();

            for (usize i = new_size; i < m_size; ++i)
                m_data[i].~T();

            m_size = new_size;
        }

        inline void clear() noexcept
        {
            for (usize i = 0; i < m_size; ++i)
                m_data[i].~T();
            m_size = 0;
        }

        inline bool reserve(usize new_capacity) noexcept
        {
            if (new_capacity <= m_capacity)
                return true;

            T* new_data = static_cast<T*>(::operator new[](new_capacity * sizeof(T), std::nothrow));
            if (!new_data)
                return false; // allocation failed

            for (usize i = 0; i < m_size; ++i)
            {
                new (new_data + i) T(static_cast<T&&>(m_data[i]));
                m_data[i].~T();
            }

            ::operator delete[](m_data, std::nothrow);
            m_data = new_data;
            m_capacity = new_capacity;
            return true;
        }

    private:
        T* m_data{ nullptr };
        usize m_size{ 0 };
        usize m_capacity{ 0 };
    };

	template<typename T, usize N>
	struct bx_api array_fixed
	{
		using value_type = T;
		using iterator = T*;
		using const_iterator = const T*;

        array_fixed() noexcept = default;

        explicit array_fixed(std::initializer_list<T> init) noexcept
        {
            usize i = 0;
            for (auto it = init.begin(); it != init.end() && i < N; ++it, ++i)
                m_data[i] = *it;
        }

		inline T& operator[](usize i) noexcept { return m_data[i]; }
		inline const T& operator[](usize i) const noexcept { return m_data[i]; }

		inline iterator begin() noexcept { return m_data; }
		inline iterator end() noexcept { return m_data + N; }

		inline const_iterator begin() const noexcept { return m_data; }
		inline const_iterator end() const noexcept { return m_data + N; }

		inline usize size() const noexcept { return N; }
		inline bool empty() const noexcept { return N == 0; }

        inline T* data() noexcept { return m_data; }
        inline const T* data() const noexcept { return m_data; }

    private:
        T m_data[N]{};
	};

	template <typename T>
	struct bx_api array_view
	{
		using iterator = T*;
		using const_iterator = const T*;
		
		array_view() noexcept = default;

        array_view(array<T>& arr) noexcept
            : m_data(arr.data()), m_size(arr.size())
        {}

		array_view(T* data, usize size) noexcept
            : m_data(data), m_size(size)
        {}

		template<usize N>
		inline array_view(narray<T, N>& arr) noexcept : m_data(arr), m_size(N) {}

        template<usize N>
        inline array_view(array_fixed<T, N>& arr) noexcept : m_data(arr.data()), m_size(N) {}
		
		inline const T& operator[](usize i) const noexcept { return m_data[i]; }
		inline explicit operator bool() const noexcept { return m_data != nullptr && m_size > 0; }
		
		inline const_iterator begin() const noexcept { return m_data; }
		inline const_iterator end() const noexcept { return m_data + m_size; }

        inline usize size() const noexcept { return m_size; }
        inline bool empty() const noexcept { return m_size == 0; }

        inline T* data() noexcept { return m_data; }
        inline const T* data() const noexcept { return m_data; }
		
    private:
		T* m_data{ nullptr };
		usize m_size{ 0 };
	};
}

#endif // BX_ARRAY
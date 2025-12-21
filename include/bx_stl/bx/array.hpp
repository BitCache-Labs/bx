#ifndef BX_ARRAY
#define BX_ARRAY

#include <bx/core.hpp>
#include <initializer_list>

namespace bx
{
    template<typename T>
    struct bx_api array
    {
        using value_type = T;
        using iterator = T*;
        using const_iterator = const T*;

        array() bx_noexcept = default;

        explicit array(usize count) bx_noexcept
        {
            static_assert(std::is_default_constructible_v<T>,
                "T must be default constructible to use resize or count constructor");
            resize(count);
        }

        constexpr array(std::initializer_list<T> init) bx_noexcept
            : array(init.size())
        {
            usize i = 0;
            for (auto it = init.begin(); it != init.end() && i < init.size(); ++it, ++i)
                m_data[i] = *it;
        }

        ~array() bx_noexcept
        {
            clear();
            ::operator delete[](m_data, std::nothrow);
        }

        array(const array&) = delete;
        array& operator=(const array&) = delete;

        array(array&& other) bx_noexcept
            : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity)
        {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        array& operator=(array&& other) bx_noexcept
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

        T& operator[](usize i) bx_noexcept { return m_data[i]; }
        const T& operator[](usize i) const bx_noexcept { return m_data[i]; }

        iterator begin() bx_noexcept { return m_data; }
        iterator end() bx_noexcept { return m_data + m_size; }

        const_iterator begin() const bx_noexcept { return m_data; }
        const_iterator end() const bx_noexcept { return m_data + m_size; }

        T& front() bx_noexcept { return m_data[0]; }
        T& back() bx_noexcept { return m_data[m_size - 1]; }
        const T& front() const bx_noexcept { return m_data[0]; }
        const T& back() const bx_noexcept { return m_data[m_size - 1]; }

        usize size() const bx_noexcept { return m_size; }
        bool empty() const bx_noexcept { return m_size == 0; }

        T* data() bx_noexcept { return m_data; }
        const T* data() const bx_noexcept { return m_data; }

        void push_back(const T& value) bx_noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return; // allocation failed, silently do nothing

            new (m_data + m_size) T(value);
            ++m_size;
        }

        void push_back(T&& value) bx_noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return;

            new (m_data + m_size) T(static_cast<T&&>(value));
            ++m_size;
        }

        template<typename... Args>
        void emplace_back(Args&&... args) bx_noexcept
        {
            if (m_size == m_capacity && !reserve(m_capacity == 0 ? 4 : m_capacity * 2))
                return;

            new (m_data + m_size) T(std::forward<Args>(args)...);
            ++m_size;
        }

        void pop_back() bx_noexcept
        {
            if (m_size > 0)
            {
                --m_size;
                m_data[m_size].~T();
            }
        }

        void resize(usize new_size) bx_noexcept
        {
            static_assert(std::is_default_constructible_v<T>,
                "T must be default constructible to resize");

            if (new_size > m_capacity && !reserve(new_size))
                return;

            for (usize i = m_size; i < new_size; ++i)
                new (m_data + i) T();

            for (usize i = new_size; i < m_size; ++i)
                m_data[i].~T();

            m_size = new_size;
        }

        void clear() bx_noexcept
        {
            for (usize i = 0; i < m_size; ++i)
                m_data[i].~T();
            m_size = 0;
        }

        bool reserve(usize new_capacity) bx_noexcept
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

        constexpr array_fixed(std::initializer_list<T> init) bx_noexcept
        {
            usize i = 0;
            for (auto it = init.begin(); it != init.end() && i < N; ++it, ++i)
                m_data[i] = *it;
        }

		constexpr T& operator[](usize i) bx_noexcept { return m_data[i]; }
		constexpr const T& operator[](usize i) const bx_noexcept { return m_data[i]; }

		constexpr iterator begin() bx_noexcept { return m_data; }
		constexpr iterator end() bx_noexcept { return m_data + N; }

		constexpr const_iterator begin() const bx_noexcept { return m_data; }
		constexpr const_iterator end() const bx_noexcept { return m_data + N; }

		constexpr usize size() const bx_noexcept { return N; }
		constexpr bool empty() const bx_noexcept { return N == 0; }

        constexpr T* data() bx_noexcept { return m_data; }
        constexpr const T* data() const bx_noexcept { return m_data; }

    private:
        T m_data[N]{};
	};

	template <typename T>
	struct bx_api array_view
	{
		using iterator = T*;
		using const_iterator = const T*;
		
		constexpr array_view() bx_noexcept = default;
        constexpr array_view(const array<T>& arr) bx_noexcept : m_data(arr.data()), m_size(arr.size()) {}
		constexpr array_view(carray<T> data, usize size) bx_noexcept : m_data(data), m_size(size) {}

		template<usize N>
		constexpr array_view(const narray<T, N>& arr) bx_noexcept : m_data(arr), m_size(N) {}

        template<usize N>
        constexpr array_view(const array_fixed<T, N>& arr) bx_noexcept : m_data(arr.data()), m_size(N) {}
		
		constexpr const T& operator[](usize i) const bx_noexcept { return m_data[i]; }
		constexpr explicit operator bool() const bx_noexcept { return m_data != nullptr && m_size > 0; }
		
		constexpr const_iterator begin() const bx_noexcept { return m_data; }
		constexpr const_iterator end() const bx_noexcept { return m_data + m_size; }

        usize size() const bx_noexcept { return m_size; }
        bool empty() const bx_noexcept { return m_size == 0; }

        constexpr T* data() bx_noexcept { return m_data; }
        constexpr const T* data() const bx_noexcept { return m_data; }
		
    private:
		carray<T> m_data{ nullptr };
		usize m_size{ 0 };
	};
}

#endif // BX_ARRAY
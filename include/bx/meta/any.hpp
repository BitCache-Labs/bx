#pragma once

#include <typeinfo>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace meta
{
    class bad_any_cast : public std::bad_cast
    {
    public:
        const char* what() const noexcept override
        {
            return "bad any_cast";
        }
    };

    class any
    {
    public:
        // Default constructor
        any() noexcept : content(nullptr) {}

        // Destructor
        ~any() { delete content; }

        // Constructor for any type T
        template<typename T>
        any(const T& value) : content(new holder<T>(value)) {}

        // Move constructor
        any(any&& other) noexcept : content(other.content)
        {
            other.content = nullptr;
        }

        // Copy constructor
        any(const any& other) : content(other.content ? other.content->clone() : nullptr) {}

        // Move assignment
        any& operator=(any&& other) noexcept
        {
            if (this != &other)
            {
                delete content;
                content = other.content;
                other.content = nullptr;
            }
            return *this;
        }

        // Copy assignment
        any& operator=(const any& other)
        {
            if (this != &other)
            {
                delete content;
                content = other.content ? other.content->clone() : nullptr;
            }
            return *this;
        }

        // Assignment operator for any type T
        template<typename T>
        any& operator=(const T& value)
        {
            any(value).swap(*this);
            return *this;
        }

        // Swap function
        void swap(any& other) noexcept
        {
            std::swap(content, other.content);
        }

        // Clear the held value
        void reset() noexcept
        {
            delete content;
            content = nullptr;
        }

        // Check if there's a value stored
        bool has_value() const noexcept
        {
            return content != nullptr;
        }

        // Get the stored type information
        const std::type_info& type() const noexcept
        {
            return content ? content->type() : typeid(void);
        }

    private:
        // Base class for type erasure
        struct placeholder
        {
            virtual ~placeholder() = default;
            virtual const std::type_info& type() const noexcept = 0;
            virtual placeholder* clone() const = 0;
        };

        // Template class for holding a value of type T
        template<typename T>
        struct holder : placeholder
        {
            holder(const T& value) : held(value) {}

            const std::type_info& type() const noexcept override
            {
                return typeid(T);
            }

            placeholder* clone() const override
            {
                return new holder(held);
            }

            T held;
        };

        // Pointer to the currently held value
        placeholder* content;

        // Friend functions to allow access to private members
        template<typename T>
        friend T any_cast(const any&);
        template<typename T>
        friend T any_cast(any&);
        template<typename T>
        friend T any_cast(any&&);
    };

    // Function to cast `any` back to its original type
    template<typename T>
    T any_cast(const any& operand)
    {
        if (operand.type() != typeid(T))
        {
            throw bad_any_cast();
        }

        return static_cast<any::holder<typename std::decay<const T>::type>*>(operand.content)->held;
    }

    template<typename T>
    T any_cast(any& operand)
    {
        if (operand.type() != typeid(T))
        {
            throw bad_any_cast();
        }

        return static_cast<any::holder<typename std::decay<T>::type>*>(operand.content)->held;
    }

    template<typename T>
    T any_cast(any&& operand)
    {
        if (operand.type() != typeid(T))
        {
            throw bad_any_cast();
        }

        return std::move(static_cast<any::holder<typename std::decay<T>::type>*>(operand.content)->held);
    }
}

using Any = meta::any;
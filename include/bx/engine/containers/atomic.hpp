#pragma once

#include <mutex>
#include <thread>
#include <functional>

template<class T>
struct Atomic
{
public:
    Atomic() = default;
    explicit Atomic(T in)
        : data(std::move(in))
    {
    
    }

    void Read(std::function<void(const T&)> f) const
    {
        auto l = std::unique_lock<std::mutex>(m_mutex);
        f(data);
    }

    void Write(std::function<void(T&)> f)
    {
        auto l = std::unique_lock<std::mutex>(m_mutex);
        f(data);
    }
    
private:
    mutable std::mutex m_mutex;
    T data;
};
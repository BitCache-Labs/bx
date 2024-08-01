#pragma once

#include <memory>
#include <utility>

#include "bx/engine/containers/hash_map.hpp"

// Lazily initialize a const T on demand
template <typename InitializerT, typename T>
struct LazyInit : NoCopy
{
	virtual ~LazyInit() {}

    template <typename ...Params>
	static const T& Get(Params&&... params)
    {
        if (!cache)
        {
            cache = std::unique_ptr<InitializerT>(new InitializerT(std::forward<Params>(params)...));
        }

        return cache->data;
    }

	static void Clear()
    {
        cache.reset();
    }

protected:
    T data;

private:
    static std::unique_ptr<InitializerT> cache;
};

// Lazily initialize a const T on demand but caches based on initializer args
template <typename InitializerT, typename T, typename InitArgsT>
struct LazyInitMap : NoCopy
{
    virtual ~LazyInitMap() {}

    static const T& Get(const InitArgsT& params)
    {
        auto cacheIter = cache.find(params);
        if (cacheIter == cache.end())
        {
            cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(params),
                std::forward_as_tuple(new InitializerT(params)));
        }

        return cache.find(params)->second->data;
    }

    static void Clear()
    {
        cache.clear();
    }

protected:
    T data;

private:
    static HashMap<InitArgsT, std::unique_ptr<InitializerT>> cache;
};
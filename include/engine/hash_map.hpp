#pragma once

#include <engine/hash.hpp>
#include <unordered_map>

template <typename K, typename V, typename Hasher = Hash<K>>
using HashMap = std::unordered_map<K, V, Hasher>;
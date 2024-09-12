#pragma once

#include "bx/engine/containers/tree.hpp"

#include "bx/engine/core/serial.serial.hpp"
#include "bx/engine/containers/list.serial.hpp"
#include "bx/engine/containers/hash_map.serial.hpp"

template<class Archive, typename T>
void serialize(Archive& ar, TreeNode<T>& data)
{
	ar(cereal::make_nvp("data", data.data));
	ar(cereal::make_nvp("parent", data.parent));
	ar(cereal::make_nvp("children", data.children));
}

template <typename T>
class Serial<Tree<T>>
{
public:
	template<class Archive>
	static void Save(Archive& ar, const Tree<T>& data)
	{
		ar(cereal::make_nvp("root", data.m_root));
		ar(cereal::make_nvp("nodes", data.m_nodes));
		ar(cereal::make_nvp("indices", data.m_indices));
	}

	template<class Archive>
	static void Load(Archive& ar, Tree<T>& data)
	{
		ar(cereal::make_nvp("root", data.m_root));
		ar(cereal::make_nvp("nodes", data.m_nodes));
		ar(cereal::make_nvp("indices", data.m_indices));
	}
};
REGISTER_SERIAL_T(Tree)
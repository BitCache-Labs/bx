#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/string.hpp>
#include <engine/log.hpp>
#include <engine/guard.hpp>
#include <engine/memory.hpp>
#include <engine/file.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/json.hpp>

LOG_CHANNEL(Serial)

namespace rttr
{
    namespace serial
    {
        namespace tags
        {
            static constexpr const char* no_serialize = "NO_SERIALIZE";
        }

        struct BX_API ObjectWrapper
        {
            rttr::variant data{};
        };

        struct BX_API EnumWrapper
        {
            rttr::variant data{};
        };
    }
}

namespace rttr
{
    namespace serial
    {
        template <class Archive, typename T>
        void save_val(Archive& archive, const rttr::string_view& name, const T& val)
        {
            if (!name.empty())
                archive(cereal::make_nvp(name.data(), val));
            else
                archive(val);
        }

        template <class Archive>
        void save_enum(Archive& archive, const rttr::variant& obj)
        {
            bool enum_is_str = false;
            auto enum_str = obj.to_string(&enum_is_str);
            archive(cereal::make_nvp("enum_str", enum_is_str ? enum_str : ""));

            bool enum_is_val = false;
            auto enum_val = obj.to_int32(&enum_is_val);
            archive(cereal::make_nvp("enum_val", enum_is_val ? enum_val : 0));
        }

        template<class Archive>
        bool save_basic_type(Archive& archive, const rttr::string_view& name, const rttr::variant& obj)
        {
            auto value_type = obj.get_type();
            auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
            bool is_wrapper = wrapped_type != value_type;

            const rttr::type& type = is_wrapper ? wrapped_type : value_type;
            const rttr::variant& var = is_wrapper ? obj.extract_wrapped_value() : obj;

            if (type.is_arithmetic())
            {
                if (type == rttr::type::get<bool>())
                    save_val(archive, name, var.to_bool());
                else if (type == rttr::type::get<char>())
                    save_val(archive, name, var.to_bool());
                else if (type == rttr::type::get<i8>())
                    save_val(archive, name, var.to_int8());
                else if (type == rttr::type::get<i16>())
                    save_val(archive, name, var.to_int16());
                else if (type == rttr::type::get<i32>())
                    save_val(archive, name, var.to_int32());
                else if (type == rttr::type::get<i64>())
                    save_val(archive, name, var.to_int64());
                else if (type == rttr::type::get<u8>())
                    save_val(archive, name, var.to_uint8());
                else if (type == rttr::type::get<u16>())
                    save_val(archive, name, var.to_uint16());
                else if (type == rttr::type::get<u32>())
                    save_val(archive, name, var.to_uint32());
                else if (type == rttr::type::get<u64>())
                    save_val(archive, name, var.to_uint64());
                else if (type == rttr::type::get<f32>())
                    save_val(archive, name, var.to_float());
                else if (type == rttr::type::get<f64>())
                    save_val(archive, name, var.to_double());

                return true;
            }
            else if (type.is_enumeration())
            {
                save_val(archive, name, EnumWrapper{ var });
                return true;
            }
            else if (type == rttr::type::get<String>())
            {
                save_val(archive, name, var.to_string());
                return true;
            }

            return false;
        }
        
        template<class Archive>
        void save_array(Archive& archive, const rttr::variant_sequential_view& view)
        {
            archive(cereal::make_size_tag(static_cast<cereal::size_type>(view.get_size())));
            
            for (const auto& item : view)
            {
                if (save_basic_type(archive, "", item))
                    continue;

                archive(ObjectWrapper{ item });
            }
        }

        template<class Archive>
        void save_map(Archive& archive, const rttr::variant_associative_view& view)
        {
            static const rttr::string_view key_name{ "key" };
            static const rttr::string_view value_name{ "value" };

            archive(cereal::make_size_tag(static_cast<cereal::size_type>(view.get_size())));

            if (view.is_key_only_type())
            {
                for (const auto& item : view)
                {
                    if (save_basic_type(archive, "", item.first))
                        continue;

                    archive(ObjectWrapper{ item.first });
                }
            }
            else
            {
                for (const auto& item : view)
                {
                    archive(cereal::make_map_item(ObjectWrapper{ item.first }, ObjectWrapper{ item.second }));
                }
            }
        }

        inline rttr::variant unwrap_instance(const rttr::variant& obj)
        {
            rttr::variant unwrapped_inst{ obj };
            bool is_wrapper{ true };
            do
            {
                is_wrapper = unwrapped_inst.get_type().get_raw_type().is_wrapper();
                unwrapped_inst = is_wrapper ? unwrapped_inst.extract_wrapped_value() : unwrapped_inst;

            } while (is_wrapper);
            return unwrapped_inst;
        }

        template<class Archive>
        void save_instance(Archive& archive, const rttr::variant& obj)
        {
            bool is_wrapper = obj.get_type().get_raw_type().is_wrapper();
            const rttr::instance inst = is_wrapper ? unwrap_instance(obj) : obj;

            //bool is_derived = inst.get_type() != inst.get_derived_type();
            if (is_wrapper)// && is_derived)
            {
                // Write out a special property for the type.
                // TODO: Make a serialization function for StringView and CString<N>
                rttr::type actual_type = inst.get_derived_type();
                rttr::string_view type_name = actual_type.get_name();
                archive(cereal::make_nvp("@type", type_name.to_string()));
            }

            // Now write the properties.
            // (Use the underlying non-wrapper object, if applicable.)
            auto prop_list = inst.get_derived_type().get_properties();

            if (prop_list.empty())
            {
                bool ok = false;
                auto text = obj.to_string(&ok);
                archive(ok ? text : "unknown");
                return;
            }

            for (auto prop : prop_list)
            {
                if (prop.get_metadata(rttr::serial::tags::no_serialize))
                    continue;

                rttr::variant prop_value = prop.get_value(inst);
                if (!prop_value)
                    continue; // cannot serialize, because we cannot retrieve the value

                const auto prop_name = prop.get_name();
                if (save_basic_type(archive, prop_name, prop_value))
                    continue;

                archive(cereal::make_nvp(prop_name.data(), ObjectWrapper{ prop_value }));
            }
        }

        template<class Archive>
        void save_variant(Archive& archive, const rttr::variant& obj)
        {
            if (save_basic_type(archive, "", obj))
                return;

            if (obj.is_sequential_container())
            {
                save_array(archive, obj.create_sequential_view());
            }
            else if (obj.is_associative_container())
            {
                save_map(archive, obj.create_associative_view());
            }
            else
            {
                save_instance(archive, obj);
            }
        }

        template<class Archive>
        void save(Archive& archive, const EnumWrapper& wrapper)
        {
            save_enum(archive, wrapper.data);
        }

        template<class Archive>
        void save(Archive& archive, const ObjectWrapper& wrapper)
        {
            save_variant(archive, wrapper.data);
        }
    }
}

template<class Archive, typename T>
void save(Archive& archive, const T& data)
{
    rttr::serial::save_variant(archive, data);
}

namespace rttr
{
    namespace serial
    {
        template <class Archive, typename T>
        void load_val(Archive& archive, const rttr::string_view& name, rttr::variant& obj)
        {
            T val{};
            if (!name.empty())
                archive(cereal::make_nvp(name.data(), val));
            else
                archive(val);
            obj = val;
        }

        template <class Archive>
        void load_enum(Archive& archive, rttr::variant& obj)
        {
            String enum_str{};
            i32 enum_val{};

            archive(cereal::make_nvp("enum_str", enum_str));
            archive(cereal::make_nvp("enum_val", enum_val));

            if (!enum_str.empty())
                obj = enum_str;
            else
                obj = enum_val;
        }

        template <class Archive>
        bool load_basic_type(Archive& archive, const rttr::string_view& name, rttr::variant& obj)
        {
            auto value_type = obj.get_type();
            auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
            bool is_wrapper = wrapped_type != value_type;

            rttr::type type = is_wrapper ? wrapped_type : value_type;
            rttr::variant var = is_wrapper ? obj.extract_wrapped_value() : obj;

            if (type.is_arithmetic())
            {
                if (type == rttr::type::get<bool>())
                    load_val<Archive, bool>(archive, name, var);
                else if (type == rttr::type::get<char>())
                    load_val<Archive, bool>(archive, name, var);
                else if (type == rttr::type::get<i8>())
                    load_val<Archive, i8>(archive, name, var);
                else if (type == rttr::type::get<i16>())
                    load_val<Archive, i16>(archive, name, var);
                else if (type == rttr::type::get<i32>())
                    load_val<Archive, i32>(archive, name, var);
                else if (type == rttr::type::get<i64>())
                    load_val<Archive, i64>(archive, name, var);
                else if (type == rttr::type::get<u8>())
                    load_val<Archive, u8>(archive, name, var);
                else if (type == rttr::type::get<u16>())
                    load_val<Archive, u16>(archive, name, var);
                else if (type == rttr::type::get<u32>())
                    load_val<Archive, u32>(archive, name, var);
                else if (type == rttr::type::get<u64>())
                    load_val<Archive, u64>(archive, name, var);
                else if (type == rttr::type::get<f32>())
                    load_val<Archive, f32>(archive, name, var);
                else if (type == rttr::type::get<f64>())
                    load_val<Archive, f64>(archive, name, var);

                obj = var;
                return true;
            }
            else if (type.is_enumeration())
            {
                EnumWrapper wrapper{ var };

                if (!name.empty())
                    archive(cereal::make_nvp(name.data(), wrapper));
                else
                    archive(wrapper);

                obj = wrapper.data;
                return true;
            }
            else if (type == rttr::type::get<String>())
            {
                load_val<Archive, String>(archive, name, var);
                
                obj = var;
                return true;
            }

            return false;
        }

        template<class Archive>
        void load_array(Archive& archive, rttr::variant_sequential_view& view)
        {
            cereal::size_type size{};
            archive(cereal::make_size_tag(size));

            view.set_size(size);
            const rttr::type array_value_type = view.get_rank_type(1);

            for (cereal::size_type i = 0; i < size; ++i)
            {
                auto item = view.get_value(i);

                if (load_basic_type(archive, "", item))
                {
                    BX_ENSURE(item.convert(array_value_type));
                    BX_ENSURE(view.set_value(i, item));
                    continue;
                }

                rttr::variant wrapped_var = item.is_sequential_container() ? item : item.extract_wrapped_value();
                ObjectWrapper wrapper{ wrapped_var };
                archive(wrapper);
                BX_ENSURE(wrapper.data.convert(array_value_type));
                BX_ENSURE(view.set_value(i, wrapper.data));
            }
        }

        template <class Archive>
        void validate_variant(Archive& archive, rttr::variant& obj)
        {
            const rttr::instance inst{ obj };
            bool is_wrapper = obj.get_type().get_raw_type().is_wrapper();
            bool is_valid = is_wrapper ? inst.get_wrapped_instance().is_valid() : inst.is_valid();

            if (!is_valid)
            {
                if (is_wrapper)
                {
                    String type_name{};
                    archive(cereal::make_nvp("@type", type_name));

                    rttr::type actual_type = rttr::type::get_by_name(type_name);
                    obj = actual_type.create();
                }
                else
                {
                    obj = obj.get_type().create();
                }
            }
        }

        inline rttr::variant create_map_item(rttr::type type)
        {
            if (type.is_arithmetic())
            {
                if (type == rttr::type::get<bool>())
                    return false;
                else if (type == rttr::type::get<char>())
                    return false;
                else if (type == rttr::type::get<i8>())
                    return (i8)0;
                else if (type == rttr::type::get<i16>())
                    return (i16)0;
                else if (type == rttr::type::get<i32>())
                    return (i32)0;
                else if (type == rttr::type::get<i64>())
                    return (i64)0;
                else if (type == rttr::type::get<u8>())
                    return (u8)0;
                else if (type == rttr::type::get<u16>())
                    return (u16)0;
                else if (type == rttr::type::get<u32>())
                    return (u32)0;
                else if (type == rttr::type::get<u64>())
                    return (u64)0;
                else if (type == rttr::type::get<f32>())
                    return (f32)0;
                else if (type == rttr::type::get<f64>())
                    return (f64)0;
            }

            if (type.is_enumeration())
            {
                auto val = *type.get_enumeration().get_values().begin();
                return val;
            }

            if (type == rttr::type::get<String>())
                return String{};

            if (type.is_wrapper())
            {
                auto wrapped_type = type.get_wrapped_type().get_raw_type();
                auto val = wrapped_type.create();
                auto t = val.get_type();
                return val;
            }

            rttr::constructor ctor = type.get_constructor();
            for (auto& item : type.get_constructors())
            {
                if (item.get_instantiated_type() == type)
                    ctor = item;
            }
            return ctor.invoke();
        }

        template <class Archive>
        void load_map(Archive& archive, rttr::variant_associative_view& view)
        {
            cereal::size_type size{};
            archive(cereal::make_size_tag(size));

            for (cereal::size_type i = 0; i < size; ++i)
            {
                if (view.is_key_only_type())
                {
                    auto item = create_map_item(view.get_key_type());

                    if (load_basic_type(archive, "", item))
                    {
                        BX_ENSURE(item.convert(view.get_key_type()));
                        auto ret = view.insert(item);
                        BX_ENSURE(ret.second);
                        continue;
                    }

                    rttr::variant wrapped_var = item;// .is_sequential_container() ? item : item.extract_wrapped_value();
                    ObjectWrapper wrapper{ wrapped_var };
                    archive(wrapper);
                    BX_ENSURE(wrapper.data.convert(view.get_key_type()));
                    auto ret = view.insert(wrapper.data);
                    BX_ENSURE(ret.second);
                }
                else
                {
                    auto item_key = create_map_item(view.get_key_type());
                    auto item_value = create_map_item(view.get_value_type());

                    rttr::variant wrapped_key_var = item_key;// .is_sequential_container() ? item_key : item_key.extract_wrapped_value();
                    auto t1 = wrapped_key_var.get_type();
                    auto key_wrapper = ObjectWrapper{ wrapped_key_var };

                    rttr::variant wrapped_value_var = item_value;// .is_sequential_container() ? item_value : item_value.extract_wrapped_value();
                    auto t2 = wrapped_value_var.get_type();
                    auto value_wrapper = ObjectWrapper{ wrapped_value_var };

                    archive(cereal::make_map_item(key_wrapper, value_wrapper));

                    BX_ENSURE(key_wrapper.data.convert(view.get_key_type()));
                    BX_ENSURE(value_wrapper.data.convert(view.get_value_type()));

                    if (key_wrapper.data && value_wrapper.data)
                    {
                        auto ret = view.insert(key_wrapper.data, value_wrapper.data);
                        BX_ENSURE(ret.second);
                    }
                }
            }
        }

        template<class Archive>
        void load_instance(Archive& archive, const rttr::instance& obj)
        {
            rttr::instance inst = obj.get_type().get_raw_type().is_wrapper() ? obj.get_wrapped_instance() : obj;
            const auto prop_list = inst.get_derived_type().get_properties();

            for (auto prop : prop_list)
            {
                if (prop.get_metadata(rttr::serial::tags::no_serialize))
                    continue;

                const rttr::type prop_type = prop.get_type();

                rttr::variant prop_value = prop.get_value(inst);
                if (!prop_value)
                    continue;

                const auto prop_name = prop.get_name();
                if (load_basic_type(archive, prop_name, prop_value))
                {
                    BX_ENSURE(prop_value.convert(prop_type));
                    BX_ENSURE(prop.set_value(inst, prop_value));
                    continue;
                }

                ObjectWrapper wrapper{ prop_value };
                archive(cereal::make_nvp(prop_name.data(), wrapper));
                BX_ENSURE(prop.set_value(obj, wrapper.data));
            }
        }

        template <class Archive>
        void load_variant(Archive& archive, rttr::variant& obj)
        {
            if (load_basic_type(archive, "", obj))
                return;

            auto value_type = obj.get_type();
            auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
            bool is_wrapper = wrapped_type != value_type;

            if (obj.is_sequential_container())
            {
                load_array(archive, obj.create_sequential_view());
            }
            else if (obj.is_associative_container())
            {
                load_map(archive, obj.create_associative_view());
            }
            else
            {
                validate_variant(archive, obj);

                auto child_props = is_wrapper ? wrapped_type.get_properties() : value_type.get_properties();
                if (!child_props.empty())
                {
                    load_instance(archive, obj);
                }
                else
                {
                    String text{};
                    archive(text);
                }
            }
        }

        template<class Archive>
        void load(Archive& archive, EnumWrapper& wrapper)
        {
            load_enum(archive, wrapper.data);
        }

        template<class Archive>
        void load(Archive& archive, ObjectWrapper& wrapper)
        {
            load_variant(archive, wrapper.data);
        }
    }
}

template<class Archive, typename T>
void load(Archive& archive, T& data)
{
    rttr::variant var{ data };
    rttr::serial::load_variant(archive, var);
    var.convert(data);
}
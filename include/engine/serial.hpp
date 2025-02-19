#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/string.hpp>
#include <engine/log.hpp>
#include <engine/guard.hpp>
#include <engine/memory.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

LOG_CHANNEL(Serial)

namespace rttr
{
    namespace serial
    {
        struct ObjectWrapper
        {
            rttr::variant data{};
        };

        struct EnumWrapper
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
            archive(cereal::make_size_tag(static_cast<cereal::size_type>(view.get_size()))); // number of elements
            
            for (const auto& item : view)
            {
                if (save_basic_type(archive, "", item))
                    continue;

                archive(ObjectWrapper{ item });
            }
        }

        /*template<class Archive>
        void save_map(Archive& archive, const rttr::variant_associative_view& view)
        {
            static const rttr::string_view key_name("key");
            static const rttr::string_view value_name("value");

            writer.StartArray();

            if (view.is_key_only_type())
            {
                for (auto& item : view)
                {
                    write_variant(item.first, writer);
                }
            }
            else
            {
                for (auto& item : view)
                {
                    writer.StartObject();
                    writer.String(key_name.data(), static_cast<rapidjson::SizeType>(key_name.length()), false);

                    write_variant(item.first, writer);

                    writer.String(value_name.data(), static_cast<rapidjson::SizeType>(value_name.length()), false);

                    write_variant(item.second, writer);

                    writer.EndObject();
                }
            }

            writer.EndArray();
        }*/

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
                if (prop.get_metadata("NO_SERIALIZE"))
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
                //save_map(archive, obj.create_associative_view());
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
        void extract_basic_type(Archive& archive, const rttr::string_view& name, rttr::variant& obj)
        {
            T v{};
            archive(cereal::make_nvp(name.data(), v));
            obj = v;
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

            const rttr::type& type = is_wrapper ? wrapped_type : value_type;
            const rttr::variant& var = is_wrapper ? obj.extract_wrapped_value() : obj;

            if (type.is_arithmetic())
            {
                if (type == rttr::type::get<bool>())
                    extract_basic_type<Archive, bool>(archive, name, obj);
                else if (type == rttr::type::get<char>())
                    extract_basic_type<Archive, bool>(archive, name, obj);
                else if (type == rttr::type::get<i8>())
                    extract_basic_type<Archive, i8>(archive, name, obj);
                else if (type == rttr::type::get<i16>())
                    extract_basic_type<Archive, i16>(archive, name, obj);
                else if (type == rttr::type::get<i32>())
                    extract_basic_type<Archive, i32>(archive, name, obj);
                else if (type == rttr::type::get<i64>())
                    extract_basic_type<Archive, i64>(archive, name, obj);
                else if (type == rttr::type::get<u8>())
                    extract_basic_type<Archive, u8>(archive, name, obj);
                else if (type == rttr::type::get<u16>())
                    extract_basic_type<Archive, u16>(archive, name, obj);
                else if (type == rttr::type::get<u32>())
                    extract_basic_type<Archive, u32>(archive, name, obj);
                else if (type == rttr::type::get<u64>())
                    extract_basic_type<Archive, u64>(archive, name, obj);
                else if (type == rttr::type::get<f32>())
                    extract_basic_type<Archive, f32>(archive, name, obj);
                else if (type == rttr::type::get<f64>())
                    extract_basic_type<Archive, f64>(archive, name, obj);

                return true;
            }
            else if (type.is_enumeration())
            {
                EnumWrapper wrapper{ obj };
                archive(cereal::make_nvp(name.data(), wrapper));
                obj = wrapper.data;

                return true;
            }
            else if (type == rttr::type::get<String>())
            {
                extract_basic_type<Archive, String>(archive, name, obj);
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
                BX_ENSURE(view.set_value(i, wrapper.data));
            }
        }

        template<class Archive>
        void load_instance(Archive& archive, const rttr::instance& obj)
        {
            rttr::instance inst = obj.get_type().get_raw_type().is_wrapper() ? obj.get_wrapped_instance() : obj;
            const auto prop_list = inst.get_derived_type().get_properties();

            for (auto prop : prop_list)
            {
                if (prop.get_metadata("NO_SERIALIZE"))
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
            //else if (var.is_associative_container())
            //{
            //    write_associative_container(var.create_associative_view(), writer);
            //}
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
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
                    archive(cereal::make_nvp(name.data(), var.to_bool()));
                else if (type == rttr::type::get<char>())
                    archive(cereal::make_nvp(name.data(), var.to_bool()));
                else if (type == rttr::type::get<i8>())
                    archive(cereal::make_nvp(name.data(), var.to_int8()));
                else if (type == rttr::type::get<i16>())
                    archive(cereal::make_nvp(name.data(), var.to_int16()));
                else if (type == rttr::type::get<i32>())
                    archive(cereal::make_nvp(name.data(), var.to_int32()));
                else if (type == rttr::type::get<i64>())
                    archive(cereal::make_nvp(name.data(), var.to_int64()));
                else if (type == rttr::type::get<u8>())
                    archive(cereal::make_nvp(name.data(), var.to_uint8()));
                else if (type == rttr::type::get<u16>())
                    archive(cereal::make_nvp(name.data(), var.to_uint16()));
                else if (type == rttr::type::get<u32>())
                    archive(cereal::make_nvp(name.data(), var.to_uint32()));
                else if (type == rttr::type::get<u64>())
                    archive(cereal::make_nvp(name.data(), var.to_uint64()));
                else if (type == rttr::type::get<f32>())
                    archive(cereal::make_nvp(name.data(), var.to_float()));
                else if (type == rttr::type::get<f64>())
                    archive(cereal::make_nvp(name.data(), var.to_double()));

                return true;
            }
            else if (type.is_enumeration())
            {
                archive(cereal::make_nvp(name.data(), EnumWrapper{ var }));
                return true;
            }
            else if (type == rttr::type::get<String>())
            {
                archive(cereal::make_nvp(name.data(), var.to_string()));
                return true;
            }

            return false;
        }

        template<class Archive>
        void save_instance(Archive& archive, const rttr::instance& obj)
        {
            bool is_wrapper = obj.get_type().get_raw_type().is_wrapper();
            const rttr::instance inst = is_wrapper ? obj.get_wrapped_instance() : obj;

            if (is_wrapper)
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

            auto value_type = obj.get_type();
            auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
            bool is_wrapper = wrapped_type != value_type;

            //else if (var.is_sequential_container())
            //{
            //    write_array(var.create_sequential_view(), writer);
            //}
            //else if (var.is_associative_container())
            //{
            //    write_associative_container(var.create_associative_view(), writer);
            //}
            //else
            {
                auto child_props = is_wrapper ? wrapped_type.get_properties() : value_type.get_properties();
                if (!child_props.empty())
                {
                    save_instance(archive, obj);
                }
                else
                {
                    bool ok = false;
                    auto text = obj.to_string(&ok);
                    archive(ok ? text : "unknown");
                }
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
    archive(cereal::make_nvp("wrapper", rttr::serial::ObjectWrapper{ data }));
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
                prop.set_value(obj, wrapper.data);
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

            //else if (var.is_sequential_container())
            //{
            //    write_array(var.create_sequential_view(), writer);
            //}
            //else if (var.is_associative_container())
            //{
            //    write_associative_container(var.create_associative_view(), writer);
            //}
            //else
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
    rttr::serial::ObjectWrapper wrapper{ data };
    archive(cereal::make_nvp("wrapper", wrapper));
    wrapper.data.convert(data);
}
#include <engine/serial.hpp>
#include <engine/file.hpp>

#include <cstdio>
#include <string>
#include <vector>
#include <array>

#include <iostream>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>

using namespace rapidjson;
using namespace rttr;

static bool write_variant(const variant& var, PrettyWriter<StringBuffer>& writer);
static void write_obj(const instance& obj2, PrettyWriter<StringBuffer>& writer);

static bool write_basic_types(const type& t, const variant& var, PrettyWriter<StringBuffer>& writer)
{
    if (t.is_arithmetic())
    {
        if (t == type::get<bool>())
            writer.Bool(var.to_bool());
        else if (t == type::get<char>())
            writer.Bool(var.to_bool());
        else if (t == type::get<int8_t>())
            writer.Int(var.to_int8());
        else if (t == type::get<int16_t>())
            writer.Int(var.to_int16());
        else if (t == type::get<int32_t>())
            writer.Int(var.to_int32());
        else if (t == type::get<int64_t>())
            writer.Int64(var.to_int64());
        else if (t == type::get<uint8_t>())
            writer.Uint(var.to_uint8());
        else if (t == type::get<uint16_t>())
            writer.Uint(var.to_uint16());
        else if (t == type::get<uint32_t>())
            writer.Uint(var.to_uint32());
        else if (t == type::get<uint64_t>())
            writer.Uint64(var.to_uint64());
        else if (t == type::get<float>())
            writer.Double(var.to_double());
        else if (t == type::get<double>())
            writer.Double(var.to_double());

        return true;
    }
    else if (t.is_enumeration())
    {
        bool ok = false;
        auto result = var.to_string(&ok);
        if (ok)
        {
            writer.String(var.to_string());
        }
        else
        {
            ok = false;
            auto value = var.to_uint64(&ok);
            if (ok)
                writer.Uint64(value);
            else
                writer.Null();
        }

        return true;
    }
    else if (t == type::get<std::string>())
    {
        writer.String(var.to_string());
        return true;
    }

    return false;
}

static void write_array(const variant_sequential_view& view, PrettyWriter<StringBuffer>& writer)
{
    writer.StartArray();
    for (const auto& item : view)
    {
        if (item.is_sequential_container())
        {
            write_array(item.create_sequential_view(), writer);
        }
        else
        {
            variant wrapped_var = item.extract_wrapped_value();
            type value_type = wrapped_var.get_type();
            if (value_type.is_arithmetic() || value_type == type::get<std::string>() || value_type.is_enumeration())
            {
                write_basic_types(value_type, wrapped_var, writer);
            }
            else // object
            {
                write_obj(wrapped_var, writer);
            }
        }
    }
    writer.EndArray();
}

static void write_associative_container(const variant_associative_view& view, PrettyWriter<StringBuffer>& writer)
{
    static const string_view key_name("key");
    static const string_view value_name("value");

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
}

static void write_obj(const instance& obj2, PrettyWriter<StringBuffer>& writer)
{
    writer.StartObject();

    // Get the actual (derived) type of the instance.
    rttr::type actual_type = obj2.get_derived_type();
    std::string type_name = actual_type.get_name().to_string();

    // Write out a special property for the type.
    writer.String("@type");
    writer.String(type_name.c_str());

    // Now write the properties.
    // (Use the underlying non-wrapper object, if applicable.)
    bool is_wrapper = obj2.get_type().get_raw_type().is_wrapper();
    instance obj = is_wrapper ? obj2.get_wrapped_instance() : obj2;

    auto prop_list = obj.get_derived_type().get_properties();
    for (auto prop : prop_list)
    {
        if (prop.get_metadata("NO_SERIALIZE"))
            continue;

        variant prop_value = prop.get_value(obj);
        if (!prop_value)
            continue; // cannot serialize, because we cannot retrieve the value

        const auto name = prop.get_name();
        writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);
        if (!write_variant(prop_value, writer))
        {
            std::cerr << "cannot serialize property: " << name << std::endl;
        }
    }
    writer.EndObject();
}

static bool write_variant(const variant& var, PrettyWriter<StringBuffer>& writer)
{
    auto value_type = var.get_type();
    auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
    bool is_wrapper = wrapped_type != value_type;

    if (write_basic_types(is_wrapper ? wrapped_type : value_type,
        is_wrapper ? var.extract_wrapped_value() : var, writer))
    {
    }
    else if (var.is_sequential_container())
    {
        write_array(var.create_sequential_view(), writer);
    }
    else if (var.is_associative_container())
    {
        write_associative_container(var.create_associative_view(), writer);
    }
    else
    {
        auto child_props = is_wrapper ? wrapped_type.get_properties() : value_type.get_properties();
        if (!child_props.empty())
        {
            write_obj(var, writer);
        }
        else
        {
            bool ok = false;
            auto text = var.to_string(&ok);
            if (!ok)
            {
                writer.String(text);
                return false;
            }

            writer.String(text);
        }
    }

    return true;
}

bool Serial::Save(rttr::variant obj, StringView filename)
{
    if (!obj.is_valid())
        return false;

    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);

    write_variant(obj, writer);

    if (!File::Get().WriteText(filename, sb.GetString()))
        return false;

    return true;
}

static void read_variant(variant& var, Value& json_value);

static variant read_basic_types(Value& json_value)
{
    switch (json_value.GetType())
    {
    case kStringType:
    {
        return std::string(json_value.GetString());
        break;
    }
    case kNullType:     break;
    case kFalseType:
    case kTrueType:
    {
        return json_value.GetBool();
        break;
    }
    case kNumberType:
    {
        if (json_value.IsInt())
            return json_value.GetInt();
        else if (json_value.IsDouble())
            return json_value.GetDouble();
        else if (json_value.IsUint())
            return json_value.GetUint();
        else if (json_value.IsInt64())
            return json_value.GetInt64();
        else if (json_value.IsUint64())
            return json_value.GetUint64();
        break;
    }
    // we handle only the basic types here
    case kObjectType:
    case kArrayType: return variant();
    }

    return variant();
}

static variant read_value(Value::MemberIterator& itr, const type& t)
{
    auto& json_value = itr->value;
    variant extracted_value = read_basic_types(json_value);
    const bool could_convert = extracted_value.convert(t);
    if (!could_convert)
    {
        if (json_value.IsObject())
        {
            constructor ctor = t.get_constructor();
            for (auto& item : t.get_constructors())
            {
                if (item.get_instantiated_type() == t)
                    ctor = item;
            }
            extracted_value = ctor.invoke();
            read_variant(extracted_value, json_value);
        }
    }

    return extracted_value;
}

static void read_array(variant_sequential_view& view, Value& json_array_value)
{
    view.set_size(json_array_value.Size());
    const type array_value_type = view.get_rank_type(1);

    for (rapidjson::SizeType i = 0; i < json_array_value.Size(); ++i)
    {
        auto& json_index_value = json_array_value[i];
        if (json_index_value.IsArray())
        {
            auto sub_array_view = view.get_value(i).create_sequential_view();
            read_array(sub_array_view, json_index_value);
        }
        else if (json_index_value.IsObject())
        {
            variant var_tmp = view.get_value(i);
            variant extracted_value = read_basic_types(json_index_value);
            const bool could_convert = extracted_value.convert(var_tmp.get_type());
            if (!could_convert)
            {
                if (json_index_value.IsObject())
                {
                    constructor ctor = var_tmp.get_type().get_constructor();
                    for (auto& item : var_tmp.get_type().get_constructors())
                    {
                        if (item.get_instantiated_type() == var_tmp.get_type())
                            ctor = item;
                    }
                    extracted_value = ctor.invoke();
                    read_variant(extracted_value, json_index_value);
                }
            }

            if (extracted_value)
                view.set_value(i, extracted_value);
        }
        else
        {
            variant extracted_value = read_basic_types(json_index_value);
            if (extracted_value.convert(array_value_type))
                view.set_value(i, extracted_value);
        }
    }
}

static void read_associative_container(variant_associative_view& view, Value& json_array_value)
{
    for (rapidjson::SizeType i = 0; i < json_array_value.Size(); ++i)
    {
        auto& json_index_value = json_array_value[i];
        if (json_index_value.IsObject()) // key-value pair
        {
            Value::MemberIterator key_itr = json_index_value.FindMember("key");
            Value::MemberIterator value_itr = json_index_value.FindMember("value");

            if (key_itr != json_index_value.MemberEnd() &&
                value_itr != json_index_value.MemberEnd())
            {
                auto key_var = read_value(key_itr, view.get_key_type());
                auto value_var = read_value(value_itr, view.get_value_type());
                if (key_var && value_var)
                    view.insert(key_var, value_var);
            }
        }
        else // key-only associative container
        {
            variant extracted_value = read_basic_types(json_index_value);
            if (extracted_value && extracted_value.convert(view.get_key_type()))
                view.insert(extracted_value);
        }
    }
}

/// <summary>
/// Reads a variant from a JSON value. This function handles containers,
/// objects with type metadata, and basic types.
/// </summary>
static void read_variant(variant& var, Value& json_value)
{
    if (json_value.IsArray())
    {
        // If the variant is a container, handle sequential and associative cases.
        if (var.is_sequential_container())
        {
            auto view = var.create_sequential_view();
            read_array(view, json_value);
            return;
        }
        else if (var.is_associative_container())
        {
            auto view = var.create_associative_view();
            read_associative_container(view, json_value);
            return;
        }
    }
    else if (json_value.IsObject())
    {
        // Check for type metadata using the custom key (@type)
        if (json_value.HasMember("@type"))
        {
            std::string type_name = json_value["@type"].GetString();
            rttr::type actual_type = rttr::type::get_by_name(type_name);
            if (actual_type.is_valid() && actual_type != var.get_type())
            {
                variant new_instance = actual_type.create();
                if (new_instance.is_valid())
                    var = new_instance;
            }
        }

        // Treat the JSON object as a set of properties.
        instance inst = var;
        // If var is a wrapper, work on the wrapped instance.
        instance raw = var.get_type().get_raw_type().is_wrapper() ? inst.get_wrapped_instance() : inst;
        const auto prop_list = raw.get_derived_type().get_properties();
        for (auto prop : prop_list)
        {
            Value::MemberIterator it = json_value.FindMember(prop.get_name().data());
            if (it == json_value.MemberEnd())
                continue;
            const type prop_type = prop.get_type();
            variant prop_var = prop.get_value(raw);
            read_variant(prop_var, it->value);
            prop.set_value(raw, prop_var);
        }
        return;
    }
    else
    {
        // For basic types (string, bool, numbers, etc.)
        variant basic = read_basic_types(json_value);
        if (basic.convert(var.get_type()))
            var = basic;
    }
}

bool Serial::Load(rttr::variant obj, StringView filepath)
{
    auto json = File::Get().ReadText(filepath);
    Document document;  // Uses UTF8 and MemoryPoolAllocator by default.

    if (document.Parse(json.c_str()).HasParseError())
        return false;

    read_variant(obj, document);
    return true;
}
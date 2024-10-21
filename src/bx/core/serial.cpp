#include <bx/core/serial.hpp>

#include <bx/core/macros.hpp>

// TODO: Implementation below was taken from the examples of RTTR.
// This is a simple temporary solution that allows us to have serialization for now
// while the rest of the engine matures.
// Eventually this needs to be reworked to support different archives (at least binary),
// and hopefully have a more robust system (like cereal?).

#define RAPIDJSON_HAS_STDSTRING 1
#include <cereal/external/rapidjson/prettywriter.h>
#include <cereal/external/rapidjson/document.h>
#include <rttr/type>

/////////////////////////////////////////////////////////////////////////////////////////

using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

void to_json_recursively(const rttr::instance& obj, Writer& writer);

String Serializer::Save(rttr::instance obj)
{
    if (!obj.is_valid())
        return std::string();

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    to_json_recursively(obj, writer);

    return sb.GetString();
}

bool write_variant(const rttr::variant& var, Writer& writer);

bool write_atomic_types_to_json(const rttr::type& t, const rttr::variant& var, Writer& writer)
{
    if (t.is_arithmetic())
    {
        if (t == rttr::type::get<bool>())
            writer.Bool(var.to_bool());
        else if (t == rttr::type::get<char>())
            writer.Bool(var.to_bool());
        else if (t == rttr::type::get<int8_t>())
            writer.Int(var.to_int8());
        else if (t == rttr::type::get<int16_t>())
            writer.Int(var.to_int16());
        else if (t == rttr::type::get<int32_t>())
            writer.Int(var.to_int32());
        else if (t == rttr::type::get<int64_t>())
            writer.Int64(var.to_int64());
        else if (t == rttr::type::get<uint8_t>())
            writer.Uint(var.to_uint8());
        else if (t == rttr::type::get<uint16_t>())
            writer.Uint(var.to_uint16());
        else if (t == rttr::type::get<uint32_t>())
            writer.Uint(var.to_uint32());
        else if (t == rttr::type::get<uint64_t>())
            writer.Uint64(var.to_uint64());
        else if (t == rttr::type::get<float>())
            writer.Double(var.to_double());
        else if (t == rttr::type::get<double>())
            writer.Double(var.to_double());

        return true;
    }
    else if (t.is_enumeration())
    {
        bool ok = false;
        auto result = var.to_string(&ok);
        if (ok)
        {
            writer.String(var.to_string().c_str());
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
    else if (t == rttr::type::get<std::string>())
    {
        writer.String(var.to_string().c_str());
        return true;
    }

    return false;
}

static void write_array(const rttr::variant_sequential_view& view, Writer& writer)
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
            rttr::variant wrapped_var = item.extract_wrapped_value();
            rttr::type value_type = wrapped_var.get_type();
            if (value_type.is_arithmetic() || value_type == rttr::type::get<std::string>() || value_type.is_enumeration())
            {
                write_atomic_types_to_json(value_type, wrapped_var, writer);
            }
            else // object
            {
                to_json_recursively(wrapped_var, writer);
            }
        }
    }
    writer.EndArray();
}

static void write_associative_container(const rttr::variant_associative_view& view, Writer& writer)
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
}

bool write_variant(const rttr::variant& var, Writer& writer)
{
    auto value_type = var.get_type();
    auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
    bool is_wrapper = wrapped_type != value_type;

    if (write_atomic_types_to_json(is_wrapper ? wrapped_type : value_type,
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
            to_json_recursively(var, writer);
        }
        else
        {
            bool ok = false;
            auto text = var.to_string(&ok);
            if (!ok)
            {
                writer.String(text.c_str());
                return false;
            }

            writer.String(text.c_str());
        }
    }

    return true;
}

void to_json_recursively(const rttr::instance& obj2, Writer& writer)
{
    writer.StartObject();
    rttr::instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;

    auto prop_list = obj.get_derived_type().get_properties();
    for (auto prop : prop_list)
    {
        if (prop.get_metadata("NO_SERIALIZE"))
            continue;

        rttr::variant prop_value = prop.get_value(obj);
        if (!prop_value)
            continue; // cannot serialize, because we cannot retrieve the value

        const auto name = prop.get_name();
        writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);
        if (!write_variant(prop_value, writer))
        {
            BX_LOGE("Cannot serialize property: {}", name.data());
        }
    }

    writer.EndObject();
}


/////////////////////////////////////////////////////////////////////////////////////////

void fromjson_recursively(rttr::instance obj, rapidjson::Value& json_object);

rttr::instance Serializer::Load(const String& str)
{
    rttr::instance obj;
    rapidjson::Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.

    // "normal" parsing, decode strings to new buffers. Can use other input stream via ParseStream().
    if (document.Parse(str.c_str()).HasParseError())
    {
        BX_LOGE("Failed to parse: {}", str);
        return obj;
    }

    fromjson_recursively(obj, document);
    return obj;
}

rttr::variant extract_basic_types(rapidjson::Value& json_value)
{
    switch (json_value.GetType())
    {
    case rapidjson::kStringType:
    {
        return std::string(json_value.GetString());
        break;
    }
    case rapidjson::kNullType:     break;
    case rapidjson::kFalseType:
    case rapidjson::kTrueType:
    {
        return json_value.GetBool();
        break;
    }
    case rapidjson::kNumberType:
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
    case rapidjson::kObjectType:
    case rapidjson::kArrayType: return rttr::variant();
    }

    return rttr::variant();
}

static void write_array_recursively(rttr::variant_sequential_view& view, rapidjson::Value& json_array_value)
{
    view.set_size(json_array_value.Size());
    const rttr::type array_value_type = view.get_rank_type(1);

    for (SizeType i = 0; i < json_array_value.Size(); ++i)
    {
        auto& json_index_value = json_array_value[i];
        if (json_index_value.IsArray())
        {
            auto sub_array_view = view.get_value(i).create_sequential_view();
            write_array_recursively(sub_array_view, json_index_value);
        }
        else if (json_index_value.IsObject())
        {
            rttr::variant var_tmp = view.get_value(i);
            rttr::variant wrapped_var = var_tmp.extract_wrapped_value();
            fromjson_recursively(wrapped_var, json_index_value);
            view.set_value(i, wrapped_var);
        }
        else
        {
            rttr::variant extracted_value = extract_basic_types(json_index_value);
            if (extracted_value.convert(array_value_type))
                view.set_value(i, extracted_value);
        }
    }
}

rttr::variant extract_value(rapidjson::Value::MemberIterator& itr, const rttr::type& t)
{
    auto& json_value = itr->value;
    rttr::variant extracted_value = extract_basic_types(json_value);
    const bool could_convert = extracted_value.convert(t);
    if (!could_convert)
    {
        if (json_value.IsObject())
        {
            rttr::constructor ctor = t.get_constructor();
            for (auto& item : t.get_constructors())
            {
                if (item.get_instantiated_type() == t)
                    ctor = item;
            }
            extracted_value = ctor.invoke();
            fromjson_recursively(extracted_value, json_value);
        }
    }

    return extracted_value;
}

static void write_associative_view_recursively(rttr::variant_associative_view& view, rapidjson::Value& json_array_value)
{
    for (SizeType i = 0; i < json_array_value.Size(); ++i)
    {
        auto& json_index_value = json_array_value[i];
        if (json_index_value.IsObject()) // a key-value associative view
        {
            rapidjson::Value::MemberIterator key_itr = json_index_value.FindMember("key");
            rapidjson::Value::MemberIterator value_itr = json_index_value.FindMember("value");

            if (key_itr != json_index_value.MemberEnd() &&
                value_itr != json_index_value.MemberEnd())
            {
                auto key_var = extract_value(key_itr, view.get_key_type());
                auto value_var = extract_value(value_itr, view.get_value_type());
                if (key_var && value_var)
                {
                    view.insert(key_var, value_var);
                }
            }
        }
        else // a key-only associative view
        {
            rttr::variant extracted_value = extract_basic_types(json_index_value);
            if (extracted_value && extracted_value.convert(view.get_key_type()))
                view.insert(extracted_value);
        }
    }
}

void fromjson_recursively(rttr::instance obj2, rapidjson::Value& json_object)
{
    rttr::instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
    const auto prop_list = obj.get_derived_type().get_properties();

    for (auto prop : prop_list)
    {
        rapidjson::Value::MemberIterator ret = json_object.FindMember(prop.get_name().data());
        if (ret == json_object.MemberEnd())
            continue;
        const rttr::type value_t = prop.get_type();

        auto& json_value = ret->value;
        switch (json_value.GetType())
        {
        case rapidjson::kArrayType:
        {
            rttr::variant var;
            if (value_t.is_sequential_container())
            {
                var = prop.get_value(obj);
                auto view = var.create_sequential_view();
                write_array_recursively(view, json_value);
            }
            else if (value_t.is_associative_container())
            {
                var = prop.get_value(obj);
                auto associative_view = var.create_associative_view();
                write_associative_view_recursively(associative_view, json_value);
            }

            prop.set_value(obj, var);
            break;
        }
        case rapidjson::kObjectType:
        {
            rttr::variant var = prop.get_value(obj);
            fromjson_recursively(var, json_value);
            prop.set_value(obj, var);
            break;
        }
        default:
        {
            rttr::variant extracted_value = extract_basic_types(json_value);
            if (extracted_value.convert(value_t)) // REMARK: CONVERSION WORKS ONLY WITH "const type", check whether this is correct or not!
                prop.set_value(obj, extracted_value);
        }
        }
    }
}
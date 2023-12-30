/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#define SRV_CLASS "srv::Parameters"

#include "parameters.h"
#include "types.h"

namespace srv {

    std::vector<flatbuffers::Offset<FBS::RtpParameters::Parameter>> Parameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        std::vector<flatbuffers::Offset<FBS::RtpParameters::Parameter>> parameters;

        for (const auto& kv : this->mapKeyValues) {
            const auto& key   = kv.first;
            const auto& value = kv.second;

            flatbuffers::Offset<FBS::RtpParameters::Parameter> parameter;

            switch (value.type) {
                case Value::Type::BOOLEAN: {
                    auto valueOffset = FBS::RtpParameters::CreateBoolean(builder, value.booleanValue);
                    parameter = FBS::RtpParameters::CreateParameterDirect(builder, key.c_str(), FBS::RtpParameters::Value::Boolean, valueOffset.Union());
                    break;
                }

                case Value::Type::INTEGER: {
                    auto valueOffset = FBS::RtpParameters::CreateInteger32(builder, value.integerValue);
                    parameters.emplace_back(FBS::RtpParameters::CreateParameterDirect(builder, key.c_str(), FBS::RtpParameters::Value::Integer32, valueOffset.Union()));
                    break;
                }

                case Value::Type::DOUBLE: {
                    auto valueOffset = FBS::RtpParameters::CreateDouble(builder, value.doubleValue);
                    parameters.emplace_back(FBS::RtpParameters::CreateParameterDirect(builder, key.c_str(), FBS::RtpParameters::Value::Double, valueOffset.Union()));
                    break;
                }

                case Value::Type::STRING: {
                    auto valueOffset = FBS::RtpParameters::CreateStringDirect(builder, value.stringValue.c_str());
                    parameters.emplace_back(FBS::RtpParameters::CreateParameterDirect(builder, key.c_str(), FBS::RtpParameters::Value::String, valueOffset.Union()));
                    break;
                }

                case Value::Type::ARRAY_OF_INTEGERS: {
                    auto valueOffset = FBS::RtpParameters::CreateInteger32ArrayDirect(builder, &value.arrayOfIntegers);
                    parameters.emplace_back(FBS::RtpParameters::CreateParameterDirect(builder, key.c_str(), FBS::RtpParameters::Value::Integer32Array, valueOffset.Union()));
                    break;
                }
            }
        }

        return parameters;
    }

    void Parameters::Set(const flatbuffers::Vector<flatbuffers::Offset<FBS::RtpParameters::Parameter>>* data)
    {
        for (const auto* parameter : *data) {
            const auto key = parameter->name()->str();

            switch (parameter->value_type()) {
                case FBS::RtpParameters::Value::Boolean: {
                    this->mapKeyValues.emplace(key, Value((parameter->value_as_Boolean()->value() == 0) ? false : true));
                    break;
                }

                case FBS::RtpParameters::Value::Integer32: {
                    this->mapKeyValues.emplace(key, Value(parameter->value_as_Integer32()->value()));
                    break;
                }

                case FBS::RtpParameters::Value::Double: {
                    this->mapKeyValues.emplace(key, Value(parameter->value_as_Double()->value()));
                    break;
                }

                case FBS::RtpParameters::Value::String: {
                    this->mapKeyValues.emplace(key, Value(parameter->value_as_String()->value()->str()));
                    break;
                }

                case FBS::RtpParameters::Value::Integer32Array: {
                    this->mapKeyValues.emplace(key, Value(parameter->value_as_Integer32Array()->value()));
                    break;
                }

                default:; // Just ignore other value types.
            }
        }
    }

    void Parameters::serialize(nlohmann::json& jsonObject) const
    {
        // Force it to be an object even if no key/values are added below.
        jsonObject = nlohmann::json::object();

        for (auto& kv : this->mapKeyValues) {
            auto& key   = kv.first;
            auto& value = kv.second;

            switch (value.type) {
                case Value::Type::BOOLEAN:
                    jsonObject[key] = value.booleanValue;
                    break;
                case Value::Type::INTEGER:
                    jsonObject[key] = value.integerValue;
                    break;
                case Value::Type::DOUBLE:
                    jsonObject[key] = value.doubleValue;
                    break;
                case Value::Type::STRING:
                    jsonObject[key] = value.stringValue;
                    break;
                case Value::Type::ARRAY_OF_INTEGERS:
                    jsonObject[key] = value.arrayOfIntegers;
                    break;
            }
        }
    }

    void Parameters::Set(nlohmann::json& data)
    {
        SRV_ASSERT(data.is_object(), "data is not an object");

        for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
            const std::string& key = it.key();
            auto& value= it.value();

            switch (value.type()) {
                case nlohmann::json::value_t::boolean: {
                    this->mapKeyValues.emplace(key, Value(value.get<bool>()));
                    break;
                }

                case nlohmann::json::value_t::number_integer:
                case nlohmann::json::value_t::number_unsigned: {
                    this->mapKeyValues.emplace(key, Value(value.get<int32_t>()));
                    break;
                }

                case nlohmann::json::value_t::number_float: {
                    this->mapKeyValues.emplace(key, Value(value.get<double>()));
                    break;
                }

                case nlohmann::json::value_t::string: {
                    this->mapKeyValues.emplace(key, Value(value.get<std::string>()));
                    break;
                }

                case nlohmann::json::value_t::array: {
                    std::vector<int32_t> arrayOfIntegers;
                    bool isValid = true;

                    for (auto& entry : value) {
                        if (!entry.is_number_integer()) {
                            isValid = false;
                            break;
                        }
                        arrayOfIntegers.emplace_back(entry.get<int32_t>());
                    }

                    if (!arrayOfIntegers.empty() && isValid) {
                        this->mapKeyValues.emplace(key, Value(arrayOfIntegers));
                    }
                    break;
                }

                default:; // Just ignore other value types.
            }
        }
    }

    bool Parameters::HasBoolean(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::BOOLEAN;
    }

    bool Parameters::HasInteger(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::INTEGER;
    }

    bool Parameters::HasPositiveInteger(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::INTEGER && value.integerValue >= 0;
    }

    bool Parameters::HasDouble(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::DOUBLE;
    }

    bool Parameters::HasString(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::STRING;
    }

    bool Parameters::HasArrayOfIntegers(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;

        return value.type == Value::Type::ARRAY_OF_INTEGERS;
    }

    bool Parameters::IncludesInteger(const std::string& key, int32_t integer) const
    {
        auto it = this->mapKeyValues.find(key);

        if (it == this->mapKeyValues.end())
        {
            return false;
        }

        const auto& value = it->second;
        const auto& array = value.arrayOfIntegers;

        return std::find(array.begin(), array.end(), integer) != array.end();
    }

    bool Parameters::GetBoolean(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        SRV_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

        const auto& value = it->second;

        return value.booleanValue;
    }

    int32_t Parameters::GetInteger(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        SRV_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

        const auto& value = it->second;

        return value.integerValue;
    }

    double Parameters::GetDouble(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        SRV_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

        const auto& value = it->second;

        return value.doubleValue;
    }

    const std::string& Parameters::GetString(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        SRV_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

        const auto& value = it->second;

        return value.stringValue;
    }

    const std::vector<int32_t>& Parameters::GetArrayOfIntegers(const std::string& key) const
    {
        auto it = this->mapKeyValues.find(key);

        SRV_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

        const auto& value = it->second;

        return value.arrayOfIntegers;
    }
}

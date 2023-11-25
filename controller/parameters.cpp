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

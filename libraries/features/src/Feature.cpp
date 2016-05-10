////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Feature.cpp (features)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Feature.h"
#include "InputFeature.h"
#include "StringUtil.h"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace features
{
    //
    // feature base class
    //
    int Feature::_instanceCount = 0;
    std::unordered_map<std::string, Feature::DeserializeFunction> Feature::_createTypeMap;

    Feature::Feature() : _isDirty(true)
    {
        using std::to_string;
        // create id
        _instanceId = _instanceCount;
        ++_instanceCount;
        _id = "f_" + to_string(_instanceId);
    }
    
    Feature::Feature(std::string id) : _isDirty(true), _id(id)
    {
        ++_instanceCount;
    }

    Feature::Feature(const std::vector<std::shared_ptr<Feature>>& inputs) : Feature()
    {
        _inputFeatures = inputs;
    }

    Feature::Feature(std::string id, const std::vector<std::shared_ptr<Feature>>& inputs) : Feature(id)
    {
        _inputFeatures = inputs;
    }

    std::string Feature::Id() const
    {
        return _id;
    }

    size_t Feature::NumColumns() const
    {
        return _numColumns;
    }

    bool Feature::HasOutput() const
    {
        return !IsDirty();
    }

    std::vector<double> Feature::GetOutput() const
    {
        if (IsDirty() || _cachedValue.size() == 0)
        {
            _cachedValue = ComputeOutput();
            _isDirty = false; // Note: we don't call SetDirtyFlag(false) here, as that will start a cascade of SetDirtyFlag calls
        }

        return _cachedValue;
    }

    void Feature::Reset()
    {
        SetDirtyFlag(true);
        for (auto& f : _dependents)
        {
            f->Reset();
        }
    }

    size_t Feature::WarmupTime() const
    {
        size_t maxTime = 0;
        for (const auto& inputFeature : _inputFeatures)
        {
            maxTime = std::max(maxTime, inputFeature->WarmupTime());
        }
        return maxTime;
    }

    std::vector<std::string> Feature::GetDescription() const
    {
        std::vector<std::string> result;
        result.reserve(_inputFeatures.size() + 2);

        // Write out our id and type
        result.push_back(Id());
        result.push_back(FeatureType());

        // Write out ids of everybody I depend on
        for (const auto& inputFeature : _inputFeatures)
        {
            result.push_back(inputFeature->Id());
        }
        
        // Now add subclass-specific parts
        AddToDescription(result);
        return result;
    }

    std::vector<std::string> Feature::GetColumnDescriptions() const
    {
        using std::to_string;
        std::vector<std::string> result;
        for (int index = 0; index < NumColumns(); index++)
        {
            result.push_back(FeatureType() + "_" + to_string(index));
        }
        return result;
    }

    const std::vector<std::shared_ptr<Feature>>& Feature::GetInputFeatures() const
    {
        return _inputFeatures;
    }

    std::vector<std::string> Feature::GetRegisteredTypes()
    {
        std::vector<std::string> result;
        for (const auto& entry : _createTypeMap)
        {
            result.push_back(entry.first);
        }
        return result;
    }

    void Feature::RegisterDeserializeFunction(std::string class_name, DeserializeFunction create_fn)
    {
        _createTypeMap[class_name] = create_fn;
    }

    void Feature::AddDependent(std::shared_ptr<Feature> f)
    {
        _dependents.push_back(f);
    }

    bool Feature::IsDirty() const
    {
        return _isDirty;
    }

    void Feature::SetDirtyFlag(bool dirty) const
    {
        _isDirty = dirty;
        if (dirty)
        {
            for (auto& f : _dependents)
            {
                assert(f != nullptr);
                f->SetDirtyFlag(true);
            }
        }
    }

    void Feature::AddInputFeature(std::shared_ptr<Feature> inputFeature)
    {
        _inputFeatures.push_back(inputFeature);
    }
    
    void Feature::Serialize(std::ostream& outStream) const
    {
        bool first = true;
        for (auto s : GetDescription())
        {
            if (!first)
            {
                outStream << '\t';
            }
            else
            {
                first = false;
            }
            outStream << s;
        }
        outStream << '\n';
    }

    // searches recursively through input features and returns first InputFeature it finds
    // returns nullptr if it can't find one
    std::shared_ptr<InputFeature> Feature::FindInputFeature() const
    {
        for (const auto& inputFeature : _inputFeatures)
        {
            auto m = std::dynamic_pointer_cast<InputFeature>(inputFeature);
            if (m != nullptr)
            {
                return m;
            }
            else
            {
                return inputFeature->FindInputFeature();
            }
        }

        return nullptr;
    }

    std::shared_ptr<Feature> Feature::FromDescription(const std::vector<std::string>& description, Feature::FeatureMap& deserializedFeatureMap)
    {
        std::string featureId = TrimString(description[0]);
        std::string featureClass = TrimString(description[1]);

        auto createFunction = _createTypeMap[featureClass];
        if (createFunction == nullptr)
        {
            std::string error = std::string("Error deserializing feature description: unknown feature type '") + featureClass + "'";
            throw std::runtime_error(error);
        }
        return createFunction(description, deserializedFeatureMap);
    }
}

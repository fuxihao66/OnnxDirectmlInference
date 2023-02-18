// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{

class DmlOperatorInstanceNormalization
{
public:
    DmlOperatorInstanceNormalization(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap,
                                     ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    {
        m_input = expressionMap[node.inputNames[0]];
        m_weight = expressionMap[node.inputNames[1]];
        m_bias = expressionMap[node.inputNames[2]];
        
        CheckReference(initializerMap, node.inputNames[0]);
        CheckReference(initializerMap, node.inputNames[1]);
        CheckReference(initializerMap, node.inputNames[2]);


        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;
        weightShape = m_weight.GetOutputDesc().sizes;
        
        {
            //std::vector<char> temp;
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("epsilon", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid()){
                memcpy(&epsilon, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                epsilon = 1e-5f;
            }
        }
        
        axes.resize(inputShape.size() - 2);
        std::iota(axes.begin(), axes.end(), 2u); // if input is nxcxhxw, then axes is [2,3]
        
        // operatorDesc.FusedActivation = fusedActivation ? &fusedActivationDmlDesc : nullptr;

    }
    dml::Expression Create(){

        return dml::MeanVarianceNormalization(m_input, 
                                              std::optional(dml::Reinterpret(m_weight, dml::TensorDimensions{1, weightShape[0], 1, 1}, std::nullopt)), // reshape tensor from 1d to 4d
                                              std::optional(dml::Reinterpret(m_bias, dml::TensorDimensions{1, weightShape[0], 1, 1}, std::nullopt)),
                                              axes, true, epsilon);
    }
private:
    dml::Expression m_input;
    dml::Expression m_weight;
    dml::Expression m_bias;
    std::vector<uint32_t> axes;
    dml::TensorDimensions weightShape;
    // bool normalizeVariance;
    float epsilon;

};

DML_OP_DEFINE_CREATION_FUNCTION(InstanceNormalization, DmlOperatorInstanceNormalization);
// DML_OP_DEFINE_CREATION_FUNCTION(FusedInstanceNormalization, DmlOperatorInstanceNormalization);

} // namespace ODI

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "OperatorRegistration.h"


namespace ODI
{

class DmlOperatorBatchNormalization
{
public:
    DmlOperatorBatchNormalization(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap,
                                     ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    {
        m_input = expressionMap[node.inputNames[0]];
        m_scale = expressionMap[node.inputNames[1]];
        m_bias = expressionMap[node.inputNames[2]];
        m_mean = expressionMap[node.inputNames[3]];
        m_var = expressionMap[node.inputNames[4]];

        CheckReference(initializerMap, node.inputNames[0]);
        CheckReference(initializerMap, node.inputNames[1]);
        CheckReference(initializerMap, node.inputNames[2]);
        CheckReference(initializerMap, node.inputNames[3]);
        CheckReference(initializerMap, node.inputNames[4]);


        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;
        weightShape = m_scale.GetOutputDesc().sizes;
        
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
        {
            //std::vector<char> temp;
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("momentum", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid()) {
                memcpy(&momentum, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else {
                momentum = 0.9f;
            }
        }
        
        //axes.resize(inputShape.size() - 2);
        //std::iota(axes.begin(), axes.end(), 2u); // if input is nxcxhxw, then axes is [2,3]
        
        // operatorDesc.FusedActivation = fusedActivation ? &fusedActivationDmlDesc : nullptr;

    }
    dml::Expression Create(){

        //return dml::MeanVarianceNormalization(m_input, 
        //                                      std::optional(dml::Reinterpret(m_weight, dml::TensorDimensions{1, weightShape[0], 1, 1}, std::nullopt)), // reshape tensor from 1d to 4d
        //                                      std::optional(dml::Reinterpret(m_bias, dml::TensorDimensions{1, weightShape[0], 1, 1}, std::nullopt)),
        //                                      axes, true, epsilon);

        return dml::BatchNormalization(
            m_input,
            dml::Reinterpret(m_mean, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_var, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_scale, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_bias, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            true,
            epsilon);
    }
protected:
    dml::Expression m_input;
    dml::Expression m_scale;
    dml::Expression m_bias;
    dml::Expression m_mean;
    dml::Expression m_var;
    dml::TensorDimensions weightShape;
    // bool normalizeVariance;
    float epsilon;
    float momentum; // will not be used for inference
};

class DmlOperatorFusedBatchNormalization : public DmlOperatorBatchNormalization
{
public:
    DmlOperatorFusedBatchNormalization(
        std::map<std::string, dml::Expression>& expressionMap, std::map<std::string,
        ONNX_PARSER::InitializerTensorInfo>& initializerMap,
        ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
        : DmlOperatorBatchNormalization(expressionMap, initializerMap, node, graph, opsetVersion) {

        ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("fused_activation", ONNX_PARSER::AttributeType::STRING);
        if (attriWrapper.isValid()) {
            fusedActivationType.resize(attriWrapper.getValue().size());
            memcpy(fusedActivationType.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
        }
        else {
            fusedActivationType = "Relu";
        }
    }

    dml::Expression Create() {
        dml::FusedActivation activation = dml::FusedActivation::None();
        if (fusedActivationType == "Relu") {
            activation = dml::FusedActivation::Relu();
        }
        else if (fusedActivationType == "Sigmoid") {
            activation = dml::FusedActivation::Sigmoid();
        }


        return dml::BatchNormalization(
            m_input,
            dml::Reinterpret(m_mean, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_var, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_scale, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            dml::Reinterpret(m_bias, dml::TensorDimensions{ 1, weightShape[0], 1, 1 }, std::nullopt),
            true,
            epsilon,
            activation);
    }
private:
    std::string fusedActivationType;
};

DML_OP_DEFINE_CREATION_FUNCTION(BatchNormalization, DmlOperatorBatchNormalization);
DML_OP_DEFINE_CREATION_FUNCTION(DmlFusedBatchNormalization, DmlOperatorFusedBatchNormalization);
// DML_OP_DEFINE_CREATION_FUNCTION(FusedInstanceNormalization, DmlOperatorInstanceNormalization);

} // namespace ODI

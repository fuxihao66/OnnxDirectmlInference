// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "OperatorRegistration.h"

namespace ODI
{

template<DML_OPERATOR_TYPE operatorType>
class DmlOperatorActivation
{
public:
    DmlOperatorActivation(
        std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion
        )
    {
        //std::vector<char> attribVal;
        
        switch (operatorType)
        {
        case DML_OPERATOR_ACTIVATION_ELU:
        case DML_OPERATOR_ACTIVATION_CELU:
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 1.0f;
            break;
        }
        // case DML_OPERATOR_ACTIVATION_SOFTMAX:
        // case DML_OPERATOR_ACTIVATION_LOG_SOFTMAX:
        // case DML_OPERATOR_ACTIVATION_HARDMAX:
        // case DML_OPERATOR_ACTIVATION_SOFTMAX1:
        // case DML_OPERATOR_ACTIVATION_LOG_SOFTMAX1:
        // case DML_OPERATOR_ACTIVATION_HARDMAX1:
        case DML_OPERATOR_ACTIVATION_HARD_SIGMOID:
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 0.2f;
            attriWrapper = node.GetAttribute("beta", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param1, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else   
                Param1 = 0.5f;
            break;
        }
        case DML_OPERATOR_ACTIVATION_LEAKY_RELU:
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 0.01f;
            break;
        }
        // case DML_OPERATOR_ACTIVATION_LINEAR: // TODO: NOT USED
        //     bool hasAlpha = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT, attribVal);
        //     if (hasAlpha)
        //         memcpy(&operatorDesc.linear.Alpha, attribVal.data(), attribVal.size());
        //     else
        //         operatorDesc.linear.Alpha = 0.2f;
        //     bool hasBeta = node.GetAttribute("beta", ONNX_PARSER::AttributeType::FLOAT, attribVal);
        //     if (hasBeta)
        //         memcpy(&operatorDesc.linear.Beta, attribVal.data(), attribVal.size());
        //     else   
        //         operatorDesc.linear.Beta = 0.5f;
        //     break;
        // case DML_OPERATOR_ACTIVATION_PARAMETRIC_SOFTPLUS:  // TODO: NOT USED
        //     operatorDesc.parametricSoftplus.Alpha = ;
        //     operatorDesc.parametricSoftplus.Beta  = ;
        //     break;
        case DML_OPERATOR_ACTIVATION_SCALED_ELU:
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 1.67326f;
            attriWrapper = node.GetAttribute("gamma", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param1, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else   
                Param1 = 1.0507f;
            break;
        }

        // case DML_OPERATOR_ACTIVATION_SCALED_TANH: // TODO: NOT USED
        //     operatorDesc.scaledTanh.Alpha = ;
        //     operatorDesc.scaledTanh.Beta  = ;
        //     break;

        case DML_OPERATOR_ACTIVATION_SOFTPLUS:
        {
            Param0 = 1.0f;
            break;
        }
        case DML_OPERATOR_ACTIVATION_THRESHOLDED_RELU:
        {    
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("alpha", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 1.f;
            break;
        }
        case DML_OPERATOR_ACTIVATION_SHRINK:
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("bias", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param0, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else
                Param0 = 0.f;
            attriWrapper = node.GetAttribute("lambd", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid())
                memcpy(&Param1, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            else   
                Param1 = 0.5f;
            break;
        }
        case DML_OPERATOR_ACTIVATION_IDENTITY:
        case DML_OPERATOR_ACTIVATION_PARAMETERIZED_RELU:
        case DML_OPERATOR_ACTIVATION_RELU:
        case DML_OPERATOR_ACTIVATION_SIGMOID:
        case DML_OPERATOR_ACTIVATION_TANH:
        case DML_OPERATOR_ACTIVATION_SOFTSIGN:
        // case DML_OPERATOR_ACTIVATION_GELU:
            break;

        default:
            assert(false);
            break;
        }

        //gsl::span<const uint32_t> outputSizes = m_outputTensorDescs[0].GetSizes();
        std::vector<DML_TENSOR_DESC> inputDescs;
        std::vector<DML_TENSOR_DESC> outputDescs;

        

        m_input = expressionMap[node.inputNames[0]];

        CheckReference(initializerMap, node.inputNames[0]);


        dml::detail::GraphBuilder* builder = m_input.Impl()->GetGraphBuilder();

		dml::TensorDesc inputTensor = m_input.Impl()->GetOutputDesc();
		dml::TensorDesc outputTensor = dml::TensorDesc(inputTensor.dataType, inputTensor.sizes, builder->GetTensorPolicy());

        if (operatorType == DML_OPERATOR_ACTIVATION_PARAMETERIZED_RELU)
        {
            *m_slope = expressionMap[node.inputNames[1]];

            CheckReference(initializerMap, node.inputNames[1]);
        }
        else // All other activation descrptions are equivalent to Elu in layout.
        {
            m_slope = std::nullopt;
        }

    }
    dml::Expression Create(){ // TODO: Previous Implementation somehow fails, but I dont know why (compiler flagH) 
        /*dml::detail::GraphBuilder* builder = m_input.Impl()->GetGraphBuilder();

        dml::detail::NodeID node;
        if (m_slope) {
            dml::detail::NodeOutput* const inputs[] = { m_input.Impl(), m_slope->Impl() };
            node = builder->CreateOperatorNode(operatorType, &operatorDesc, inputs);
        }
        else {
            dml::detail::NodeOutput* const inputs[] = { m_input.Impl() };
            node = builder->CreateOperatorNode(operatorType, &operatorDesc, inputs);
        }

        dml::TensorDesc inputTensor = m_input.Impl()->GetOutputDesc();
        dml::TensorDesc outputTensor(inputTensor.dataType, inputTensor.sizes, builder->GetTensorPolicy());
        dml::detail::NodeOutput* output = builder->CreateNodeOutput(node, 0, std::move(outputTensor));

        return output;*/

        switch (operatorType) {
        case DML_OPERATOR_ACTIVATION_ELU:
            return dml::ActivationElu(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_CELU:
            return dml::ActivationCelu(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_HARD_SIGMOID:
            return dml::ActivationHardSigmoid(m_input, Param0, Param1);
        case DML_OPERATOR_ACTIVATION_LEAKY_RELU:
            return dml::ActivationLeakyRelu(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_SCALED_ELU:
            return dml::ActivationScaledElu(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_SOFTPLUS:
            return dml::ActivationSoftplus(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_THRESHOLDED_RELU:
            return dml::ActivationThresholdedRelu(m_input, Param0);
        case DML_OPERATOR_ACTIVATION_SHRINK:
            return dml::ActivationShrink(m_input, Param0, Param1);
        case DML_OPERATOR_ACTIVATION_IDENTITY:
            return dml::ActivationIdentity(m_input);
        case DML_OPERATOR_ACTIVATION_PARAMETERIZED_RELU:
            return dml::ActivationParameterizedRelu(m_input, *m_slope);
        case DML_OPERATOR_ACTIVATION_RELU:
            return dml::ActivationRelu(m_input);
        case DML_OPERATOR_ACTIVATION_SIGMOID:
            return dml::ActivationSigmoid(m_input);
        case DML_OPERATOR_ACTIVATION_TANH:
            return dml::ActivationTanh(m_input);
        case DML_OPERATOR_ACTIVATION_SOFTSIGN:
            return dml::ActivationSoftsign(m_input);
        default: // Default is Relu
            return dml::ActivationRelu(m_input);
        }

        
    }
private:
    dml::Expression m_input;
    std::optional<dml::Expression> m_slope;
    float Param0;
    float Param1;
    //ActivationOperatorDescUnion operatorDesc;
    /*dml::TensorDesc inputTensor;
    dml::TensorDesc outputTensor;*/
    // DML_OPERATOR_TYPE remappedOperatorType(const DML_OPERATOR_TYPE operatorType) const {
    //     switch (operatorType)
    //     {
    //         case DML_OPERATOR_ACTIVATION_HARDMAX:
    //             return DML_OPERATOR_ACTIVATION_HARDMAX1;
    //         case DML_OPERATOR_ACTIVATION_SOFTMAX:
    //             return DML_OPERATOR_ACTIVATION_SOFTMAX1;
    //         case DML_OPERATOR_ACTIVATION_LOG_SOFTMAX:
    //             return DML_OPERATOR_ACTIVATION_LOG_SOFTMAX1;
    //         default:
    //             return operatorType;
    //     }
    // }
};


DML_OP_DEFINE_CREATION_FUNCTION(Sigmoid,             DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SIGMOID>);
DML_OP_DEFINE_CREATION_FUNCTION(HardSigmoid,         DmlOperatorActivation<DML_OPERATOR_ACTIVATION_HARD_SIGMOID>);
DML_OP_DEFINE_CREATION_FUNCTION(Tanh,                DmlOperatorActivation<DML_OPERATOR_ACTIVATION_TANH>);
// DML_OP_DEFINE_CREATION_FUNCTION(ScaledTanh,          DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SCALED_TANH>);
DML_OP_DEFINE_CREATION_FUNCTION(Relu,                DmlOperatorActivation<DML_OPERATOR_ACTIVATION_RELU>);
DML_OP_DEFINE_CREATION_FUNCTION(Celu,                DmlOperatorActivation<DML_OPERATOR_ACTIVATION_CELU>);
DML_OP_DEFINE_CREATION_FUNCTION(LeakyRelu,           DmlOperatorActivation<DML_OPERATOR_ACTIVATION_LEAKY_RELU>);
DML_OP_DEFINE_CREATION_FUNCTION(PRelu,               DmlOperatorActivation<DML_OPERATOR_ACTIVATION_PARAMETERIZED_RELU>);
DML_OP_DEFINE_CREATION_FUNCTION(ThresholdedRelu,     DmlOperatorActivation<DML_OPERATOR_ACTIVATION_THRESHOLDED_RELU>);
DML_OP_DEFINE_CREATION_FUNCTION(Elu,                 DmlOperatorActivation<DML_OPERATOR_ACTIVATION_ELU>);
DML_OP_DEFINE_CREATION_FUNCTION(Selu,                DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SCALED_ELU>);
DML_OP_DEFINE_CREATION_FUNCTION(Softsign,            DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SOFTSIGN>);
DML_OP_DEFINE_CREATION_FUNCTION(Softplus,            DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SOFTPLUS>);
// DML_OP_DEFINE_CREATION_FUNCTION(ParametricSoftplus,  DmlOperatorActivation<DML_OPERATOR_ACTIVATION_PARAMETRIC_SOFTPLUS>);
DML_OP_DEFINE_CREATION_FUNCTION(Dropout,             DmlOperatorActivation<DML_OPERATOR_ACTIVATION_IDENTITY>);
// DML_OP_DEFINE_CREATION_FUNCTION(Softmax,             DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SOFTMAX>);
// // DML_OP_DEFINE_CREATION_FUNCTION(Softmax13,           DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SOFTMAX1>);
// DML_OP_DEFINE_CREATION_FUNCTION(LogSoftmax,          DmlOperatorActivation<DML_OPERATOR_ACTIVATION_LOG_SOFTMAX>);
// DML_OP_DEFINE_CREATION_FUNCTION(LogSoftmax13,        DmlOperatorActivation<DML_OPERATOR_ACTIVATION_LOG_SOFTMAX1>);
// DML_OP_DEFINE_CREATION_FUNCTION(Hardmax,             DmlOperatorActivation<DML_OPERATOR_ACTIVATION_HARDMAX>);
// DML_OP_DEFINE_CREATION_FUNCTION(Hardmax13,           DmlOperatorActivation<DML_OPERATOR_ACTIVATION_HARDMAX1>);
DML_OP_DEFINE_CREATION_FUNCTION(Shrink,              DmlOperatorActivation<DML_OPERATOR_ACTIVATION_SHRINK>);
// DML_OP_DEFINE_CREATION_FUNCTION(Gelu,                DmlOperatorActivation<DML_OPERATOR_ACTIVATION_GELU>);

} // namespace ODI

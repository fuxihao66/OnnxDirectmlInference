// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//#include "precomp.h"
#include "OperatorRegistration.h"
#include <vector>


namespace ODI
{

class DmlOperatorGather 
{
public:
    DmlOperatorGather(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap,
                      ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    {
        if (node.inputNames.size() != 2)
            throw std::exception("Gather expects 2 inputs!");

        m_input = expressionMap[node.inputNames[0]];
        m_indices = expressionMap[node.inputNames[1]];
        CheckReference(initializerMap, node.inputNames[0]);
        CheckReference(initializerMap, node.inputNames[1]);

        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;
        dml::TensorDimensions indicesShape = m_indices.GetOutputDesc().sizes;
        
        int tempaxis;
        {
            //std::vector<char> temp;
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("axis", ONNX_PARSER::AttributeType::INT);
            if (attriWrapper.isValid()){
                memcpy(&tempaxis, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else
                tempaxis = 0;
        }
        if (tempaxis < 0)
            axis = inputShape.size() + tempaxis;
        else
            axis = tempaxis;

        indexDimensions = indicesShape.size(); 

        paddedIndicesSize.resize(inputShape.size());
        for (int i = 0; i < inputShape.size() - indicesShape.size(); i++) {
            paddedIndicesSize[i] = 1;
        }
        int index = 0;
        for (int i = inputShape.size() - indicesShape.size(); i < paddedIndicesSize.size(); i++) {
            paddedIndicesSize[i] = indicesShape[index++];
        }
    }
    dml::Expression Create(){
        // DML Gather requires the rank of indices being the same as input 
        return dml::Gather(m_input, dml::Reinterpret( m_indices, paddedIndicesSize, std::nullopt), axis, indexDimensions);
    }
private:
    dml::Expression m_input;
    dml::Expression m_indices;
    uint32_t axis;
    uint32_t indexDimensions;
    dml::TensorDimensions paddedIndicesSize;
};

// class DmlOperatorGatherElements : public DmlOperator
// {
// public:
//     DmlOperatorGatherElements(const MLOperatorKernelCreationContext& kernelCreationContext)
//     :   DmlOperator(kernelCreationContext)
//     {
//         ML_CHECK_VALID_ARGUMENT(kernelCreationContext.GetInputCount() == 2, "GatherElements expects 2 inputs.");
//         ML_CHECK_VALID_ARGUMENT(kernelCreationContext.GetOutputCount() == 1, "GatherElements expects 1 output.");

//         auto tensorShapeDescription = kernelCreationContext.GetTensorShapeDescription();
//         std::vector<DimensionType> dataDimensions = tensorShapeDescription.GetInputTensorShape(0);
//         std::vector<DimensionType> indicesDimensions = tensorShapeDescription.GetInputTensorShape(1);
//         std::vector<DimensionType> outputDimensions = tensorShapeDescription.GetOutputTensorShape(0);

//         size_t dimensionCountMax = std::max({dataDimensions.size(), indicesDimensions.size(), outputDimensions.size()});
//         DmlOperator::Initialize(kernelCreationContext, gsl::narrow_cast<uint32_t>(dimensionCountMax));

//         std::vector<DML_TENSOR_DESC> inputDescs = GetDmlInputDescs();
//         std::vector<DML_TENSOR_DESC> outputDescs = GetDmlOutputDescs();
//         assert(inputDescs.size() == 2);
//         assert(outputDescs.size() == 1);

//         int32_t signedOnnxAxis = kernelCreationContext.GetOptionalAttribute<int>(AttrName::Axis, 0);
//         uint32_t dmlAxis = GetDmlAdjustedAxis(signedOnnxAxis, kernelCreationContext, m_inputTensorDescs.front().GetDimensionCount());

//         DML_GATHER_ELEMENTS_OPERATOR_DESC operatorDesc = {};
//         operatorDesc.InputTensor = &inputDescs[0];
//         operatorDesc.IndicesTensor = &inputDescs[1];
//         operatorDesc.OutputTensor = outputDescs.data();
//         operatorDesc.Axis = dmlAxis;

//         DML_OPERATOR_DESC opDesc = { DML_OPERATOR_GATHER_ELEMENTS, &operatorDesc };
//         SetDmlOperatorDesc(opDesc, kernelCreationContext);
//     }
// };

// class DmlOperatorGatherNd : public DmlOperator, public GatherNdHelper
// {
// public:
//     DmlOperatorGatherNd(const MLOperatorKernelCreationContext& kernelCreationContext)
//     :   DmlOperator(kernelCreationContext),
//         GatherNdHelper(kernelCreationContext, kernelCreationContext.GetTensorShapeDescription())
//     {
//         ML_CHECK_VALID_ARGUMENT(kernelCreationContext.GetInputCount() == 2, "GatherND expects 2 inputs.");
//         ML_CHECK_VALID_ARGUMENT(kernelCreationContext.GetOutputCount() == 1, "GatherND expects 1 output.");

//         auto tensorShapeDescription = kernelCreationContext.GetTensorShapeDescription();
//         std::vector<DimensionType> dataDimensions = tensorShapeDescription.GetInputTensorShape(0);
//         std::vector<DimensionType> indicesDimensions = tensorShapeDescription.GetInputTensorShape(1);
//         std::vector<DimensionType> outputDimensions = tensorShapeDescription.GetOutputTensorShape(0);

//         size_t dimensionCountMax = std::max({dataDimensions.size(), indicesDimensions.size(), outputDimensions.size()});
//         DmlOperator::Initialize(kernelCreationContext, gsl::narrow_cast<uint32_t>(dimensionCountMax));

//         std::vector<DML_TENSOR_DESC> inputDescs = GetDmlInputDescs();
//         std::vector<DML_TENSOR_DESC> outputDescs = GetDmlOutputDescs();
//         assert(inputDescs.size() == 2);
//         assert(outputDescs.size() == 1);

//         DML_GATHER_ND1_OPERATOR_DESC operatorDesc = {};
//         operatorDesc.InputTensor = &inputDescs[0];
//         operatorDesc.IndicesTensor = &inputDescs[1];
//         operatorDesc.OutputTensor = outputDescs.data();
//         operatorDesc.InputDimensionCount = static_cast<uint32_t>(dataDimensions.size());
//         operatorDesc.IndicesDimensionCount = static_cast<uint32_t>(indicesDimensions.size());
//         operatorDesc.BatchDimensionCount = m_batchCount;

//         DML_OPERATOR_DESC opDesc = { DML_OPERATOR_GATHER_ND1, &operatorDesc };
//         SetDmlOperatorDesc(opDesc, kernelCreationContext);
//     }
// };

DML_OP_DEFINE_CREATION_FUNCTION(Gather, DmlOperatorGather);
// DML_OP_DEFINE_CREATION_FUNCTION(GatherElements, DmlOperatorGatherElements);
// DML_OP_DEFINE_CREATION_FUNCTION(GatherND, DmlOperatorGatherNd);

} // namespace ODI

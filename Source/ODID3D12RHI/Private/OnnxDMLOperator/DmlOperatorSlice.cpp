// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//#include "precomp.h"
#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{

class DmlOperatorSlice //: public DmlOperator, public SliceHelper
{
public:
    DmlOperatorSlice(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    // :   DmlOperator(kernelInfo),
    //     SliceHelper(kernelInfo, kernelInfo.GetTensorShapeDescription(), opsetVersion)
    {
        const uint32_t inputCount = node.inputNames.size();
        assert((opsetVersion <  10 && inputCount == 1)
                             || (opsetVersion >= 10 && inputCount >= 3 && inputCount <= 5));

        m_input = expressionMap[node.inputNames[0]];
        CheckReference(initializerMap, node.inputNames[0]);

        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;

        std::vector<int> steps;
        std::vector<int> axes;
        std::vector<int> ends;
        std::vector<int> starts;

        //std::vector<char> temp;
        if (opsetVersion < 10){
            auto getIntsAttriAndCopy = [&](const std::string& attriName, std::vector<int>& attriVec){
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute(attriName, ONNX_PARSER::AttributeType::INTS);
                if (attriWrapper.isValid()){
                    attriVec.resize(attriWrapper.getValue().size() / 4);
                    memcpy(attriVec.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
                }
                else{
                    assert(false);
                }
            };
            getIntsAttriAndCopy("starts", starts);
            getIntsAttriAndCopy("ends", ends);
            {
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("axes", ONNX_PARSER::AttributeType::INTS);
                if (attriWrapper.isValid()){
                    axes.resize(attriWrapper.getValue().size() / 4);
                    memcpy(axes.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
                }
            }
        }
        else{
            auto getTensorAttriAndCopy = [&](const std::string& attriName, std::vector<int>& attriVec){
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute(attriName, ONNX_PARSER::AttributeType::TENSOR);
                if (attriWrapper.isValid()){
                    attriVec.resize(attriWrapper.getValue().size() / 4);
                    memcpy(attriVec.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
                }
                else{
                    assert(false);
                }
            };
            getTensorAttriAndCopy("starts", starts);
            getTensorAttriAndCopy("ends", ends);
            {
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("axes", ONNX_PARSER::AttributeType::TENSOR);
                if (attriWrapper.isValid()){
                    axes.resize(attriWrapper.getValue().size() / 4);
                    memcpy(axes.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
                }
            }
            {
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("steps", ONNX_PARSER::AttributeType::TENSOR);
                if (attriWrapper.isValid()){
                    steps.resize(attriWrapper.getValue().size() / 4);
                    memcpy(steps.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
                }
            }
            
        }
        if (axes.empty()){
            axes.resize(starts.size());
            std::iota(axes.begin(), axes.end(), 0);
        }
        if (steps.empty()){
            steps.resize(axes.size());
            std::fill(steps.begin(), steps.end(), 1);
        }

        inputWindowOffsets.resize(inputShape.size());
        inputWindowSizes.resize(inputShape.size());
        inputWindowStrides.resize(inputShape.size());

        std::fill(inputWindowOffsets.begin(), inputWindowOffsets.end(), 0);
        std::fill(inputWindowSizes.begin(), inputWindowSizes.end(), 0);
        std::fill(inputWindowStrides.begin(), inputWindowStrides.end(), 0);


        for (int i = 0; i < axes.size(); i++){
            int axis = axes[i];

            // modify start and end
            auto checkAndModifiyIfMinusOrOOB = [&](int& val) {
                if (val < 0) {
                    val = inputShape[axis] + val;
                }
                else if (val >= inputShape[axis]) {
                    val = inputShape[axis];
                }
            };
            
            checkAndModifiyIfMinusOrOOB(starts[axis]);
            checkAndModifiyIfMinusOrOOB(ends[axis]);

            inputWindowOffsets[axis] = starts[axis];
            inputWindowSizes[axis] = ends[axis] - starts[axis];
            inputWindowStrides[axis] = steps[axis];
        }
    }

    dml::Expression Create(){

        return dml::Slice(
                            m_input,
                            inputWindowOffsets,
                            inputWindowSizes,
                            inputWindowStrides);
    }
private:
    dml::Expression m_input;
    std::vector<uint32_t> inputWindowOffsets;
    std::vector<uint32_t> inputWindowSizes;
    std::vector<int32_t> inputWindowStrides;
};

DML_OP_DEFINE_CREATION_FUNCTION(Slice,  DmlOperatorSlice );
// DML_OP_DEFINE_CREATION_FUNCTION(Slice7,  VersionedKernel<DmlOperatorSlice, 7> );
// DML_OP_DEFINE_CREATION_FUNCTION(Slice10, VersionedKernel<DmlOperatorSlice, 10>);
// DML_OP_DEFINE_CREATION_FUNCTION(Slice11, VersionedKernel<DmlOperatorSlice, 11>);
// DML_OP_DEFINE_CREATION_FUNCTION(Slice13, VersionedKernel<DmlOperatorSlice, 13>);
} // namespace ODI

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// modified by fuxihao, 10/7/2022
#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{

class DmlOperatorConvolution //: public DmlOperator, public ConvolutionHelperBase
{

public:
    using Self = DmlOperatorConvolution;

    DmlOperatorConvolution(
        std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion )
    // :   DmlOperator(kernelInfo),
        // ConvolutionHelperBase(kernelInfo, kernelInfo.GetTensorShapeDescription(), direction == DML_CONVOLUTION_DIRECTION_BACKWARD, hasDynamicPads, 0, 1)
    {
        if (node.inputNames.size() < 2)
            throw std::exception("Convolution parameter number is less than 2!");

        auto& inputName = node.inputNames[0];
        auto& weightName = node.inputNames[1];
        m_input = expressionMap[inputName];
        m_weight = expressionMap[weightName];

        CheckReference(initializerMap, inputName);
        CheckReference(initializerMap, weightName);


        if (node.inputNames.size() == 2){
            // hasBias = false;
            m_bias = std::nullopt;
        }
        else{
            auto& biasName = node.inputNames[2];
            auto tempBias = expressionMap[biasName];
            CheckReference(initializerMap, biasName);

            if (tempBias.GetOutputDesc().sizes.size() == 1)
                m_bias = std::optional(dml::Reinterpret(tempBias, dml::TensorDimensions{ 1, tempBias.GetOutputDesc().sizes[0], 1, 1 }, std::nullopt));
            else
                m_bias = std::optional(tempBias);



        }
        // attribute
        //std::vector<char> tempAttri;
        std::vector<int> paddingsAttri;
        std::vector<int> stridesAttri;
        std::vector<int> dialationAttri;

        { 
        //auto_pad must be either NOTSET, SAME_UPPER, SAME_LOWER or VALID.
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("auto_pad", ONNX_PARSER::AttributeType::STRING);
            if (attriWrapper.isValid()){
                autoPad.resize(attriWrapper.getValue().size());
                memcpy(autoPad.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                autoPad = "NOTSET";
            }
        }
        
        {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("group", ONNX_PARSER::AttributeType::INT);
            if (attriWrapper.isValid()){
                memcpy(&group, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                group = 1;
            }
        }

        auto getIntsAttriAndCopy = [&](const std::string& attriName, std::vector<int>& attriVec) {
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute(attriName, ONNX_PARSER::AttributeType::INTS);
            if (attriWrapper.isValid()) {
                attriVec.resize(attriWrapper.getValue().size() / 4);
                memcpy(attriVec.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else {
                assert(false);
            }
        };

        /*{
            std::vector<int> kernelShape;

            getIntsAttriAndCopy("kernel_shape", kernelShape);
            
        }*/

        getIntsAttriAndCopy("dilations", dialationAttri);
        // getIntsAttriAndCopy("kernel_shape", kernelShape);// no necessity
        getIntsAttriAndCopy("pads", paddingsAttri);
        getIntsAttriAndCopy("strides", stridesAttri);
        
        outputPaddings.resize(paddingsAttri.size() / 2);
        startPaddings.resize(paddingsAttri.size() / 2);
        endPaddings.resize(paddingsAttri.size() / 2);

        std::fill(outputPaddings.begin(), outputPaddings.end(), 0);

        int index = 0;
        for (int i = 0; i < paddingsAttri.size() / 2; i++){
            startPaddings[i] = paddingsAttri[index++];
        }
        for (int i = 0; i < paddingsAttri.size() / 2; i++) {
            endPaddings[i] = paddingsAttri[index++];
        }
        
        dialations.resize(dialationAttri.size());
        for (int i = 0; i < dialationAttri.size(); i++) {
            dialations[i] = dialationAttri[i];
        }

        strides.resize(stridesAttri.size());
        for (int i = 0; i < stridesAttri.size(); i++) {
            strides[i] = stridesAttri[i];
        }
    }

    dml::Expression Create(){
        return dml::ConvolutionBuilder(m_input, m_weight, m_bias)
                    //.Mode()
                    .Direction(DML_CONVOLUTION_DIRECTION_FORWARD) // TODO: Add reverse direction to support transposedconv
                    .Strides(strides)
                    .Dilations(dialations)
                    .StartPadding(startPaddings)
                    .EndPadding(endPaddings)
                    //.OutputPadding(outputPaddings) // pad to the conv output
                    .GroupCount(group)
                    //.FusedActivation(dml::FusedActivation::Relu())
                    .Build();
    }
private:
    // bool hasBias = true;
    std::string autoPad;
    std::vector<uint32_t> dialations;
    // std::vector<int> kernelShape;
    std::vector<uint32_t> startPaddings;
    std::vector<uint32_t> endPaddings;
    std::vector<uint32_t> outputPaddings;
    std::vector<uint32_t> strides;
    int group;
    dml::Expression m_input;
    dml::Expression m_weight;
    std::optional<dml::Expression> m_bias;

};

// // A specific type of operation for registration.
// template <DML_CONVOLUTION_MODE Mode, DML_CONVOLUTION_DIRECTION Direction, bool hasDynamicPads = false>
// class DmlOperatorConvolutionTemplate : public DmlOperatorConvolution
// {
// public:
//     DmlOperatorConvolutionTemplate(const MLOperatorKernelCreationContext& kernelInfo)
//     :   DmlOperatorConvolution(kernelInfo, Mode, Direction, hasDynamicPads)
//     {
//     }
// };

DML_OP_DEFINE_CREATION_FUNCTION(Conv, DmlOperatorConvolution);

//DML_OP_DEFINE_CREATION_FUNCTION(Conv,                           DmlOperatorConvolutionTemplate<DML_CONVOLUTION_MODE_CROSS_CORRELATION, DML_CONVOLUTION_DIRECTION_FORWARD>);
// DML_OP_DEFINE_CREATION_FUNCTION(ConvTranspose,                  DmlOperatorConvolutionTemplate<DML_CONVOLUTION_MODE_CROSS_CORRELATION, DML_CONVOLUTION_DIRECTION_BACKWARD>);
// DML_OP_DEFINE_CREATION_FUNCTION(FusedConv,                      DmlOperatorConvolutionTemplate<DML_CONVOLUTION_MODE_CROSS_CORRELATION, DML_CONVOLUTION_DIRECTION_FORWARD>);
// DML_OP_DEFINE_CREATION_FUNCTION(FusedConvTranspose,             DmlOperatorConvolutionTemplate<DML_CONVOLUTION_MODE_CROSS_CORRELATION, DML_CONVOLUTION_DIRECTION_BACKWARD>);
// DML_OP_DEFINE_CREATION_FUNCTION(ConvTransposeWithDynamicPads,   DmlOperatorConvolutionTemplate<DML_CONVOLUTION_MODE_CROSS_CORRELATION, DML_CONVOLUTION_DIRECTION_BACKWARD, true>);

} // namespace ODI

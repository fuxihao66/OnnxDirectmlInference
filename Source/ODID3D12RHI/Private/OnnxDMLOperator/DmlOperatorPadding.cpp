// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//#include "precomp.h"
#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{

class DmlOperatorPadding// : public DmlOperator, public PaddingHelper
{
public:
    DmlOperatorPadding(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap,ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    {
        const uint32_t inputCount = node.inputNames.size();
        assert((opsetVersion >= 2 && opsetVersion < 11 && inputCount == 1)
                             || (opsetVersion >= 11 && inputCount >= 2 && inputCount <= 3));

        m_input = expressionMap[node.inputNames[0]];

        CheckReference(initializerMap, node.inputNames[0]);

        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;
        
        //std::vector<char> tempAttri;
        { 
            std::string mode;
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("mode", ONNX_PARSER::AttributeType::STRING);
            if (attriWrapper.isValid()){
                mode.resize(attriWrapper.getValue().size());
                memcpy(mode.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                mode = "constant";
            }

            if (mode == "constant"){
                paddingMode = DML_PADDING_MODE_CONSTANT;
    
            }
            else if (mode == "reflect"){
                paddingMode = DML_PADDING_MODE_REFLECTION;
            }
            else if (mode == "edge"){
                paddingMode = DML_PADDING_MODE_EDGE;
            }
            else{
                assert(false);
            }
        }
        if (opsetVersion >= 16){
            // TODO: not supported yet
            assert(false);
        }

        if (opsetVersion >= 11){
            // 

            std::vector<int64_t> paddings; // TODO: check 

            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("pads", ONNX_PARSER::AttributeType::TENSOR);
            if (attriWrapper.isValid()){
                paddings.resize(attriWrapper.getValue().size() / sizeof(int64_t));
                memcpy(paddings.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else {
                assert(false);
            }

            // TODO: CHECK INITIALIZER TYPE, might not be FLOAT 
            attriWrapper = node.GetAttribute("constant_value", ONNX_PARSER::AttributeType::TENSOR);
            if (attriWrapper.isValid()){
                memcpy(&paddingValue, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else {
                paddingValue = 0.f;
            }

            assert(paddings.size() == inputShape.size() * 2);
            // // Pad the parameters to respect DML's requirements
            startPadding.resize(inputShape.size());
            endPadding.resize(inputShape.size());
            int index = 0;
            for (int i = 0; i < inputShape.size(); i++) {
                startPadding[i] = paddings[index++];

            }
            for (int i = 0; i < inputShape.size(); i++) {
                endPadding[i] = paddings[index++];
            }
        }
        else if (opsetVersion >= 2){
            std::vector<int> paddings;


            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("value", ONNX_PARSER::AttributeType::FLOAT);
            if (attriWrapper.isValid()){
                memcpy(&paddingValue, attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                paddingValue = 0.f;
            }
            attriWrapper = node.GetAttribute("pads", ONNX_PARSER::AttributeType::INTS);
            if (attriWrapper.isValid()){
                paddings.resize(attriWrapper.getValue().size() / 4);
                memcpy(paddings.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            else{
                assert(false);
            }


            assert(paddings.size() == inputShape.size() * 2);
            // // Pad the parameters to respect DML's requirements
            startPadding.resize(inputShape.size());
            endPadding.resize(inputShape.size());
            int index = 0;
            for (int i = 0; i < inputShape.size(); i++) {
                startPadding[i] = paddings[index++];

            }
            for (int i = 0; i < inputShape.size(); i++) {
                endPadding[i] = paddings[index++];
            }
        }
        else{
            assert(false);
        }

        
   }

    dml::Expression Create(){
        return dml::Padding(
                            m_input,
                            paddingMode,
                            paddingValue,
                            startPadding,
                            endPadding);
    }
private:
    dml::Expression m_input;
    DML_PADDING_MODE paddingMode;
    float paddingValue;
    std::vector<uint32_t> startPadding;
    std::vector<uint32_t> endPadding;
};


DML_OP_DEFINE_CREATION_FUNCTION(Pad, DmlOperatorPadding);
// DML_OP_DEFINE_CREATION_FUNCTION(Pad7, VersionedKernel<DmlOperatorPadding, 7>);
// DML_OP_DEFINE_CREATION_FUNCTION(Pad11, VersionedKernel<DmlOperatorPadding, 11>);
// DML_OP_DEFINE_CREATION_FUNCTION(Pad13, VersionedKernel<DmlOperatorPadding, 13>);

} // namespace ODI

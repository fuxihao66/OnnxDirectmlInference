// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//#include "precomp.h"
#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{

class DmlOperatorCast// : public DmlOperator
{
public:
    //using Self = DmlOperatorCast;

    DmlOperatorCast(
        std::map<std::string, dml::Expression>& expressionMap,
        std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, 
        ONNX_PARSER::Op& node, 
        dml::Graph& graph,
        unsigned int opsetVersion)
    {
        if (node.inputNames.size() != 1)
            throw std::exception("Cast parameter number must be 1!");

        auto& inputName = node.inputNames[0];
        m_input = expressionMap[inputName];

        CheckReference(initializerMap, inputName);

        { // data type
            int toVal;
            ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("to", ONNX_PARSER::AttributeType::INT);
            if (attriWrapper.isValid()) {
                memcpy(&toVal, attriWrapper.getValue().data(), attriWrapper.getValue().size());

                targetDataType = static_cast<DML_TENSOR_DATA_TYPE>(ONNX_PARSER::OnnxTensorType2DmlTensorType(toVal));
            }
            else {
                assert(false);
            }
        }

        
    }

    dml::Expression Create(){
        return dml::Cast(m_input, targetDataType);
    }
private:
    dml::Expression m_input;
    DML_TENSOR_DATA_TYPE targetDataType;
};

DML_OP_DEFINE_CREATION_FUNCTION(Cast, DmlOperatorCast);
DML_OP_DEFINE_CREATION_FUNCTION(CastLike15, DmlOperatorCast);

} // namespace ODI

#pragma once
#include "../helper/pch.h"
//#include "OnnxDMLOperatorGenerator.h"

inline void CheckReference(std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, const std::string& name) {
    if (initializerMap.count(name) > 0) {
        initializerMap[name].ReferredByDml();
    }
}


// operator that has different parameter with different ir version
#define REG_INFO_VER(operatorName, version) \
    #operatorName, Create##operatorName##version,
#define REG_INFO(operatorName) \
    #operatorName, Create##operatorName,
// operaters that can be mapped to DmlOperatorCopy
#define REG_INFO_COPY(operatorName) \
    #operatorName, CreateCopy,

#define DML_OP_EXTERN_CREATION_FUNCTION(operatorName) extern dml::Expression __stdcall Create##operatorName(std::map<std::string, dml::Expression> &expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op &node, dml::Graph& graph, unsigned int opsetVersion)


#define DML_OP_DEFINE_CREATION_FUNCTION(operatorName, ...)                                                                               \
extern dml::Expression __stdcall Create##operatorName(std::map<std::string, dml::Expression> &expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op &node, dml::Graph& graph, unsigned int opsetVersion) \
{                                                                                                                                   \
        using T = __VA_ARGS__;                                                                                                           \
        T op(expressionMap, initializerMap, node, graph, opsetVersion); \
        return op.Create();\
}
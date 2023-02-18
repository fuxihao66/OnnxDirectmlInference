#pragma once

#include "ODI.h"
#include "ODIRHI.h"

class Network{
public:
    Network() = default;
    Network(const std::wstring& onnx_file_path, const std::string& model_name = "");
    ~Network();
    bool CreateModelAndUploadData(FRDGBuilder& GraphBuilder);
    bool ExecuteInference(FRDGBuilder& GraphBuilder, std::map<std::string, FRDGBufferRef> InputBuffers, FRDGBufferRef OutputBuffer);
private:
    const std::string m_model_name;
    const std::wstring m_onnx_file_path;
    ODIRHI* m_ODIRHIExtensions;
};
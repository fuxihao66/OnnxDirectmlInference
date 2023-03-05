#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "RendererInterface.h"
#include "RenderGraphUtils.h"

#include "ODID3D12RHI.h"
#include <string>

class Network{
public:
    Network() = default;
    Network(const std::wstring& onnx_file_path, const std::string& model_name = "");
    ~Network();
    bool CreateModelAndUploadData(FRDGBuilder& GraphBuilder);
    bool ExecuteInference(FRDGBuilder& GraphBuilder, std::map<std::string, FRDGBufferRef> InputBuffers, FRDGBufferRef OutputBuffer);
private:
    std::string m_model_name;
    std::wstring m_onnx_file_path;
	ODIRHI* m_ODIRHIExtensions;
};
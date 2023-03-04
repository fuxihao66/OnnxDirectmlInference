#pragma once

#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "Runtime/Launch/Resources/Version.h"

#include <map>

typedef enum ODI_Result
{
    ODI_Result_Success = 0x0,

    ODI_Result_Fail = 0x1,
} ODI_Result;

struct FODIRHICreateArguments
{
	FString PluginBaseDir;
	FDynamicRHI* DynamicRHI = nullptr;
	FString UnrealEngineVersion;
	FString UnrealProjectID;
};


struct FODIRHIInferArguments
{
	std::map<std::string, FRHIBuffer*> InputBuffers;
	FRHIBuffer* OutputBuffer = nullptr; 
	std::string ModelName;
	uint32 GPUNode = 0;
	uint32 GPUVisibility = 0;
};
class ODIRHI
{
public:
	ODIRHI(const FODIRHICreateArguments& Arguments);
	virtual ODI_Result ParseAndUploadModelData(FRHICommandList& CmdList, const std::wstring& path_to_onnx, const std::string& model_name) { return ODI_Result::ODI_Result_Fail; }
	virtual ODI_Result InitializeNewModel(FRHICommandList& CmdList, const std::string& model_name) { return ODI_Result::ODI_Result_Fail; }
	virtual ODI_Result BindResources(const std::string& model_name) { return ODI_Result::ODI_Result_Fail; }
	virtual ODI_Result ExecuteInference(FRHICommandList& CmdList, const FODIRHIInferArguments& InArguments) { return ODI_Result::ODI_Result_Fail; }

	virtual ~ODIRHI();
private:

	FDynamicRHI* DynamicRHI = nullptr;
};

class IODIRHIModule : public IModuleInterface{
public:
	virtual TUniquePtr<ODIRHI> CreateODIRHI(const FODIRHICreateArguments& Arguments) = 0;
};

class FODIRHIModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule();
	virtual void ShutdownModule();
};

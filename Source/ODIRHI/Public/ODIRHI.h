#pragma once

#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "Runtime/Launch/Resources/Version.h"
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
	FRHIBuffer* OutputBuffers = nullptr; 
};
class ODIRHI
{
public:
	ODIRHI(const FODIRHICreateArguments& Arguments);
    virtual ODI_Result ParseAndUploadModelData(FRHICommandList& CmdList, const std::wstring& path_to_onnx, const std::string& model_name);
	virtual ODI_Result InitializeNewModel(FRHICommandList& CmdList, const std::string& model_name);
	virtual ODI_Result BindResources(const std::string& model_name);
	virtual ODI_Result ExecuteInference(FRHICommandList& CmdList, const FODIRHIInferArguments& InArguments);

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

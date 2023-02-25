// Copyright Epic Games, Inc. All Rights Reserved.

#include "ODI.h"
#include "CoreMinimal.h"
#include "GeneralProjectSettings.h"

#include "Interfaces/IPluginManager.h"
//#include "Windows.h"


#define LOCTEXT_NAMESPACE "FODIModule"
DECLARE_LOG_CATEGORY_EXTERN(LogODI, Verbose, All);
DEFINE_LOG_CATEGORY(LogODI);

//DECLARE_LOG_CATEGORY_EXTERN(LogDML, Log, All);

void FODIModule::StartupModule()
{
	// FString BaseDir = IPluginManager::Get().FindPlugin("ODI")->GetBaseDir();
	// FString DmlBinariesRoot = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/DirectML_1_9_1/bin/Win64/"));
	// DirectMLLibraryHandle = FPlatformProcess::GetDllHandle(*(DmlBinariesRoot + "DirectML.dll"));

	const FString RHIName = GDynamicRHI->GetName();
	const bool bIsDX12 = (RHIName == TEXT("D3D12"));

	if (bIsDX12)
	{
		const TCHAR* ODIRHIModuleName = TEXT("ODID3D12RHI");

		const FString PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("ODI"))->GetBaseDir();

		FODIRHICreateArguments Arguments;
		Arguments.PluginBaseDir = PluginBaseDir;
		Arguments.DynamicRHI = GDynamicRHI;

		Arguments.UnrealEngineVersion = FString::Printf(TEXT("%u.%u"), FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor());
		Arguments.UnrealProjectID = GetDefault<UGeneralProjectSettings>()->ProjectID.ToString();

		IODIRHIModule* ODIRHIModule = &FModuleManager::LoadModuleChecked<IODIRHIModule>(ODIRHIModuleName);
		ODIRHIExtensions = ODIRHIModule->CreateODIRHI(Arguments);
	}
	else{
		UE_LOG(LogODI, Log, TEXT("ODI does not support %s RHI"), *RHIName);
	}

}

// TODO: make sure ODIRHI is not released before ODI is released
ODIRHI* FODIModule::GetODIRHIRef(){
	return ODIRHIExtensions.Get();
}

// void FODIModule::CreateAndInitializeModel(){
// 	ODIRHIExtensions->ParseAndUploadModelData(Cmd, DLSSArguments, DLSSState);
//     ODIRHIExtensions->InitializeNewModel();
// }

// void FODIModule::BindResourceAndExecuteInference(){
//     ODIRHIExtensions->BindResources(Arguments.ModelName);
//     LocalODIRHIExtensions->ExecuteInference(Cmd, Arguments);
// }
void FODIModule::ShutdownModule()
{

	// FPlatformProcess::FreeDllHandle(DirectMLLibraryHandle);
	// DirectMLLibraryHandle = nullptr;

	ODIRHIExtensions.Reset();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FODIModule, ODI)

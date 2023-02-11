// Copyright Epic Games, Inc. All Rights Reserved.

#include "ODI.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
//#include "Windows.h"

#pragma comment(lib, "d3d12.lib")

#define LOCTEXT_NAMESPACE "FODIModule"

DEFINE_LOG_CATEGORY(LogODI);

//DECLARE_LOG_CATEGORY_EXTERN(LogDML, Log, All);

void FODIModule::StartupModule()
{
	FString BaseDir = IPluginManager::Get().FindPlugin("ODI")->GetBaseDir();
	FString DmlBinariesRoot = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/DirectML_1_9_1/bin/Win64/"));
	DirectMLLibraryHandle = FPlatformProcess::GetDllHandle(*(DmlBinariesRoot + "DirectML.dll"));

	const FString RHIName = GDynamicRHI->GetName();
	const bool bIsDX12 = (RHIName == TEXT("D3D12"));
	ODIRHIModuleName = nullptr;

	if (bIsDX12)
	{
		const TCHAR* ODIRHIModuleName = TEXT("ODID3D12RHI");

		FNGXRHICreateArguments Arguments;
		Arguments.PluginBaseDir = PluginBaseDir;
		Arguments.DynamicRHI = GDynamicRHI;
		Arguments.NGXBinariesSearchOrder = ENGXBinariesSearchOrder(FMath::Clamp(CVarNGXBinarySearchOrder.GetValueOnAnyThread(), int32(ENGXBinariesSearchOrder::MinValue), int32(ENGXBinariesSearchOrder::MaxValue)));

		Arguments.ProjectIdentifier = ENGXProjectIdentifier(FMath::Clamp(CVarNGXProjectIdentifier.GetValueOnAnyThread(), int32(ENGXProjectIdentifier::MinValue), int32(ENGXProjectIdentifier::MaxValue)));
		Arguments.NGXAppId = NGXAppID;
		Arguments.UnrealEngineVersion = FString::Printf(TEXT("%u.%u"), FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor());
		Arguments.UnrealProjectID = GetDefault<UGeneralProjectSettings>()->ProjectID.ToString();
		Arguments.bAllowOTAUpdate = bAllowOTAUpdate;

		IODIRHIModule* ODIRHIModule = &FModuleManager::LoadModuleChecked<IODIRHIModule>(ODIRHIModuleName);
		ODIRHIExtensions = ODIRHIModule->CreateODIRHI(Arguments);
	}
	else{
		UE_LOG(LogODI, Log, TEXT("ODI does not support %s RHI"), *RHIName);
	}

}

void FODIModule::ShutdownModule()
{

	FPlatformProcess::FreeDllHandle(DirectMLLibraryHandle);
	DirectMLLibraryHandle = nullptr;

	ODIRHIExtensions.Reset();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FODIModule, ODI)

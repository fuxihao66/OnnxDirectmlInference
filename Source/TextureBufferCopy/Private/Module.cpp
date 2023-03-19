// Copyright Epic Games, Inc. All Rights Reserved.

#include "Module.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "Utils.h"

#define LOCTEXT_NAMESPACE "TextureBufferCopy"

// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
void FTextureBufferCopyModule::StartupModule()
{
	// Add shader directory
	const TSharedPtr<IPlugin> ODIPlugin = IPluginManager::Get().FindPlugin(TEXT("ODI"));
	if (ODIPlugin.IsValid())
	{
		const FString RealShaderDirectory = ODIPlugin->GetBaseDir();// +TEXT("/Shaders/");
		AddShaderSourceDirectoryMapping(TEXT("/ShadersUtility"), RealShaderDirectory);
	}
	else
	{
		/*UE_LOG(LogNeuralNetworkInferenceShaders, Warning,
			TEXT("FTextureBufferCopyModule::StartupModule(): NeuralNetworkInferencePlugin was nullptr, shaders directory not added."));*/
	}
}

// This function may be called during shutdown to clean up your module. For modules that support dynamic reloading,
// we call this function before unloading the module.
void FTextureBufferCopyModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FTextureBufferCopyModule, TextureBufferCopy);

#undef LOCTEXT_NAMESPACE

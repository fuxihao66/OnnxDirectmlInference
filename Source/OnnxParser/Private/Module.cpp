
#include "Module.h"
#include "ShaderCore.h"
#include "Utils.h"

#define LOCTEXT_NAMESPACE "OnnxParser"

// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
void FOnnxParserModule::StartupModule()
{
	
}

// This function may be called during shutdown to clean up your module. For modules that support dynamic reloading,
// we call this function before unloading the module.
void FOnnxParserModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FOnnxParserModule, OnnxParser);

#undef LOCTEXT_NAMESPACE

#include "ODIRHI.h"

#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"

DEFINE_LOG_CATEGORY_STATIC(LogODIRHI, Log, All);

#define LOCTEXT_NAMESPACE "ODIRHI"

ODIRHI::ODIRHI(const FODIRHICreateArguments& Arguments)
	: DynamicRHI(Arguments.DynamicRHI)
{
	
	UE_LOG(LogODIRHI, Log, TEXT("ODIRHI DEBUG INFO"));
}

ODIRHI::~ODIRHI()
{
}

IMPLEMENT_MODULE(FODIRHIModule, ODIRHI)

#undef LOCTEXT_NAMESPACE


void FODIRHIModule::StartupModule()
{
}

void FODIRHIModule::ShutdownModule()
{
}

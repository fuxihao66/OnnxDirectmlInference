#pragma once

#include "Modules/ModuleManager.h"
#include "ODIRHI.h"
#define MAX_DESCRIPTOR_COUNT 100000


class FODID3D12RHIModule final : public IODIRHIModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule();
	virtual void ShutdownModule();

	/** IODIRHIModule implementation */
	virtual TUniquePtr<ODIRHI> CreateODIRHI(const FODIRHICreateArguments& Arguments);
};
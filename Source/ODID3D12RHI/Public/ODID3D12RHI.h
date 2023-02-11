#pragma once

#include "Modules/ModuleManager.h"
#include "ODIRHI.h"

class FODID3D12RHIModule final : public IODIRHIModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule();
	virtual void ShutdownModule();

	/** INGXRHIModule implementation */

	virtual TUniquePtr<NGXRHI> CreateNGXRHI(const FNGXRHICreateArguments& Arguments);
};
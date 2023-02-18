// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FODIModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	ODIRHI* GetODIRHIRef();
private:
	/** Handle to the test dll we will load */
	void* DirectMLLibraryHandle;
	ODIRHI* ODIRHIExtensions;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

class ODIRHI;

class FODIModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	ODIRHI* GetODIRHIRef();
private:
	/** Handle to the test dll we will load */
	//void* DirectMLLibraryHandle;
	TUniquePtr<ODIRHI> ODIRHIExtensions;
};


#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FOnnxParserModule : public IModuleInterface
{
public:
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules
 * within this plugin.
 */
class FRoadBuilder : public IModuleInterface
{

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FRoadBuilder& Get()
	{
		return FModuleManager::LoadModuleChecked<FRoadBuilder>("RoadBuilder");
	}
	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RoadBuilder");
	}
};

class FCustomCycleCounter
{
public:
	FCustomCycleCounter(const FString& Tag);
	~FCustomCycleCounter();
	FString Tag;
	double StartTime;
};

ROADBUILDER_API DECLARE_LOG_CATEGORY_EXTERN(LogRoadBuilder, Verbose, All);
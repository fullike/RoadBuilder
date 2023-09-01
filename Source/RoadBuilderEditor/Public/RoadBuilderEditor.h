// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "EditorExtensions.h"

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class FRoadBuilderEditor : public IModuleInterface
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
	static inline FRoadBuilderEditor& Get()
	{
		return FModuleManager::LoadModuleChecked<FRoadBuilderEditor>( "RoadBuilderEditor" );
	}
	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "RoadBuilderEditor" );
	}
	TSharedPtr<FEditorExtensions> Extensions;
	TSharedPtr<IAssetTypeActions> LaneShapeTypeActions;
	TSharedPtr<IAssetTypeActions> LaneMarkStyleTypeActions;
	TSharedPtr<IAssetTypeActions> CrosswalkStyleTypeActions;
	TSharedPtr<IAssetTypeActions> PolygonMarkStyleTypeActions;
	TSharedPtr<IAssetTypeActions> RoadStyleTypeActions;
	TSharedPtr<IAssetTypeActions> RoadPropsTypeActions;
};

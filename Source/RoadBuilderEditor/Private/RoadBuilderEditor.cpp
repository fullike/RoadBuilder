// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadBuilderEditor.h"
#include "RoadEdMode.h"
#include "OSMVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Factories/LaneMarkStyleFactory.h"
#include "Factories/LaneShapeFactory.h"
#include "Factories/RoadPropsFactory.h"
#include "Factories/RoadStyleFactory.h"
#include "OSMActor.h"

IMPLEMENT_MODULE(FRoadBuilderEditor, RoadBuilderEditor)
#define LOCTEXT_NAMESPACE "RoadBuilder"
void FRoadBuilderEditor::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("RoadBuilder")), LOCTEXT("RoadBuilder", "RoadBuilder"));
	LaneShapeTypeActions = MakeShareable(new FLaneShapeTypeActions(Category));
	LaneMarkStyleTypeActions = MakeShareable(new FLaneMarkStyleTypeActions(Category));
	CrosswalkStyleTypeActions = MakeShareable(new FCrosswalkStyleTypeActions(Category));
	PolygonMarkStyleTypeActions = MakeShareable(new FPolygonMarkStyleTypeActions(Category));
	RoadStyleTypeActions = MakeShareable(new FRoadStyleTypeActions(Category));
	RoadPropsTypeActions = MakeShareable(new FRoadPropsTypeActions(Category));
	AssetTools.RegisterAssetTypeActions(LaneShapeTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(LaneMarkStyleTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(CrosswalkStyleTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(PolygonMarkStyleTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(RoadStyleTypeActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(RoadPropsTypeActions.ToSharedRef());
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	Extensions = MakeShareable(new FEditorExtensions);
	Extensions->InstallHooks();
	if (GUnrealEd)
		GUnrealEd->RegisterComponentVisualizer(UOSMComponent::StaticClass()->GetFName(), MakeShareable(new FOSMVisualizer));
	FEditorModeRegistry::Get().RegisterMode<FEdModeRoad>(FEdModeRoad::GetModeID(), NSLOCTEXT("EditorModes", "RoadMode", "Road"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.BrushEdit"), true, 7000);
}

void FRoadBuilderEditor::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FEdModeRoad::GetModeID());
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	Extensions->RemoveHooks();
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(RoadPropsTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(RoadStyleTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(PolygonMarkStyleTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(CrosswalkStyleTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(LaneMarkStyleTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(LaneShapeTypeActions.ToSharedRef());
	}
}
#undef LOCTEXT_NAMESPACE

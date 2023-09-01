// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "Factories/LaneShapeFactory.h"
#include "LaneShape.h"
#include "EditorModeManager.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "RoadBuilder"
ULaneShapeFactory::ULaneShapeFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = ULaneShape::StaticClass();
}

UObject* ULaneShapeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULaneShape* Object = NewObject<ULaneShape>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}
/*
FString ULaneMarkStyleFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("New Object"));
}
*/
FText FLaneShapeTypeActions::GetName() const
{
	return LOCTEXT("FLaneShapeTypeActionsName", "LaneShape");
}

UClass* FLaneShapeTypeActions::GetSupportedClass() const
{
	return ULaneShape::StaticClass();
}
#undef LOCTEXT_NAMESPACE
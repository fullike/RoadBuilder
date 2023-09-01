// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "Factories/LaneMarkStyleFactory.h"
#include "LaneMarkStyle.h"
#include "EditorModeManager.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "RoadBuilder"
ULaneMarkStyleFactory::ULaneMarkStyleFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = ULaneMarkStyle::StaticClass();
}

UObject* ULaneMarkStyleFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULaneMarkStyle* Object = NewObject<ULaneMarkStyle>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}

UCrosswalkStyleFactory::UCrosswalkStyleFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = UCrosswalkStyle::StaticClass();
}

UObject* UCrosswalkStyleFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UCrosswalkStyle* Object = NewObject<UCrosswalkStyle>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}

UPolygonMarkStyleFactory::UPolygonMarkStyleFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = UPolygonMarkStyle::StaticClass();
}

UObject* UPolygonMarkStyleFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPolygonMarkStyle* Object = NewObject<UPolygonMarkStyle>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}

FText FLaneMarkStyleTypeActions::GetName() const
{
	return LOCTEXT("FLaneMarkStyleTypeActionsName", "LaneMark");
}

UClass* FLaneMarkStyleTypeActions::GetSupportedClass() const
{
	return ULaneMarkStyle::StaticClass();
}

FText FCrosswalkStyleTypeActions::GetName() const
{
	return LOCTEXT("FCrosswalkStyleTypeActionsName", "Crosswalk");
}

UClass* FCrosswalkStyleTypeActions::GetSupportedClass() const
{
	return UCrosswalkStyle::StaticClass();
}

FText FPolygonMarkStyleTypeActions::GetName() const
{
	return LOCTEXT("FPolygonMarkStyleTypeActionsName", "PolygonMark");
}

UClass* FPolygonMarkStyleTypeActions::GetSupportedClass() const
{
	return UPolygonMarkStyle::StaticClass();
}
#undef LOCTEXT_NAMESPACE
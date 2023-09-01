// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "Factories/RoadStyleFactory.h"
#include "RoadActor.h"
#include "EditorModeManager.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "RoadBuilder"
URoadStyleFactory::URoadStyleFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = URoadStyle::StaticClass();
}

UObject* URoadStyleFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	URoadStyle* Object = NewObject<URoadStyle>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}

FText FRoadStyleTypeActions::GetName() const
{
	return LOCTEXT("FRoadStyleTypeActionsName", "RoadStyle");
}

UClass* FRoadStyleTypeActions::GetSupportedClass() const
{
	return URoadStyle::StaticClass();
}
#undef LOCTEXT_NAMESPACE
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "Factories/RoadPropsFactory.h"
#include "RoadProps.h"
#include "EditorModeManager.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "RoadBuilder"
URoadPropsFactory::URoadPropsFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = false;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = URoadProps::StaticClass();
}

UObject* URoadPropsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	URoadProps* Object = NewObject<URoadProps>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}

FText FRoadPropsTypeActions::GetName() const
{
	return LOCTEXT("FRoadPropsTypeActionsName", "RoadProps");
}

UClass* FRoadPropsTypeActions::GetSupportedClass() const
{
	return URoadProps::StaticClass();
}
#undef LOCTEXT_NAMESPACE
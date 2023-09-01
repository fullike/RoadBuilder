// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "SRoadEdit.h"
#include "RoadEdMode.h"
#include "SlateOptMacros.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Widgets/Layout/SWrapBox.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "RoadBuilder"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SRoadEdit::Construct(const FArguments& InArgs)
{
	RoadEditMode = FEdModeRoad::Get();
	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(6.f, 3.f);
	FMargin StandardLeftPadding(6.f, 3.f, 3.f, 3.f);
	FMargin StandardRightPadding(3.f, 3.f, 6.f, 3.f);
	FSlateFontInfo StandardFont = FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
	const FText BlankText = FText::GetEmpty();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bAllowSearch = false;
	SettingsView = PropertyEditorModule.CreateDetailView(Args);
	SettingsView->RegisterInstancedCustomPropertyLayout(UObject::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FObjectDetails::MakeInstance));
	ObjectDetailsView = PropertyEditorModule.CreateDetailView(Args);
	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = true;
	StructureViewArgs.bShowInterfaces = true;
	Args.NotifyHook = RoadEditMode;
	StructDetailsView = PropertyEditorModule.CreateStructureDetailView(Args, StructureViewArgs, nullptr);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			SettingsView.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			ObjectDetailsView.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			StructDetailsView->GetWidget().ToSharedRef()
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SRoadEdit::CreateFileToolbar(FToolBarBuilder& ToolBarBuilder)
{
}

void SRoadEdit::CreateRoadToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::RoadPlan),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::RoadPlan)
		),
		NAME_None,
		LOCTEXT("RoadEdPlan", "Plan"),
		LOCTEXT("RoadEdPlanTooltip", "Plan"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::RoadHeight),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::RoadHeight)
		),
		NAME_None,
		LOCTEXT("RoadEdHeight", "Height"),
		LOCTEXT("RoadEdHeightTooltip", "Height"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::RoadChop),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::RoadChop)
		),
		NAME_None,
		LOCTEXT("RoadEdChop", "Chop"),
		LOCTEXT("RoadEdChopTooltip", "Chop"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::RoadSplit),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::RoadSplit)
		),
		NAME_None,
		LOCTEXT("RoadEdSplit", "Split"),
		LOCTEXT("RoadEdSplitTooltip", "Split"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
}

void SRoadEdit::CreateJunctionToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::JunctionLink),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::JunctionLink)
		),
		NAME_None,
		LOCTEXT("RoadEdLink", "Link"),
		LOCTEXT("RoadEdLinkTooltip", "Link"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	/*
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::JunctionCorner),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::JunctionCorner)
		),
		NAME_None,
		LOCTEXT("RoadEdCorner", "Corner"),
		LOCTEXT("RoadEdCornerTooltip", "Corner"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetPaintBucket"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::JunctionMark),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::JunctionMark)
		),
		NAME_None,
		LOCTEXT("RoadEdMark", "Mark"),
		LOCTEXT("RoadEdMarkTooltip", "Mark"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetPaintBucket"),
		EUserInterfaceActionType::ToggleButton
	);*/
}

void SRoadEdit::CreateLaneToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::LaneEdit),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::LaneEdit)
		),
		NAME_None,
		LOCTEXT("RoadEdEdit", "Edit"),
		LOCTEXT("RoadEdEditTooltip", "Edit"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::LaneCarve),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::LaneCarve)
		),
		NAME_None,
		LOCTEXT("RoadEdCarve", "Carve"),
		LOCTEXT("RoadEdCarveTooltip", "Carve"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::LaneWidth),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::LaneWidth)
		),
		NAME_None,
		LOCTEXT("RoadEdWidth", "Width"),
		LOCTEXT("RoadEdWidthTooltip", "Width"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
}

void SRoadEdit::CreateMarkingToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::MarkingLane),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::MarkingLane)
		),
		NAME_None,
		LOCTEXT("RoadEdLane", "Lane"),
		LOCTEXT("RoadEdLaneTooltip", "Lane"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::MarkingPoint),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::MarkingPoint)
		),
		NAME_None,
		LOCTEXT("RoadEdPoint", "Point"),
		LOCTEXT("RoadEdPointTooltip", "Point"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::MarkingCurve),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::MarkingCurve)
		),
		NAME_None,
		LOCTEXT("RoadEdCurve", "Curve"),
		LOCTEXT("RoadEdCurveTooltip", "Curve"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
}

void SRoadEdit::CreateGroundToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SRoadEdit::SetToolIndex, (int)FEdModeRoad::GroundEdit),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SRoadEdit::IsToolIndex, (int)FEdModeRoad::GroundEdit)
		),
		NAME_None,
		LOCTEXT("RoadEdEdit", "Edit"),
		LOCTEXT("RoadEdEditTooltip", "Edit"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FoliageEditMode.SetSelect"),
		EUserInterfaceActionType::ToggleButton
	);
}

void SRoadEdit::CreateSettingsToolbar(FToolBarBuilder& ToolBarBuilder)
{
}

bool SRoadEdit::SRoadEdit::IsToolIndex(int Index)
{
	return RoadEditMode->GetToolIndex() == Index;
}

void SRoadEdit::SetToolIndex(int Index)
{
	RoadEditMode->SetToolIndex(Index);
	if (Index == FEdModeRoad::File)
		SettingsView->SetObject(GetMutableDefault<USettings_File>());
	else if (Index == FEdModeRoad::RoadPlan)
		SettingsView->SetObject(GetMutableDefault<USettings_RoadPlan>());
	else if (Index == FEdModeRoad::RoadSplit)
		SettingsView->SetObject(GetMutableDefault<USettings_RoadSplit>());
	else if (Index == FEdModeRoad::Settings)
		SettingsView->SetObject(GetMutableDefault<USettings_Global>());
	else
		SettingsView->SetObject(nullptr);
}

void SRoadEdit::SetEditNone()
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	ObjectDetailsView->OnFinishedChangingProperties().Clear();
	StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetObject(nullptr);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
	ObjectDetailsView->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditRoadPoint(ARoadActor* Road, int Index)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Road && Index != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FRoadPoint::StaticStruct(), (uint8*)&Road->RoadPoints[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnRoadPointPropertyChanged, Road, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditHeightPoint(ARoadActor* Road, int Index)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Road && Index != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FHeightPoint::StaticStruct(), (uint8*)&Road->HeightPoints[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnRoadPointPropertyChanged, Road, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditLink(AJunctionActor* Junction, int Gate, int Link)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Junction && Gate != INDEX_NONE && Link != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FJunctionLink::StaticStruct(), (uint8*)&Junction->Gates[Gate].Links[Link])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnLinkPropertyChanged, Junction, Gate, Link);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditLaneSegment(URoadLane* Lane, int Index)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Lane && Index != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FLaneSegment::StaticStruct(), (uint8*)&Lane->Segments[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnLaneSegmentPropertyChanged, Lane, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditBoundaryOffset(URoadBoundary* Boundary, int Index)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Boundary && Index != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FCurveOffset::StaticStruct(), (uint8*)&Boundary->LocalOffsets[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnBoundaryOffsetPropertyChanged, Boundary, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditBoundarySegment(URoadBoundary* Boundary, int Index)
{
	StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
	if (Boundary && Index != INDEX_NONE)
	{
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FBoundarySegment::StaticStruct(), (uint8*)&Boundary->Segments[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnBoundarySegmentPropertyChanged, Boundary, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Visible);
}

void SRoadEdit::SetEditMarkingPoint(UMarkingPoint* Marking)
{
	ObjectDetailsView->OnFinishedChangingProperties().Clear();
	ObjectDetailsView->SetObject(Marking);
	ObjectDetailsView->OnFinishedChangingProperties().AddSP(this, &SRoadEdit::OnMarkingPointPropertyChanged, Marking);	
	ObjectDetailsView->SetVisibility(EVisibility::Visible);
	StructDetailsView->GetWidget()->SetVisibility(EVisibility::Collapsed);
}

void SRoadEdit::SetEditMarkingCurve(UMarkingCurve* Marking, int Index)
{
	if (Index == INDEX_NONE)
	{
		ObjectDetailsView->OnFinishedChangingProperties().Clear();
		ObjectDetailsView->SetObject(Marking);
		ObjectDetailsView->OnFinishedChangingProperties().AddSP(this, &SRoadEdit::OnMarkingCurvePropertyChanged, Marking);
	}
	else
	{
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FMarkingCurvePoint::StaticStruct(), (uint8*)&Marking->Points[Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnMarkingCurvePointPropertyChanged, Marking, Index);
	}
	ObjectDetailsView->SetVisibility(Index == INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(Index == INDEX_NONE ? EVisibility::Collapsed : EVisibility::Visible);
}

void SRoadEdit::SetEditGround(AGroundActor* Ground, int Index)
{
	if (Index != INDEX_NONE)
	{
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().Clear();
		if (Ground->Points[Index].Road)
			StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FGroundPoint::StaticStruct(), (uint8*)&Ground->Points[Index])));
		else
			StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(FindObject<UStruct>(nullptr, TEXT("/Script/CoreUObject.Vector")), (uint8*)&Ground->ManualPoints[Ground->Points[Index].Index])));
		StructDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &SRoadEdit::OnGroundPointPropertyChanged, Ground, Index);
	}
	else
		StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(nullptr, 0)));
	ObjectDetailsView->SetVisibility(Index == INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed);
	StructDetailsView->GetWidget()->SetVisibility(Index == INDEX_NONE ? EVisibility::Collapsed : EVisibility::Visible);
}

void SRoadEdit::OnRoadPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, ARoadActor* Road, int Index)
{
	Road->UpdateCurve();
	Road->GetScene()->Rebuild();
}

void SRoadEdit::OnLinkPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, AJunctionActor* Junction, int Gate, int Link)
{
	Junction->GetScene()->Rebuild();
}

void SRoadEdit::OnLaneSegmentPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadLane* Lane, int Index)
{
	ARoadActor* Road = Lane->GetRoad();
	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FLaneSegment, Dist))
		ClampDist(Lane->Segments, Index, Road->Length());
	else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FLaneSegment, LaneType))
	{
		if (Lane->Segments[Index].LaneType == ELaneType::Collapsed)
		{
			URoadBoundary* Boundary = Lane->GetSide() ? Lane->LeftBoundary : Lane->RightBoundary;
			Boundary->SetZeroOffset(Lane->SegmentStart(Index), Lane->SegmentEnd(Index));
		}
	}
	Road->UpdateLanes();
	Road->GetScene()->Rebuild();
}

void SRoadEdit::OnBoundaryOffsetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadBoundary* Boundary, int Index)
{
	ARoadActor* Road = Boundary->GetRoad();
	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FCurveOffset, Dist))
		ClampDist(Boundary->LocalOffsets, Index, Road->Length());
	Road->UpdateLanes();
	Road->GetScene()->Rebuild();
}

void SRoadEdit::OnBoundarySegmentPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadBoundary* Boundary, int Index)
{
	ARoadActor* Road = Boundary->GetRoad();
	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FBoundarySegment, Dist))
		ClampDist(Boundary->Segments, Index, Road->Length());
	Road->UpdateLanes();
	Road->GetScene()->Rebuild();
}

void SRoadEdit::OnMarkingPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingPoint* Marking)
{
	Marking->GetRoad()->GetScene()->Rebuild();
}

void SRoadEdit::OnMarkingCurvePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingCurve* Marking)
{
	Marking->GetRoad()->GetScene()->Rebuild();
}

void SRoadEdit::OnMarkingCurvePointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingCurve* Marking, int Index)
{
	Marking->GetRoad()->GetScene()->Rebuild();
}

void SRoadEdit::OnGroundPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, AGroundActor* Ground, int Index)
{
	Ground->GetScene()->Rebuild();
}

TSharedRef<IDetailCustomization> FObjectDetails::MakeInstance()
{
	return MakeShareable(new FObjectDetails);
}

void FObjectDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	AddCallInEditorMethods(DetailBuilder);
}

void FObjectDetails::AddCallInEditorMethods(IDetailLayoutBuilder& DetailBuilder)
{
	// metadata tag for defining sort order of function buttons within a Category
	static const FName NAME_DisplayPriority("DisplayPriority");

	// Get all of the functions we need to display (done ahead of time so we can sort them)
	TArray<UFunction*, TInlineAllocator<8>> CallInEditorFunctions;
	for (TFieldIterator<UFunction> FunctionIter(DetailBuilder.GetBaseClass(), EFieldIteratorFlags::IncludeSuper); FunctionIter; ++FunctionIter)
	{
		UFunction* TestFunction = *FunctionIter;

		if (TestFunction->GetBoolMetaData(FBlueprintMetadata::MD_CallInEditor) && (TestFunction->ParmsSize == 0))
		{
			if (UClass* TestFunctionOwnerClass = TestFunction->GetOwnerClass())
			{
				if (UBlueprint* Blueprint = Cast<UBlueprint>(TestFunctionOwnerClass->ClassGeneratedBy))
				{
					if (FBlueprintEditorUtils::IsEditorUtilityBlueprint(Blueprint))
					{
						// Skip Blutilities as these are handled by FEditorUtilityInstanceDetails
						continue;
					}
				}
			}

			const FName FunctionName = TestFunction->GetFName();
			if (!CallInEditorFunctions.FindByPredicate([&FunctionName](const UFunction* Func) { return Func->GetFName() == FunctionName; }))
			{
				CallInEditorFunctions.Add(*FunctionIter);
			}
		}
	}

	if (CallInEditorFunctions.Num() > 0)
	{
		// Copy off the objects being customized so we can invoke a function on them later, removing any that are a CDO
		DetailBuilder.GetObjectsBeingCustomized(/*out*/ SelectedObjectsList);
		// SelectedObjectsList.RemoveAllSwap([](TWeakObjectPtr<UObject> ObjPtr) { UObject* Obj = ObjPtr.Get(); return (Obj == nullptr) || Obj->HasAnyFlags(RF_ArchetypeObject); });
		if (SelectedObjectsList.Num() == 0)
		{
			return;
		}

		// Sort the functions by category and then by DisplayPriority meta tag, and then by name
		CallInEditorFunctions.Sort([](UFunction& A, UFunction& B)
			{
				const int32 CategorySort = A.GetMetaData(FBlueprintMetadata::MD_FunctionCategory).Compare(B.GetMetaData(FBlueprintMetadata::MD_FunctionCategory));
				if (CategorySort != 0)
				{
					return (CategorySort <= 0);
				}
				else
				{
					FString DisplayPriorityAStr = A.GetMetaData(NAME_DisplayPriority);
					int32 DisplayPriorityA = (DisplayPriorityAStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityAStr));
					if (DisplayPriorityA == 0 && !FCString::IsNumeric(*DisplayPriorityAStr))
					{
						DisplayPriorityA = MAX_int32;
					}

					FString DisplayPriorityBStr = B.GetMetaData(NAME_DisplayPriority);
					int32 DisplayPriorityB = (DisplayPriorityBStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityBStr));
					if (DisplayPriorityB == 0 && !FCString::IsNumeric(*DisplayPriorityBStr))
					{
						DisplayPriorityB = MAX_int32;
					}

					return (DisplayPriorityA == DisplayPriorityB) ? (A.GetName() <= B.GetName()) : (DisplayPriorityA <= DisplayPriorityB);
				}
			});

		struct FCategoryEntry
		{
			FName CategoryName;
			FName RowTag;
			TSharedPtr<SWrapBox> WrapBox;
			FTextBuilder FunctionSearchText;

			FCategoryEntry(FName InCategoryName)
				: CategoryName(InCategoryName)
			{
				WrapBox = SNew(SWrapBox)
					// Setting the preferred size here (despite using UseAllottedSize) is a workaround for an issue
					// when contained in a scroll box: prior to the first tick, the wrap box will use preferred size
					// instead of allotted, and if preferred size is set small, it will cause the box to wrap a lot and
					// request too much space from the scroll box. On next tick, SWrapBox is updated but the scroll box
					// does not realize that it needs to show more elements, until it is scrolled.
					// Setting a large value here means that the SWrapBox will request too little space prior to tick,
					// which will cause the scroll box to virtualize more elements at the start, but this is less broken.
					.PreferredSize(2000)
					.UseAllottedSize(true);
			}
		};

		// Build up a set of functions for each category, accumulating search text and buttons in a wrap box
		FName ActiveCategory;
		TArray<FCategoryEntry, TInlineAllocator<8>> CategoryList;
		for (UFunction* Function : CallInEditorFunctions)
		{
			FName FunctionCategoryName(NAME_Default);
			if (Function->HasMetaData(FBlueprintMetadata::FBlueprintMetadata::MD_FunctionCategory))
			{
				FunctionCategoryName = FName(*Function->GetMetaData(FBlueprintMetadata::MD_FunctionCategory));
			}

			if (FunctionCategoryName != ActiveCategory)
			{
				ActiveCategory = FunctionCategoryName;
				CategoryList.Emplace(FunctionCategoryName);
			}
			FCategoryEntry& CategoryEntry = CategoryList.Last();

			//@TODO: Expose the code in UK2Node_CallFunction::GetUserFacingFunctionName / etc...
			const FText ButtonCaption = FText::FromString(FName::NameToDisplayString(*Function->GetName(), false));
			FText FunctionTooltip = Function->GetToolTipText();
			if (FunctionTooltip.IsEmpty())
			{
				FunctionTooltip = FText::FromString(Function->GetName());
			}


			TWeakObjectPtr<UFunction> WeakFunctionPtr(Function);
			CategoryEntry.WrapBox->AddSlot()
				.Padding(0.0f, 0.0f, 5.0f, 3.0f)
				[
					SNew(SButton)
					.Text(ButtonCaption)
				.OnClicked(FOnClicked::CreateSP(this, &FObjectDetails::OnExecuteCallInEditorFunction, WeakFunctionPtr))
				.ToolTipText(FunctionTooltip.IsEmptyOrWhitespace() ? LOCTEXT("CallInEditorTooltip", "Call an event on the selected object(s)") : FunctionTooltip)
				];

			CategoryEntry.RowTag = Function->GetFName();
			CategoryEntry.FunctionSearchText.AppendLine(ButtonCaption);
			CategoryEntry.FunctionSearchText.AppendLine(FunctionTooltip);
		}

		// Now edit the categories, adding the button strips to the details panel
		for (FCategoryEntry& CategoryEntry : CategoryList)
		{
			IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(CategoryEntry.CategoryName);
			CategoryBuilder.AddCustomRow(CategoryEntry.FunctionSearchText.ToText())
				.RowTag(CategoryEntry.RowTag)
				[
					CategoryEntry.WrapBox.ToSharedRef()
				];
		}
	}
}

FReply FObjectDetails::OnExecuteCallInEditorFunction(TWeakObjectPtr<UFunction> WeakFunctionPtr)
{
	if (UFunction* Function = WeakFunctionPtr.Get())
	{
		//@TODO: Consider naming the transaction scope after the fully qualified function name for better UX
		FScopedTransaction Transaction(LOCTEXT("ExecuteCallInEditorMethod", "Call In Editor Action"));

		FEditorScriptExecutionGuard ScriptGuard;
		for (TWeakObjectPtr<UObject> SelectedObjectPtr : SelectedObjectsList)
		{
			if (UObject* Object = SelectedObjectPtr.Get())
			{
				Object->ProcessEvent(Function, nullptr);
			}
		}
	}

	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE
/*
void UToolData_LaneEdit::Duplicate()
{
	FEdModeRoad::Get()->InputKeyDown(FKeyEvent(EKeys::D, FModifierKeysState(), false, 0, 0, 0));
}

void UToolData_LaneEdit::Delete()
{
	FEdModeRoad::Get()->InputKeyDown(FKeyEvent(EKeys::Delete, FModifierKeysState(), false, 0, 0, 0));
}*/
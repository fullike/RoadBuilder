// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "Widgets/SCompoundWidget.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "IDetailCustomization.h"
#include "RoadScene.h"
#include "Settings.h"

class FEdModeRoad;
class SRoadEdit : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRoadEdit) {}
	SLATE_END_ARGS()

public:
	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs);
	void CreateFileToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateRoadToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateJunctionToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateLaneToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateMarkingToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateGroundToolbar(FToolBarBuilder& ToolBarBuilder);
	void CreateSettingsToolbar(FToolBarBuilder& ToolBarBuilder);
	bool IsToolIndex(int Index);
	void SetToolIndex(int Index);
	void SetEditNone();
	void SetEditRoadPoint(ARoadActor* Road, int Index);
	void SetEditHeightPoint(ARoadActor* Road, int Index);
	void SetEditLink(AJunctionActor* Junction, int Gate, int Link);
	void SetEditLaneSegment(URoadLane* Lane, int Index);
	void SetEditBoundaryOffset(URoadBoundary* Boundary, int Index);
	void SetEditBoundarySegment(URoadBoundary* Boundary, int Index);
	void SetEditMarkingPoint(UMarkingPoint* Marking);
	void SetEditMarkingCurve(UMarkingCurve* Marking, int Index);
	void SetEditGround(AGroundActor* Ground, int Index);
	void OnRoadPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, ARoadActor* Road, int Index);
	void OnLinkPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, AJunctionActor* Junction, int Gate, int Link);
	void OnLaneSegmentPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadLane* Lane, int Index);
	void OnBoundaryOffsetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadBoundary* Boundary, int Index);
	void OnBoundarySegmentPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, URoadBoundary* Boundary, int Index);
	void OnMarkingPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingPoint* Marking);
	void OnMarkingCurvePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingCurve* Marking);
	void OnMarkingCurvePointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, UMarkingCurve* Marking, int Index);
	void OnGroundPointPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, AGroundActor* Ground, int Index);
private:
	FEdModeRoad* RoadEditMode;
	TSharedPtr<IDetailsView> SettingsView;
	TSharedPtr<IDetailsView> ObjectDetailsView;
	TSharedPtr<IStructureDetailsView> StructDetailsView;
};

class FObjectDetails : public IDetailCustomization
{
public:
	// Creates an instance of FObjectDetails
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:
	// Creates a button strip in each category that contains reflected functions marked as CallInEditor
	void AddCallInEditorMethods(IDetailLayoutBuilder& DetailBuilder);

	// Executes the specified method on the selected objects
	FReply OnExecuteCallInEditorFunction(TWeakObjectPtr<UFunction> WeakFunctionPtr);

private:
	// The list of selected objects, used when invoking a CallInEditor method
	TArray<TWeakObjectPtr<UObject>> SelectedObjectsList;
};

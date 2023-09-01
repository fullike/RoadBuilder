// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadEdModeToolkit.h"
#include "RoadEdMode.h"

FName PaletteName_File = TEXT("File");
FName PaletteName_Road = TEXT("Road");
FName PaletteName_Junction = TEXT("Junction");
FName PaletteName_Lane = TEXT("Lane");
FName PaletteName_Marking = TEXT("Marking");
FName PaletteName_Ground = TEXT("Ground");
FName PaletteName_Settings = TEXT("Settings");

#define LOCTEXT_NAMESPACE "RoadBuilder"

class FRoadBuilderEditorCommands : public TCommands<FRoadBuilderEditorCommands>
{
public:
	FRoadBuilderEditorCommands() : TCommands<FRoadBuilderEditorCommands>(TEXT("RoadBuilderEditor"), LOCTEXT("RoadBuilderEditor", "RoadBuilderEditor"), NAME_None, FAppStyle::GetAppStyleSetName()) {}
	virtual void RegisterCommands() override
	{
		UI_COMMAND(SelectParent, "SelectParent", "SelectParent", EUserInterfaceActionType::Button, FInputChord(EKeys::Escape));
	}
public:
	TSharedPtr<FUICommandInfo> SelectParent;
};

void FRoadEdModeToolkit::Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
	RoadEdWidget = SNew(SRoadEdit);
	FRoadBuilderEditorCommands::Register();
	ToolkitCommands->MapAction(FRoadBuilderEditorCommands::Get().SelectParent, FExecuteAction::CreateSP(this, &FRoadEdModeToolkit::SelectParent));
	FModeToolkit::Init(InitToolkitHost);
}

class FEdMode* FRoadEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FEdModeRoad::GetModeID());
}

TSharedPtr<SWidget> FRoadEdModeToolkit::GetInlineContent() const
{
	return RoadEdWidget;
}

void FRoadEdModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const
{
	InPaletteName.Add(PaletteName_File);
	InPaletteName.Add(PaletteName_Road);
	InPaletteName.Add(PaletteName_Junction);
	InPaletteName.Add(PaletteName_Lane);
	InPaletteName.Add(PaletteName_Marking);
	InPaletteName.Add(PaletteName_Ground);
	InPaletteName.Add(PaletteName_Settings);
}

FText FRoadEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	return FText::FromString(PaletteName.ToString());
}

void FRoadEdModeToolkit::BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolBarBuilder)
{
	if (PaletteName == PaletteName_File)
	{
		RoadEdWidget->CreateFileToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Road)
	{
		RoadEdWidget->CreateRoadToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Junction)
	{
		RoadEdWidget->CreateJunctionToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Lane)
	{
		RoadEdWidget->CreateLaneToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Marking)
	{
		RoadEdWidget->CreateMarkingToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Ground)
	{
		RoadEdWidget->CreateGroundToolbar(ToolBarBuilder);
	}
	else if (PaletteName == PaletteName_Settings)
	{
		RoadEdWidget->CreateSettingsToolbar(ToolBarBuilder);
	}
}

void FRoadEdModeToolkit::OnToolPaletteChanged(FName PaletteName)
{
	if (PaletteName == PaletteName_File)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::File);
	}
	else if (PaletteName == PaletteName_Road)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::RoadPlan);
	}
	else if (PaletteName == PaletteName_Junction)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::JunctionLink);
	}
	else if (PaletteName == PaletteName_Lane)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::LaneEdit);
	}
	else if (PaletteName == PaletteName_Marking)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::MarkingLane);
	}
	else if (PaletteName == PaletteName_Ground)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::GroundEdit);
	}
	else if (PaletteName == PaletteName_Settings)
	{
		RoadEdWidget->SetToolIndex(FEdModeRoad::Settings);
	}
}

void FRoadEdModeToolkit::SelectParent()
{
	((FRoadTool*)FEdModeRoad::Get()->GetCurrentTool())->SelectParent();
}
#undef LOCTEXT_NAMESPACE
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "Settings.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "RoadBuilderEditor/Public/RoadEdMode.h"
void USettings_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
		SaveConfig();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

USettings_Global::USettings_Global(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultDrivingShape = LoadObject<ULaneShape>(nullptr, TEXT("/RoadBuilder/LaneShapes/Driving.Driving"));
	DefaultSidewalkShape = LoadObject<ULaneShape>(nullptr, TEXT("/RoadBuilder/LaneShapes/Sidewalk.Sidewalk"));
	DefaultMedianShape = LoadObject<ULaneShape>(nullptr, TEXT("/RoadBuilder/LaneShapes/Median.Median"));
	DefaultDashStyle = LoadObject<ULaneMarkStyle>(nullptr, TEXT("/RoadBuilder/MarkStyles/LaneMark/WhiteDash.WhiteDash"));
	DefaultSolidStyle = LoadObject<ULaneMarkStyle>(nullptr, TEXT("/RoadBuilder/MarkStyles/LaneMark/WhiteSolid.WhiteSolid"));
	DefaultGroundMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("MaterialInstanceConstant'/Game/CityPark/Materials/Ground/MI_Ground02.MI_Ground02'"));
	DefaultGoreMarking = LoadObject<UPolygonMarkStyle>(nullptr, TEXT("/RoadBuilder/MarkStyles/PolygonMark/ChevronRegion.ChevronRegion'"));
	BuildJunctions = 1;
	BuildProps = 1;
	DisplayGateRadianPoints = 0;
}

USettings_OSM::USettings_OSM(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ConnectRoads = 1;
}
#if WITH_EDITOR
void USettings_File::Xodr()
{
	FEdModeRoad::Get()->Scene->ExportXodr();
}

void USettings_RoadPlan::Apply()
{
	if (ARoadActor* Road = FEdModeRoad::Get()->SelectedRoad)
	{
		const FScopedTransaction Transaction(FText::FromName(TEXT("RoadPlan")));
		USettings_RoadPlan* Data = GetMutableDefault<USettings_RoadPlan>();
		Road->Modify();
		Road->ClearLanes();
		Road->InitWithStyle(Data->Style.LoadSynchronous());
		Road->UpdateCurve();
		Road->GetScene()->Rebuild();
	}
}

void USettings_RoadPlan::Create()
{
	if (ARoadActor* Road = FEdModeRoad::Get()->SelectedRoad)
		Road->CreateStyle();
}
#endif
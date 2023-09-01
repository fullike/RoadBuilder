// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadBoundary.h"
#include "RoadCurve.h"
#include "LaneMarkStyle.h"
#include "LaneShape.h"
#include "RoadLane.generated.h"

UENUM()
enum class ELaneType : uint8
{
	None,
	Driving,
	Shoulder,
	Sidewalk,
	Median,
	Collapsed,
};

USTRUCT()
struct FLaneSegment
{
	GENERATED_USTRUCT_BODY()
	ULaneShape* GetLaneShape();
	FString GetXodrType()
	{
		if (LaneType == ELaneType::Driving) return TEXT("driving");
		if (LaneType == ELaneType::Shoulder) return TEXT("shoulder");
		if (LaneType == ELaneType::Sidewalk) return TEXT("sidewalk");
		return TEXT("none");
	}
	void ExportXodr(FXmlNode* XmlNode, double Start, double End);

	UPROPERTY(EditAnywhere, Category = Segment)
	double Dist = 0;

	UPROPERTY(EditAnywhere, Category = Segment)
	ULaneShape* LaneShape = nullptr;

	UPROPERTY(EditAnywhere, Category = Segment)
	ELaneType LaneType = (ELaneType)0;
};

UCLASS()
class ROADBUILDER_API URoadLane : public URoadCurve
{
	GENERATED_BODY()
public:
	int AddSegment(double Dist);
	int GetSegment(double Dist) { return GetElementIndex(Segments, Dist); }
	int GetSegment(ELaneType Type);
	int GetSide();
	void DeleteSegment(int Index);
	void BuildMesh(FRoadActorBuilder& Builder, int LaneId, double Start, double End, int Index);
	void BuildMesh(FRoadActorBuilder& Builder, int LaneId, double Start, double End, double Length);
	void ExportXodr(FXmlNode* XmlNode, double Start, double End);
	void Update(int Side);
	void SnapSegment(int Index);
	bool SnapSegment(double& Dist, double Tolerance = 100.0);
	void Join(URoadLane* Lane);
	void Cut(double R_Start, double R_End);
	double& SegmentStart(int i) { return Segments[i].Dist; }
	double& SegmentEnd(int i) { return i + 1 < Segments.Num() ? Segments[i + 1].Dist : Length(); }
	double GetWidth(double Dist) { return FMath::Abs(LeftBoundary->GetOffset(Dist) - RightBoundary->GetOffset(Dist)); }
	URoadBoundary*& GetBoundary(int Side) { return Side ? LeftBoundary : RightBoundary; }

	UPROPERTY(EditAnywhere, Category = Lane)
	TArray<FLaneSegment> Segments;

	UPROPERTY()
	URoadBoundary* LeftBoundary = nullptr;

	UPROPERTY()
	URoadBoundary* RightBoundary = nullptr;
};

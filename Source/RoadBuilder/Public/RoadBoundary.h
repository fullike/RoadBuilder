// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadCurve.h"
#include "RoadProps.h"
#include "LaneMarkStyle.h"
#include "RoadBoundary.generated.h"

USTRUCT()
struct FBoundarySegment
{
	GENERATED_USTRUCT_BODY()
	void ExportXodr(FXmlNode* XmlNode, double S);

	UPROPERTY(EditAnywhere, Category = Segment)
	double Dist = 0;

	UPROPERTY(EditAnywhere, Category = Segment)
	ULaneMarkStyle* LaneMarking = nullptr;

	UPROPERTY(EditAnywhere, Category = Segment)
	URoadProps* Props = nullptr;
};

UCLASS()
class ROADBUILDER_API URoadBoundary : public URoadCurve
{
	GENERATED_BODY()
public:
	int AddSegment(double Dist);
	int GetSegment(double Dist) { return GetElementIndex(Segments, Dist); }
	int GetSegment(ELaneMarkType Type);
	int GetSide();
	void DeleteSegment(int Index);
	void BuildMesh(FRoadActorBuilder& Builder, double Start, double End, int Index);
	void ExportXodr(FXmlNode* XmlNode, double Start, double End);
	void ExportWidth(FXmlNode* XmlNode, double Start, double End);
	void SnapSegment(int Index);
	void SnapOffset(int Index);
	bool SnapSegment(double& Dist, double Tolerance = 100.0);
	bool SnapOffset(double& Dist, double Tolerance = 100.0);
	bool IsZeroOffset(int Index);
	void SetZeroOffset(double Start, double End);
	void Join(URoadBoundary* Boundary);
	void Cut(double R_Start, double R_End);
	double& SegmentStart(int i) { return Segments[i].Dist; }
	double& SegmentEnd(int i) { return i + 1 < Segments.Num() ? Segments[i + 1].Dist : Length(); }
	URoadLane*& GetLane(int Side) { return Side ? LeftLane : RightLane; }

	UPROPERTY(EditAnywhere, Category = Boundary)
	TArray<FBoundarySegment> Segments;

	UPROPERTY()
	URoadLane* LeftLane = nullptr;

	UPROPERTY()
	URoadLane* RightLane = nullptr;

	TArray<FOctreeElementId2> OctreeIds;
};

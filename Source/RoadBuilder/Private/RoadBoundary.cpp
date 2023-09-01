// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadBoundary.h"
#include "RoadActor.h"

void FBoundarySegment::ExportXodr(FXmlNode* XmlNode, double S)
{
	FXmlNode* MarkNode = XmlNode_CreateChild(XmlNode, TEXT("roadMark"));
	XmlNode_AddAttribute(MarkNode, TEXT("sOffset"), S * 0.01);
	if (LaneMarking)
	{
		XmlNode_AddAttribute(MarkNode, TEXT("type"), LaneMarking->GetXodrType());
		XmlNode_AddAttribute(MarkNode, TEXT("color"), LaneMarking->GetXodrColor());
	}
	else
	{
		XmlNode_AddAttribute(MarkNode, TEXT("type"), TEXT("none"));
		XmlNode_AddAttribute(MarkNode, TEXT("color"), TEXT("standard"));
	}
}

int URoadBoundary::AddSegment(double Dist)
{
	int Index = GetSegment(Dist);
	FBoundarySegment NewSeg = { Dist, Segments[Index].LaneMarking };
	Segments.Insert(NewSeg, Index + 1);
	return Index + 1;
}

int URoadBoundary::GetSegment(ELaneMarkType Type)
{
	for (int i = 0; i < Segments.Num(); i++)
		if (Segments[i].LaneMarking && Segments[i].LaneMarking->MarkType == Type)
			return i;
	return INDEX_NONE;
}

int URoadBoundary::GetSide()
{
	ARoadActor* Road = GetRoad();
	URoadBoundary* Left = this;
	while (Left != Road->BaseCurve)
	{
		if (!Left->LeftLane)
			return 1;
		Left = Left->LeftLane->LeftBoundary;
	}
	return 0;
}

void URoadBoundary::DeleteSegment(int Index)
{
	if (Segments.Num() > 1)
	{
		Segments.RemoveAt(Index);
		Segments[0].Dist = 0;
	}
}

void URoadBoundary::BuildMesh(FRoadActorBuilder& Builder, double Start, double End, int Index)
{
	SCOPE_CYCLE_COUNTER(STAT_BuildBoundary);
	ARoadActor* Road = GetRoad();
	FPolyline Polyline = (!Road->IsLink() && GetSide() ? CreatePolyline(End, Start) : CreatePolyline(Start, End)).Redist();
	if (Segments[Index].LaneMarking)
	{
		Segments[Index].LaneMarking->BuildMesh(Road, Builder.MeshBuilder, Polyline);
	}
	if (Segments[Index].Props)
	{
		Segments[Index].Props->Generate(Road, Polyline, Builder);
	}
}

void URoadBoundary::ExportXodr(FXmlNode* XmlNode, double Start, double End)
{
	for (int i = 0; i < Segments.Num(); i++)
	{
		double Min = FMath::Max(Start, SegmentStart(i));
		double Max = FMath::Min(End, SegmentEnd(i));
		if (Min < Max)
			Segments[i].ExportXodr(XmlNode, Min - Start);
	}
}

void URoadBoundary::ExportWidth(FXmlNode* XmlNode, double Start, double End)
{
	TArray<FCurveOffset> CutOffsets = CutLocalOffsets(Start, End);
	for (int i = 0; i < CutOffsets.Num(); i++)
	{
		FCurveOffset& This = CutOffsets[i];
		FXmlNode* WidthNode = XmlNode_CreateChild(XmlNode, TEXT("width"));
		XmlNode_AddAttribute(WidthNode, TEXT("sOffset"), This.Dist * 0.01);
		FVector4 ABCD;
		if (i + 1 < CutOffsets.Num())
		{
			FCurveOffset& Next = CutOffsets[i + 1];
			double Length = (Next.Dist - This.Dist) * 0.01;
			ABCD = CalcABCD(This.Offset * 0.01, This.Dir * Length, Next.Offset * 0.01, Next.Dir * Length, Length);
		}
		else
			ABCD = FVector4(This.Offset * 0.01, 0, 0, 0);
		XmlNode_AddABCD(WidthNode, ABCD);
	}
}

void URoadBoundary::SnapSegment(int Index)
{
	ClampDist(Segments, Index, Offsets.Last().Dist);
	if (SnapOffset(Segments[Index].Dist))
		return;
	if (LeftLane)
	{
		if (LeftLane->SnapSegment(Segments[Index].Dist))
			return;
		if (LeftLane->LeftBoundary->SnapSegment(Segments[Index].Dist))
			return;
		if (LeftLane->LeftBoundary->SnapOffset(Segments[Index].Dist))
			return;
	}
	if (RightLane)
	{
		if (RightLane->SnapSegment(Segments[Index].Dist))
			return;
		if (RightLane->RightBoundary->SnapSegment(Segments[Index].Dist))
			return;
		if (RightLane->RightBoundary->SnapOffset(Segments[Index].Dist))
			return;
	}
}

void URoadBoundary::SnapOffset(int Index)
{
	ClampDist(LocalOffsets, Index, Offsets.Last().Dist);
	if (SnapSegment(LocalOffsets[Index].Dist))
		return;
	if (LeftLane)
	{
		if (LeftLane->SnapSegment(LocalOffsets[Index].Dist))
			return;
		if (LeftLane->LeftBoundary->SnapSegment(LocalOffsets[Index].Dist))
			return;
		if (LeftLane->LeftBoundary->SnapOffset(LocalOffsets[Index].Dist))
			return;
	}
	if (RightLane)
	{
		if (RightLane->SnapSegment(LocalOffsets[Index].Dist))
			return;
		if (RightLane->RightBoundary->SnapSegment(LocalOffsets[Index].Dist))
			return;
		if (RightLane->RightBoundary->SnapOffset(LocalOffsets[Index].Dist))
			return;
	}
}

bool URoadBoundary::SnapSegment(double& Dist, double Tolerance)
{
	for (int i = 0; i < Segments.Num(); i++)
	{
		if (FMath::IsNearlyEqual(Dist, Segments[i].Dist, Tolerance))
		{
			Dist = Segments[i].Dist;
			return true;
		}
	}
	return false;
}

bool URoadBoundary::SnapOffset(double& Dist, double Tolerance)
{
	int Index = GetLocalOffset(Dist, Tolerance);
	if (Index != INDEX_NONE)
	{
		Dist = LocalOffsets[Index].Dist;
		return true;
	}
	return false;
}

bool URoadBoundary::IsZeroOffset(int Index)
{
	int StartIndex = GetPointIndex(LocalOffsets, SegmentStart(Index));
	int EndIndex = FMath::Min(GetPointIndex(LocalOffsets, SegmentEnd(Index)) + 1, LocalOffsets.Num() - 1);
	for (int i = StartIndex; i <= EndIndex; i++)
		if (!FMath::IsNearlyZero(LocalOffsets[i].Offset))
			return false;
	return true;
}

void URoadBoundary::SetZeroOffset(double Start, double End)
{
	int StartIndex = GetLocalOffset(Start, 100);
	if (StartIndex == INDEX_NONE)
		StartIndex = AddLocalOffset(Start);
	int EndIndex = GetLocalOffset(End, 100);
	if (EndIndex == INDEX_NONE)
		EndIndex = AddLocalOffset(End);
	for (int i = StartIndex; i <= EndIndex; i++)
		LocalOffsets[i].Offset = 0;
}

void URoadBoundary::Join(URoadBoundary* Boundary)
{
	URoadCurve::Join(Boundary);
	for (int i = 1; i < Boundary->Segments.Num(); i++)
	{
		FBoundarySegment& Segment = Boundary->Segments[i];
		Segments.Add({ Segment.Dist + Length(), Segment.LaneMarking, Segment.Props });
	}
}

void URoadBoundary::Cut(double R_Start, double R_End)
{
	URoadCurve::Cut(R_Start, R_End);
	for (int i = 0; i < Segments.Num();)
	{
		double Start = SegmentStart(i);
		double End = SegmentEnd(i);
		if (End < R_Start || Start > R_End)
			Segments.RemoveAt(i);
		else
			i++;
	}
	for (int i = 0; i < Segments.Num(); i++)
		Segments[i].Dist -= R_Start;

}

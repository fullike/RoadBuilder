// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadLane.h"
#include "Settings.h"

ULaneShape* FLaneSegment::GetLaneShape()
{
	USettings_Global* Settings = GetMutableDefault<USettings_Global>();
	if (!LaneShape)
	{
		switch (LaneType)
		{
		case ELaneType::Sidewalk:
			LaneShape = Settings->DefaultSidewalkShape.LoadSynchronous();
			break;
		case ELaneType::Median:
			LaneShape = Settings->DefaultMedianShape.LoadSynchronous();
			break;
		default:
			LaneShape = Settings->DefaultDrivingShape.LoadSynchronous();
		}
	}
	return LaneShape;
}

void FLaneSegment::ExportXodr(FXmlNode* XmlNode, double Start, double End)
{
	XmlNode_AddAttribute(XmlNode, TEXT("type"), GetXodrType());
	/*
	TArray<FVector2D> WidthList;
	WidthList.Add({ Start, CalcWidth(Start) });
	for (FLaneWidth& Width : Widths)
		if (Width.Dist > Start && Width.Dist < End)
			WidthList.Add({ Width.Dist,Width.Width });
	WidthList.Add({ End,CalcWidth(End) });
	for (int i = 1; i < WidthList.Num();)
	{
		if (FMath::IsNearlyEqual(WidthList[i].Y, WidthList[i - 1].Y, 1.0))
			WidthList.RemoveAt(i);
		else
			i++;
	}
	for (int i = 0; i < WidthList.Num(); i++)
	{
		FVector2D& This = WidthList[i];
		FXmlNode* WidthNode = XmlNode_CreateChild(XmlNode, TEXT("width"));
		XmlNode_AddAttribute(WidthNode, TEXT("sOffset"), (This.X - Start) * 0.01);
		FVector4 ABCD;
		if (i + 1 < WidthList.Num())
		{
			FVector2D& Next = WidthList[i + 1];
			ABCD = CalcABCD(This.Y * 0.01, 0, Next.Y * 0.01, 0, (Next.X - This.X) * 0.01);
		}
		else
			ABCD = FVector4(WidthList[i].Y * 0.01, 0, 0, 0);
		XmlNode_AddABCD(WidthNode, ABCD);
	}*/
}

int URoadLane::AddSegment(double Dist)
{
	int Index = GetSegment(Dist);
	FLaneSegment NewSeg = { Dist, nullptr, Segments[Index].LaneType };
	Segments.Insert(NewSeg, Index + 1);
	return Index + 1;
}

int URoadLane::GetSegment(ELaneType Type)
{
	for (int i = 0; i < Segments.Num(); i++)
		if (Segments[i].LaneType == Type)
			return i;
	return INDEX_NONE;
}

int URoadLane::GetSide()
{
	ARoadActor* Road = GetRoad();
	URoadBoundary* Left = LeftBoundary;
	while (Left != Road->BaseCurve)
	{
		if (!Left->LeftLane)
			return 1;
		Left = Left->LeftLane->LeftBoundary;
	}
	return 0;
}

void URoadLane::DeleteSegment(int Index)
{
	Segments.RemoveAt(Index);
	if (!Segments.Num())
		GetRoad()->DeleteLane(this);
	else
		Segments[0].Dist = 0;
}

void URoadLane::BuildMesh(FRoadActorBuilder& Builder, int LaneId, double Start, double End, int Index)
{
	SCOPE_CYCLE_COUNTER(STAT_BuildLane);
	ULaneShape* LaneShape = Segments[Index].GetLaneShape();
	FPolyline LeftCurve = LaneId < 0 ? LeftBoundary->CreatePolyline(Start, End) : RightBoundary->CreatePolyline(End, Start);
	FPolyline RightCurve = LaneId < 0 ? RightBoundary->CreatePolyline(Start, End) : LeftBoundary->CreatePolyline(End, Start);
	if (LaneShape)
	{
		int Side = GetSide();
		double C = (Start + End) / 2;
		URoadLane* LeftLane = Side ? RightBoundary->RightLane : LeftBoundary->LeftLane;
		URoadLane* RightLane = Side ? LeftBoundary->LeftLane : RightBoundary->RightLane;
		bool SkipLeft = LeftLane && (Segments[Index].LaneShape == LeftLane->Segments[LeftLane->GetSegment(C)].LaneShape);
		bool SkipRight = RightLane && (Segments[Index].LaneShape == RightLane->Segments[RightLane->GetSegment(C)].LaneShape);
		LaneShape->BuildMesh(Builder.MeshBuilder, LeftCurve, RightCurve, SkipLeft, SkipRight);
	}
}

void URoadLane::BuildMesh(FRoadActorBuilder& Builder, int LaneId, double Start, double End, double Length)
{
	bool IsLink = GetRoad()->IsLink();
	for (int i = 0; i < Segments.Num(); i++)
	{
		if (IsLink && Segments[i].LaneType != ELaneType::Sidewalk)
			continue;
		double S = FMath::Max(Start, SegmentStart(i));
		double E = FMath::Min(End, SegmentEnd(i));
		if (S < E)
			BuildMesh(Builder, LaneId, S, E, i);
	}
}

void URoadLane::ExportXodr(FXmlNode* XmlNode, double Start, double End)
{
	for (int i = 0; i < Segments.Num(); i++)
	{
		double Min = FMath::Max(Start, SegmentStart(i));
		double Max = FMath::Min(End, SegmentEnd(i));
		if (Min < Max)
		{
			Segments[i].ExportXodr(XmlNode, Min, Max);
			break;
		}
	}
}

void URoadLane::Update(int Side)
{
	URoadBoundary* SrcBoundary = GetBoundary(!Side);
	URoadBoundary* DstBoundary = GetBoundary(Side);
	UpdateOffsets(SrcBoundary, DstBoundary, Side);
	DstBoundary->UpdateOffsets(SrcBoundary, DstBoundary, Side);
}

void URoadLane::SnapSegment(int Index)
{
	ClampDist(Segments, Index, Offsets.Last().Dist);
	if (LeftBoundary->SnapSegment(Segments[Index].Dist))
		return;
	if (LeftBoundary->SnapOffset(Segments[Index].Dist))
		return;
	if (RightBoundary->SnapSegment(Segments[Index].Dist))
		return;
	if (RightBoundary->SnapOffset(Segments[Index].Dist))
		return;
	if (LeftBoundary->LeftLane)
	{
		if (LeftBoundary->LeftLane->SnapSegment(Segments[Index].Dist))
			return;
	}
	if (RightBoundary->RightLane)
	{
		if (RightBoundary->RightLane->SnapSegment(Segments[Index].Dist))
			return;
	}
}

bool URoadLane::SnapSegment(double& Dist, double Tolerance)
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

void URoadLane::Join(URoadLane* Lane)
{
	for (int i = 1; i < Lane->Segments.Num(); i++)
	{
		FLaneSegment& Segment = Lane->Segments[i];
		Segments.Add({ Segment.Dist + Length(), Segment.LaneShape, Segment.LaneType });
	}
}

void URoadLane::Cut(double R_Start, double R_End)
{
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

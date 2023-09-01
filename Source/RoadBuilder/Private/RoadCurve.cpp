// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadCurve.h"
#include "RoadActor.h"

bool DoLinesIntersect(const FVector2D& Segment1Start, const FVector2D& Segment1Dir, const FVector2D& Segment2Start, const FVector2D& Segment2Dir, double& Seg1Intersection, double& Seg2Intersection)
{
    const double Determinant = FVector2D::CrossProduct(Segment1Dir, Segment2Dir);
    if (!FMath::IsNearlyZero(Determinant))
    {
        const FVector2D SegmentStartDelta = Segment2Start - Segment1Start;
        const double OneOverDet = 1.0 / Determinant;
        Seg1Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment2Dir) * OneOverDet;
        Seg2Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment1Dir) * OneOverDet;
        return (Seg1Intersection > -SMALL_NUMBER && Seg2Intersection > -SMALL_NUMBER);
    }
	else
	{
		FVector2D StartDir = Segment1Start - Segment2Start;
		if (FMath::IsNearlyZero(StartDir | FVector2D(-Segment1Dir.Y, Segment1Dir.X)))
		{
			Seg1Intersection = StartDir | Segment1Dir;
			Seg2Intersection = (-StartDir) | Segment2Dir;
			return (Seg1Intersection > -SMALL_NUMBER && Seg2Intersection > -SMALL_NUMBER);
		}
		if (FMath::IsNearlyZero(StartDir | FVector2D(-Segment2Dir.Y, Segment2Dir.X)))
		{
			Seg1Intersection = StartDir | Segment1Dir;
			Seg2Intersection = (-StartDir) | Segment2Dir;
			return (Seg1Intersection > -SMALL_NUMBER && Seg2Intersection > -SMALL_NUMBER);
		}
	}
    return false;
}

bool DoLineSegmentsIntersect(const FVector2D& Segment1Start, const FVector2D& Segment1End, const FVector2D& Segment2Start, const FVector2D& Segment2End, double& Seg1Intersection, double& Seg2Intersection)
{
	const FVector2D Segment1Dir = Segment1End - Segment1Start;
	const FVector2D Segment2Dir = Segment2End - Segment2Start;
	const double Determinant = FVector2D::CrossProduct(Segment1Dir, Segment2Dir);
	if (!FMath::IsNearlyZero(Determinant))
	{
		const FVector2D SegmentStartDelta = Segment2Start - Segment1Start;
		const double OneOverDet = 1.0 / Determinant;
		Seg1Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment2Dir) * OneOverDet;
		Seg2Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment1Dir) * OneOverDet;
		return (Seg1Intersection > -SMALL_NUMBER && Seg1Intersection < 1.0 + SMALL_NUMBER && Seg2Intersection > -SMALL_NUMBER && Seg2Intersection < 1.0 + SMALL_NUMBER);
	}
	else
	{
		FVector2D Cross;
		if (FMath::IsNearlyZero(DistanceToLine(Segment1Start, Segment1End, Segment2Start, Cross)))
		{
			Seg1Intersection = FVector2D::Distance(Segment1Start, Cross) / Segment1Dir.Size();
			Seg2Intersection = 0;
			return true;
		}
		if (FMath::IsNearlyZero(DistanceToLine(Segment2Start, Segment2End, Segment1Start, Cross)))
		{
			Seg1Intersection = 0;
			Seg2Intersection = FVector2D::Distance(Segment2Start, Cross) / Segment2Dir.Size();
			return true;
		}
	}
	return false;
}

double DistanceToLine(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& Point, FVector2D& Cross)
{
	FVector2D LineDirection = LineEnd - LineStart;
    double Length = LineDirection.Size();
	LineDirection /= Length;
	FVector2D PointToStart = Point - LineStart;
    double Dot = PointToStart | LineDirection;
	if (Dot <= 0)
	{
		Cross = LineStart;
		return FVector2D::Distance(Point, LineStart);
	}
	if (Dot >= Length)
	{
		Cross = LineEnd;
		return FVector2D::Distance(Point, LineEnd);
	}
	FVector2D Normal(-LineDirection.Y, LineDirection.X);
    double Dist = Normal | PointToStart;
	Cross = Point - Normal * Dist;
	return Dist;
}

FVector2D CalcUV(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& Point)
{
	FVector2D LineDirection = (LineEnd - LineStart).GetSafeNormal();
	FVector2D Normal(-LineDirection.Y, LineDirection.X);
	FVector2D PointToStart = Point - LineStart;
	return FVector2D(PointToStart | LineDirection, PointToStart | Normal);
}

ARoadActor* URoadCurve::GetRoad()
{
	return Cast<ARoadActor>(GetOuter());
}

FPolyline URoadCurve::CreatePolyline(double Offset)
{
	return CreatePolyline(Offsets[0].Dist, Offsets.Last().Dist, Offset);
}

FPolyline URoadCurve::CreatePolyline(double Start, double End, double Offset, double Height)
{
	FPolyline Polyline;
	if (Start < End)
	{
		for (int i = 0; i < Offsets.Num() - 1; i++)
		{
			double S_Start = FMath::Max(Start, Offsets[i].Dist);
			double S_End = FMath::Min(End, Offsets[i + 1].Dist);
			if (S_Start <= S_End)
				FillPolyline(Polyline, S_Start, S_End, Offset, Height);
		}
	}
	else if (Start > End)
	{
		for (int i = Offsets.Num() - 1; i > 0; i--)
		{
			double S_Start = FMath::Min(Start, Offsets[i].Dist);
			double S_End = FMath::Max(End, Offsets[i - 1].Dist);
			if (S_Start >= S_End)
				FillPolyline(Polyline, S_Start, S_End, Offset, Height);
		}
	}
	return MoveTemp(Polyline);
}

void URoadCurve::FillPolyline(FPolyline& Polyline, double Start, double End, double Offset, double Height)
{
	ARoadActor* Road = GetRoad();
	double Length = End - Start;
	double OffsetDiff = GetOffset(End) - GetOffset(Start);
	int NumSegments = FMath::Max(1, FMath::RoundToInt((FMath::Abs(Length) + FMath::Abs(OffsetDiff) * 16) / Road->Smoothness));
	for (int i = 0; i <= NumSegments; i++)
	{
		int NumSegs = 1;
		if (i < NumSegments)
		{
			double R = Road->GetRadian(FMath::Lerp(Start, End, double(i) / NumSegments));
			double NextR = Road->GetRadian(FMath::Lerp(Start, End, double(i + 1) / NumSegments));
			double Diff = WrapRadian(NextR - R);
			NumSegs = FMath::Max(NumSegs, FMath::RoundToInt(51200 * FMath::Abs(Diff) / DOUBLE_PI / Road->Smoothness));
		}
		for (int j = 0; j < NumSegs; j++)
		{
			double A = (i + double(j) / NumSegs) / NumSegments;
			double S = FMath::Lerp(Start, End, A);
			double R = Road->GetRadian(S);
			double O = GetOffset(S) + Offset;
			double H = Road->GetHeight(S) + Height;
			check(!FMath::IsNaN(H));
			FVector Right(-FMath::Sin(R), FMath::Cos(R), 0);
			FVector Pos = FVector(Road->GetPos(S), H) + Right * O;
			Polyline.AddPoint(Pos, End > Start ? R : R + DOUBLE_PI, S);
		}
	}
}

FVector2D URoadCurve::GetPos2D(double Dist)
{
	ARoadActor* Road = GetRoad();
	double O = GetOffset(Dist);
	double R = Road->GetRadian(Dist);
	return Road->GetPos(Dist) + FVector2D(-FMath::Sin(R), FMath::Cos(R)) * O;
}

FVector2D URoadCurve::GetDir2D(double Dist)
{
	double R = GetRoad()->GetRadian(Dist);
	FCurveOffset& Offset = Offsets[GetPointIndex(Offsets, Dist)];
	R += Offset.GetR(Dist - Offset.Dist);
	return FVector2D(FMath::Cos(R), FMath::Sin(R));
}

FVector URoadCurve::GetPos(double Dist)
{
	ARoadActor* Road = GetRoad();
	double O = GetOffset(Dist);
	double H = Road->GetHeight(Dist);
	FVector Right = Road->GetRight(Dist);
	return FVector(Road->GetPos(Dist), H) + Right * O;
}

FVector URoadCurve::GetPos(const FVector2D& UV)
{
	ARoadActor* Road = GetRoad();
	double O = GetOffset(UV.X) + UV.Y;
	double H = Road->GetHeight(UV.X);
	FVector Right = Road->GetRight(UV.X);
	return FVector(Road->GetPos(UV.X), H) + Right * O;
}

FVector URoadCurve::GetDir(double Dist)
{
	double R = GetRoad()->GetRadian(Dist);
	FCurveOffset& Offset = Offsets[GetPointIndex(Offsets, Dist)];
	R += Offset.GetR(Dist - Offset.Dist);
	return FVector(FMath::Cos(R), FMath::Sin(R), 0);
}

int URoadCurve::AddLocalOffset(double Dist)
{
	int Index = GetPointIndex(LocalOffsets, Dist);
	FCurveOffset NewOffset = { Dist, LocalOffsets[Index].Offset };
	LocalOffsets.Insert(NewOffset, Index + 1);
	return Index + 1;
}

int URoadCurve::GetLocalOffset(double Dist, double Tolerance)
{
	for (int i = 0; i < LocalOffsets.Num(); i++)
		if (FMath::IsNearlyEqual(Dist, LocalOffsets[i].Dist, Tolerance))
			return i;
	return INDEX_NONE;
}

double URoadCurve::GetOffset(double Dist)
{
	FCurveOffset& Offset = Offsets[GetPointIndex(Offsets, Dist)];
	return Offset.Get(Dist - Offset.Dist);
}

double URoadCurve::GetLocalOffset(double Dist)
{
	FCurveOffset& Offset = LocalOffsets[GetPointIndex(LocalOffsets, Dist)];
	return Offset.Get(Dist - Offset.Dist);
}

void URoadCurve::UpdateOffsets(URoadCurve* SrcCurve, URoadCurve* DstCurve, int Side)
{
	if (SrcCurve)
	{
		double Sign = Side ? -1 : 1;
		double Scale = DstCurve == this ? Sign : 0.5 * Sign;
		Offsets.Reset();
		for (FCurveOffset& Offset : SrcCurve->Offsets)
			Offsets.Add({ Offset.Dist, Offset.Offset + DstCurve->GetLocalOffset(Offset.Dist) * Scale });
		for (FCurveOffset& Offset : DstCurve->LocalOffsets)
		{
			int Index = GetPointIndex(Offsets, Offset.Dist);
			Offsets.Insert({ Offset.Dist, SrcCurve->GetOffset(Offset.Dist) + Offset.Offset * Scale }, Index + 1);
		}
		CleanOffsets();
	}
	else
		Offsets = LocalOffsets;
	Curve = CreatePolyline();
}

void URoadCurve::DeleteOffset(int Index)
{
	if (LocalOffsets.Num() > 2)
	{
		LocalOffsets.RemoveAt(Index);
		LocalOffsets[0].Dist = 0;
	}
}

void URoadCurve::Join(URoadCurve* RoadCurve)
{
	LocalOffsets.Pop();
	for (int i = 1; i < RoadCurve->LocalOffsets.Num(); i++)
	{
		FCurveOffset& Offset = RoadCurve->LocalOffsets[i];
		LocalOffsets.Add({ Offset.Dist + Length(), Offset.Offset });
	}
}

void URoadCurve::Cut(double R_Start, double R_End)
{
	if (GetLocalOffset(R_Start, 0) == INDEX_NONE)
		AddLocalOffset(R_Start);
	if (GetLocalOffset(R_End, 0) == INDEX_NONE)
		AddLocalOffset(R_End);
	for (int i = 0; i < LocalOffsets.Num();)
	{
		if (LocalOffsets[i].Dist < R_Start || LocalOffsets[i].Dist > R_End)
			LocalOffsets.RemoveAt(i);
		else
			i++;
	}
	for (int i = 0; i < LocalOffsets.Num(); i++)
		LocalOffsets[i].Dist -= R_Start;
}

TArray<FCurveOffset> URoadCurve::CutOffsets(const TArray<FCurveOffset>& Offsets, double R_Start, double R_End)
{
	TArray<FCurveOffset> Results;
	for (int i = 0; i < Offsets.Num() - 1; i++)
	{
		const FCurveOffset& ThisOffset = Offsets[i];
		const FCurveOffset& NextOffset = Offsets[i + 1];
		double S_Start = FMath::Max(R_Start, ThisOffset.Dist);
		double S_End = FMath::Min(R_End, NextOffset.Dist);
		if (S_Start < S_End)
		{
			double Start = S_Start - ThisOffset.Dist;
			double End = S_End - ThisOffset.Dist;
			double Length = NextOffset.Dist - ThisOffset.Dist;
			check(Length > 0);
			FCurveOffset& NewOffset = Results[Results.AddDefaulted()];
			NewOffset.Dist = S_Start - R_Start;
			NewOffset.Offset = FMath::CubicInterp(ThisOffset.Offset, ThisOffset.Dir * Length, NextOffset.Offset, NextOffset.Dir * Length, Start / Length);
			NewOffset.Dir = FMath::CubicInterpDerivative(ThisOffset.Offset, ThisOffset.Dir * Length, NextOffset.Offset, NextOffset.Dir * Length, Start / Length) / Length;
			if (S_End <= NextOffset.Dist)
			{
				FCurveOffset& LastOffset = Results[Results.AddDefaulted()];
				LastOffset.Dist = S_End - R_Start;
				LastOffset.Offset = FMath::CubicInterp(ThisOffset.Offset, ThisOffset.Dir * Length, NextOffset.Offset, NextOffset.Dir * Length, End / Length);
				LastOffset.Dir = FMath::CubicInterpDerivative(ThisOffset.Offset, ThisOffset.Dir * Length, NextOffset.Offset, NextOffset.Dir * Length, End / Length) / Length;
			}
		}
	}
	return MoveTemp(Results);
}

void URoadCurve::CleanOffsets(TArray<FCurveOffset>& Offsets)
{
	Offsets.Sort();
	for (int i = 1; i < Offsets.Num() - 1;)
	{
		FCurveOffset& Prev = Offsets[i - 1];
		FCurveOffset& Next = Offsets[i + 1];
		FCurveOffset& Offset = Offsets[i];
		if (FMath::IsNearlyEqual(Prev.Dist, Offset.Dist, 100) || FMath::IsNearlyEqual(Next.Dist, Offset.Dist, 100))
		{
			Offsets.RemoveAt(i);
			continue;
		}
		if (FMath::IsNearlyEqual(Prev.Offset, Offset.Offset, 1) && FMath::IsNearlyEqual(Next.Offset, Offset.Offset, 1))
		{
			Offsets.RemoveAt(i);
			continue;
		}
		i++;
	}
}

#if WITH_EDITOR
bool URoadCurve::Modify(bool bAlwaysMarkDirty)
{
	return UObject::Modify(bAlwaysMarkDirty);
}
void URoadCurve::PostEditUndo()
{
	UObject::PostEditUndo();
	ARoadActor* Road = GetRoad();
	if (IsValid(Road))
		Road->UpdateCurve();
}
#endif
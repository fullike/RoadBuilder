// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Math/GenericOctree.h"
#include "XmlFile.h"
#include "Spiral.h"
#include "RoadCurve.generated.h"

ROADBUILDER_API bool DoLinesIntersect(const FVector2D& Segment1Start, const FVector2D& Segment1Dir, const FVector2D& Segment2Start, const FVector2D& Segment2Dir, double& Seg1Intersection, double& Seg2Intersection);
ROADBUILDER_API bool DoLineSegmentsIntersect(const FVector2D& Segment1Start, const FVector2D& Segment1End, const FVector2D& Segment2Start, const FVector2D& Segment2End, double& Seg1Intersection, double& Seg2Intersection);
ROADBUILDER_API double DistanceToLine(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& Point, FVector2D& Cross);
ROADBUILDER_API FVector2D CalcUV(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& Point);
DECLARE_STATS_GROUP(TEXT("RoadBuilder"), STATGROUP_RoadBuilder, STATCAT_Advanced);

inline FVector4 CalcABCD(double P0, double T0, double P1, double T1, double D)
{
	double D1 = 1.0 / D;
	double D2 = D1 * D1;
	double D3 = D1 * D2;
	return FVector4(P0, T0 * D1, (3 * P1 - 3 * P0 - 2 * T0 - T1) * D2, (2 * P0 - 2 * P1 + T0 + T1) * D3);
}
inline FBox CalcOBB(TArray<FVector>& Points)
{
	FTransform Trans((Points[1] - Points[0]).ToOrientationQuat(), Points[0]);
	FBox Box(EForceInit::ForceInit);
	for (FVector& Point : Points)
		Box += Trans.InverseTransformPosition(Point);
	return Box;
}
inline double LerpRadian(const double StartRadian, const double EndRadian, const double Alpha)
{
	double Diff = EndRadian - StartRadian;
	if (Diff > DOUBLE_PI)
		return FMath::Lerp(StartRadian + DOUBLE_TWO_PI, EndRadian, Alpha);
	if (Diff < -DOUBLE_PI)
		return FMath::Lerp(StartRadian, EndRadian + DOUBLE_TWO_PI, Alpha);
	return FMath::Lerp(StartRadian, EndRadian, Alpha);
}
inline bool IsUVValid(const FVector2D& UV)
{
	return UV.Y < MAX_FLT;
}
template<typename StructType>
int GetElementIndex(const TArray<StructType>& Array, double Dist)
{
	int32 Length = Array.Num();
	int32 Middle = Length;
	int32 Offset = 0;
	while (Middle > 0)
	{
		Middle = Length / 2;
		if (Array[Offset + Middle].Dist <= Dist)
			Offset += Middle;
		Length -= Middle;
	}
	return Offset;
}
template<typename StructType>
int GetPointIndex(const TArray<StructType>& Array, double Dist)
{
	int Index = GetElementIndex(Array, Dist);
	if (Index == Array.Num() - 1)
		Index--;
	return Index;
}
template<typename StructType>
void ClampDist(TArray<StructType>& Array, int Index, double Len)
{
	double Min = Index > 0 ? Array[Index - 1].Dist : 0;
	double Max = Index == 0 ? 0 : (Index + 1 < Array.Num() ? Array[Index + 1].Dist : Len);
	Array[Index].Dist = FMath::Clamp(Array[Index].Dist, Min, Max);
}
inline double WrapRadian(double R)
{
	check(!FMath::IsNaN(R));
	while (R <= -DOUBLE_PI)
		R += 2 * DOUBLE_PI;
	while (R > DOUBLE_PI)
		R -= 2 * DOUBLE_PI;
	return R;
}
USTRUCT()
struct FPolyPoint
{
	GENERATED_USTRUCT_BODY()
	FPolyPoint() {}
	FPolyPoint(const FVector& P, double R = 0, double D = 0) :Pos(P), Radian(R), Dist(D) {}
	static FPolyPoint Lerp(const FPolyPoint& Start, const FPolyPoint& End, double Dist)
	{
		double Alpha = (Dist - Start.Dist) / (End.Dist - Start.Dist);
		return FPolyPoint(FMath::Lerp(Start.Pos, End.Pos, Alpha), FMath::Lerp(Start.Radian, End.Radian, Alpha), FMath::Lerp(Start.Dist, End.Dist, Alpha));
	}
	FVector2D Pos2D() const
	{
		return FVector2D(Pos);
	}

	UPROPERTY(EditAnywhere, Category = Point)
	FVector Pos = FVector::ZeroVector;
	//Radian is needed for connectivity after offset
	UPROPERTY(EditAnywhere, Category = Point)
	double Radian = 0;

	UPROPERTY(EditAnywhere, Category = Point)
	double Dist = 0;
};

USTRUCT()
struct FPolyline
{
	GENERATED_USTRUCT_BODY()
	FPolyline() {}
	FPolyline(TArray<FPolyPoint>& Pts)
	{
		Points = MoveTemp(Pts);
	}
	FPolyline(TArray<FPolyPoint>&& Pts)
	{
		Points = Pts;
	}
	int32 GetPoint(double Dist) const
	{
		check(Points[0].Dist < Points.Last().Dist);
		return GetPointIndex(Points, Dist);
	}
	void AddPoint(double Dist)
	{
		int Index = GetPoint(Dist);
		FPolyPoint& Start = Points[Index];
		FPolyPoint& End = Points[Index + 1];
		if (!FMath::IsNearlyEqual(Dist, Start.Dist) && !FMath::IsNearlyEqual(Dist, End.Dist))
			Points.Insert(FPolyPoint::Lerp(Start, End, Dist), Index+1);
	}
	void AddPoint(const FVector& Pos, double Radian)
	{
		double Dist;
		if (Points.Num())
		{
			Dist = Points.Last().Dist;
			double Diff = FVector::Dist(Points.Last().Pos, Pos);
			if (Diff < UE_DOUBLE_SMALL_NUMBER)
				return;
			Dist += Diff;
		}
		else
			Dist = 0;
		Points.Add(FPolyPoint(Pos, Radian, Dist));
	}
	void AddPoint(const FVector& Pos, double Radian, double Dist)
	{
		if (Points.Num() && Points.Last().Pos.Equals(Pos))
			return;
		Points.Add(FPolyPoint(Pos, Radian, Dist));
	}
	void Append(const FPolyline& Other)
	{
		for (int i = 0; i < Other.Points.Num(); i++)
		{
			const FPolyPoint& Point = Other.Points[i];
			AddPoint(Point.Pos, Point.Radian);
		}
	}
	FVector GetDir(int i) const
	{
		double Radian = Points[i].Radian;
		return FVector(FMath::Cos(Radian), FMath::Sin(Radian), 0);
	}
	FVector GetRight(int i) const
	{
		double Radian = Points[i].Radian;
		return FVector(-FMath::Sin(Radian), FMath::Cos(Radian), 0);
	}
	double GetStraightRadian(int i) const
	{
		FVector Dir = GetStraightDir(i);
		return FMath::Atan2(Dir.Y, Dir.X);
	}
	FVector GetStraightDir(int i) const
	{
		FVector Prev, Next;
		if (i > 0)
			Prev = (Points[i].Pos - Points[i - 1].Pos).GetSafeNormal();
		if (i < Points.Num() - 1)
			Next = (Points[i + 1].Pos - Points[i].Pos).GetSafeNormal();
		if (i == 0)
			Prev = Next;
		if (i == Points.Num() - 1)
			Next = Prev;
		return (Prev + Next).GetSafeNormal();
	}
	FVector GetStraightRight(int i) const
	{
		FVector Prev, Next;
		if (i > 0)
			Prev = (Points[i].Pos - Points[i - 1].Pos).GetSafeNormal();
		if (i < Points.Num() - 1)
			Next = (Points[i + 1].Pos - Points[i].Pos).GetSafeNormal();
		if (i == 0)
			Prev = Next;
		if (i == Points.Num() - 1)
			Next = Prev;
		FVector Dir = (Prev + Next).GetSafeNormal();
		FVector Right = (FVector::UpVector ^ Dir).GetSafeNormal();
		return Right / (Dir | Prev);
	}
	FVector GetNormal(int i, const FVector& Base) const
	{
		FVector Dir = GetDir(i);
		FVector Right = (Base ^ Dir).GetSafeNormal();
		return (Dir ^ Right).GetSafeNormal();
	}
	FVector2D GetUV(const FVector2D& Pos, int i) const
	{
		const FVector& Start = Points[i].Pos;
		const FVector& End = Points[i + 1].Pos;
		FVector Dir = End - Start;
		double Length = Dir.Size();
		double R = FMath::Atan2(Dir.Y, Dir.X);
		double SDiff, EDiff;
		if (i > 0)
		{
			FVector Prev = Points[i].Pos - Points[i - 1].Pos;
			SDiff = R - FMath::Atan2(Prev.Y, Prev.X);
		}
		else
			SDiff = 0;
		if (i < Points.Num() - 2)
		{
			FVector Next = Points[i + 2].Pos - Points[i + 1].Pos;
			EDiff = FMath::Atan2(Next.Y, Next.X) - R;
		}
		else
			EDiff = 0;
		FVector2D UV = CalcUV((const FVector2D&)Start, (const FVector2D&)End, Pos);
		double Min = -UV.Y * FMath::Tan(-SDiff / 2);
		double Max = UV.Y * FMath::Tan(-EDiff / 2) + Length;
		if ((i == 0 || UV.X > Min - DOUBLE_KINDA_SMALL_NUMBER) && (i == Points.Num() - 2 || UV.X < Max + DOUBLE_KINDA_SMALL_NUMBER))
			return FVector2D(FMath::Lerp(Points[i].Dist, Points[i + 1].Dist, (UV.X - Min) / (Max - Min)), UV.Y);
		return FVector2D(0, MAX_dbl);
	}
	FVector2D GetUV(const FVector2D& Pos) const
	{
		FVector2D BestUV(0, MAX_dbl);
		for (int i = 0; i < Points.Num() - 1; i++)
		{
			FVector2D UV = GetUV(Pos, i);
			if (FMath::Abs(BestUV.Y) > FMath::Abs(UV.Y))
				BestUV = UV;
		}
		return BestUV;
	}
	FVector2D GetUV(const FVector& Pos) const
	{
		return GetUV((const FVector2D&)Pos);
	}
	bool SolveIntersection(const FPolyline& Other, double& Dist1, double& Dist2) const
	{
		if (Points.Num() < 2 || Other.Points.Num() < 2)
			return false;
		TArray<FBox> SrcBoxes, DstBoxes;
		SrcBoxes.SetNumUninitialized(Points.Num() - 1);
		DstBoxes.SetNumUninitialized(Other.Points.Num() - 1);
		for (int i = 0; i < Points.Num() - 1; i++)
			SrcBoxes[i] = GetSegmentBounds(i);
		for (int i = 0; i < Other.Points.Num() - 1; i++)
			DstBoxes[i] = Other.GetSegmentBounds(i);
		for (int i = 0; i < SrcBoxes.Num(); i++)
		{
			for (int j = 0; j < DstBoxes.Num(); j++)
			{
				if (SrcBoxes[i].Intersect(DstBoxes[j]))
				{
					double Seg1, Seg2;
					if (DoLineSegmentsIntersect((const FVector2D&)Points[i].Pos, (const FVector2D&)Points[i + 1].Pos, (const FVector2D&)Other.Points[j].Pos, (const FVector2D&)Other.Points[j + 1].Pos, Seg1, Seg2))
					{
						Dist1 = FMath::Lerp(Points[i].Dist, Points[i + 1].Dist, Seg1);
						Dist2 = FMath::Lerp(Other.Points[j].Dist, Other.Points[j + 1].Dist, Seg2);
						return true;
					}
				}
			}
		}
		return false;
	}
	FTransform GetTransform(int i) const
	{
		return FTransform(GetDir(i).ToOrientationQuat(), Points[i].Pos);
	}
	FBox GetBounds() const
	{
		FBox Box(EForceInit::ForceInit);
		for (const FPolyPoint& Point : Points)
			Box += Point.Pos;
		return Box;
	}
	FBox GetSegmentBounds(int Index) const
	{
		double Delta = DOUBLE_KINDA_SMALL_NUMBER;
		FBox Box(EForceInit::ForceInit);
		FVector StartN = GetRight(Index);
		FVector EndN = GetRight(Index + 1);
		const FVector& StartPos = Points[Index].Pos;
		const FVector& EndPos = Points[Index + 1].Pos;
		Box += StartPos - StartN * Delta;
		Box += StartPos + StartN * Delta;
		Box += EndPos - EndN * Delta;
		Box += EndPos + EndN * Delta;
		Box.Min.Z -= Delta;
		Box.Max.Z += Delta;
		return Box;
	}
	FBox GetOrientedBounds(FTransform& OutTrans) const
	{
		FBox MinBox(EForceInit::ForceInit);
		for (int i = 0; i < Points.Num() - 1; i++)
		{
			FVector X = (Points[i + 1].Pos - Points[i].Pos).GetSafeNormal();
			FVector Y(-X.Y, X.X, X.Z);
			FTransform Trans(X, Y, (X ^ Y).GetSafeNormal(), Points[i].Pos);
			FBox Box(EForceInit::ForceInit);
			for (int j = 0; j < Points.Num(); j++)
				Box += Trans.InverseTransformPosition(Points[j].Pos);
			FVector2D MinSize(MinBox.GetSize());
			FVector2D Size(Box.GetSize());
			if (!MinBox.IsValid || MinSize.X * MinSize.Y > Size.X * Size.Y)
			{
				MinBox = Box;
				OutTrans = Trans;
			}
		}
	//	OutTrans.SetTranslation(OutTrans.TransformPosition(MinBox.GetCenter()));
		return MinBox;
	}
	TArray<FVector> GetPositions() const
	{
		TArray<FVector> Positions;
		for (int i = 0; i < Points.Num(); i++)
			Positions.Add(Points[i].Pos);
		return MoveTemp(Positions);
	}
	FPolyline Redist() const
	{
		double Dist = 0;
		TArray<FPolyPoint> NewPoints;
		NewPoints.AddUninitialized(Points.Num());
		for (int i = 0; i < Points.Num(); i++)
		{
			if (i > 0)
				Dist += (Points[i].Pos - Points[i - 1].Pos).Size();
			NewPoints[i].Pos = Points[i].Pos;
			NewPoints[i].Radian = Points[i].Radian;
			NewPoints[i].Dist = Dist;
		}
		return FPolyline(NewPoints);
	}
	FPolyline Resample(int NumSegs) const
	{
		double StartDist = Points[0].Dist;
		double EndDist = Points.Last().Dist;
		double Length = EndDist - StartDist;
		double SegLen = Length / NumSegs;
		TArray<FPolyPoint> NewPoints;
		int This = 0;
		for (int i = 0; i <= NumSegs; i++)
		{
			double Dist = StartDist + i * SegLen;
			int Next = This + 1;
			while (Dist > Points[Next].Dist + 0.01f)
			{
				This = Next;
				Next++;
			}
			double Alpha = (Dist - Points[This].Dist) / (Points[Next].Dist - Points[This].Dist);
			FVector Pos = FMath::Lerp(Points[This].Pos, Points[Next].Pos, Alpha);
			double Radian = LerpRadian(Points[This].Radian, Points[Next].Radian, Alpha);
			NewPoints.Add(FPolyPoint(Pos, Radian, Dist));
		}
		return FPolyline(NewPoints);
	}
	FPolyline Resample(double SegLen) const
	{
		double StartDist = Points[0].Dist;
		double EndDist = Points.Last().Dist;
		double Length = EndDist - StartDist;
		int NumSegs = FMath::Max(1, FMath::RoundToInt(Length / SegLen));
		SegLen = Length / NumSegs;
		TArray<FPolyPoint> NewPoints;
		int This = 0;
		for (int i = 0; i <= NumSegs; i++)
		{
			double Dist = StartDist + i * SegLen;
			int Next = This + 1;
			while (Dist > Points[Next].Dist + 0.01f)
			{
				This = Next;
				Next++;
			}
			double Alpha = (Dist - Points[This].Dist) / (Points[Next].Dist - Points[This].Dist);
			FVector Pos = FMath::Lerp(Points[This].Pos, Points[Next].Pos, Alpha);
			double Radian = LerpRadian(Points[This].Radian, Points[Next].Radian, Alpha);
			NewPoints.Add(FPolyPoint(Pos, Radian, Dist));
		}
		return FPolyline(NewPoints);
	}
	FPolyline SubCurve(double Start, double End) const
	{
		TArray<FPolyPoint> NewPoints;
		Start = FMath::Clamp(Start, Points[0].Dist, Points.Last().Dist);
		End = FMath::Clamp(End, Points[0].Dist, Points.Last().Dist);
		int StartIndex = GetPoint(Start);
		int EndIndex = GetPoint(End);
		if (End > Start)
		{
			NewPoints.Add(FPolyPoint::Lerp(Points[StartIndex], Points[StartIndex + 1], Start));
			for (int i = StartIndex + 1; i <= EndIndex; i++)
				NewPoints.Add(Points[i]);
			if (Points[EndIndex].Dist < End)
				NewPoints.Add(FPolyPoint::Lerp(Points[EndIndex], Points[EndIndex + 1], End));
		}
		else
		{
			if (Points[StartIndex].Dist < Start)
				NewPoints.Add(FPolyPoint::Lerp(Points[StartIndex], Points[StartIndex + 1], Start));
			for (int i = StartIndex; i > EndIndex; i--)
				NewPoints.Add(Points[i]);
			NewPoints.Add(FPolyPoint::Lerp(Points[EndIndex], Points[EndIndex + 1], End));
			for (FPolyPoint& NewPoint : NewPoints)
				NewPoint.Radian += DOUBLE_PI;
		}
		return FPolyline(NewPoints);
	}
	FPolyline Offset(const FVector2D& Offset, bool Straight = false) const
	{
		TArray<FPolyPoint> NewPoints;
		NewPoints.AddUninitialized(Points.Num());
		int NumSegments = Points.Num() - 1;
		for (int i = 0; i < Points.Num(); i++)
		{
			FVector Right = Straight ? GetStraightRight(i) : GetRight(i);
			NewPoints[i].Pos = Points[i].Pos + Right * Offset.X + FVector::UpVector * Offset.Y;
			NewPoints[i].Radian = Points[i].Radian;
			NewPoints[i].Dist = Points[i].Dist;
		}
		FPolyline Curve(NewPoints);
	//	Curve.CheckZig();
		return MoveTemp(Curve);
	}
	bool HasOverlapeedPoints()
	{
		for (int i = 1; i < Points.Num(); i++)
			if (Points[i].Pos.Equals(Points[i - 1].Pos))
				return true;
		return false;
	}
	bool ContainsNaN()
	{
		for (FPolyPoint& Point : Points)
			if (Point.Pos.ContainsNaN())
				return true;
		return false;
	}
	void CheckZig()
	{
		for (int i = 1; i < Points.Num()-1;)
		{
			FVector Prev = (Points[i].Pos - Points[i - 1].Pos).GetSafeNormal();
			FVector Next = (Points[i + 1].Pos - Points[i].Pos).GetSafeNormal();
			if ((Prev | Next) < -0.7071)
				Points.RemoveAt(i);
			else
				i++;
		}
	}
	UPROPERTY()
	TArray<FPolyPoint> Points;
};

class ARoadActor;
class URoadBoundary;
class URoadLane;

USTRUCT()
struct FCurveOffset
{
	GENERATED_USTRUCT_BODY()
	double Get(double S)
	{
		double EndOffset = (this + 1)->Offset;
		double EndDir = (this + 1)->Dir;
		double Length = (this + 1)->Dist - Dist;
		return FMath::CubicInterp(Offset, Dir * Length, EndOffset, EndDir * Length, S / Length);
	}
	double GetR(double S)
	{
		double EndOffset = (this + 1)->Offset;
		double EndDir = (this + 1)->Dir;
		double Length = (this + 1)->Dist - Dist;
		return FMath::Atan2(FMath::CubicInterpDerivative(Offset, Dir * Length, EndOffset, EndDir * Length, S / Length), Length);
	}
	bool operator < (const FCurveOffset& Other)const
	{
		return Dist < Other.Dist;
	}
	UPROPERTY(EditAnywhere, Category = Offset)
	double Dist = 0;

	UPROPERTY(EditAnywhere, Category = Offset)
	double Offset = 0;

	UPROPERTY(EditAnywhere, Category = Offset)
	double Dir = 0;
};

UCLASS()
class ROADBUILDER_API URoadCurve : public UObject
{
	GENERATED_BODY()
public:
	ARoadActor* GetRoad();
	FPolyline CreatePolyline(double Offset = 0);
	FPolyline CreatePolyline(double Start, double End, double Offset = 0, double Height = 0);
	void FillPolyline(FPolyline& Polyline, double Start, double End, double Offset, double Height);
	FVector2D GetPos2D(double Dist);
	FVector2D GetDir2D(double Dist);
	FVector GetPos(double Dist);
	FVector GetPos(const FVector2D& UV);
	FVector GetDir(double Dist);
	int AddLocalOffset(double Dist);
	int GetLocalOffset(double Dist, double Tolerance);
	double GetOffset(double Dist);
	double GetLocalOffset(double Dist);
	double& Length() { return Offsets.Last().Dist; }
	void UpdateOffsets(URoadCurve* SrcCurve, URoadCurve* DstCurve, int Side);
	void DeleteOffset(int Index);
	void Join(URoadCurve* RoadCurve);
	void Cut(double R_Start, double R_End);
	static TArray<FCurveOffset> CutOffsets(const TArray<FCurveOffset>& Offsets, double R_Start, double R_End);
	TArray<FCurveOffset> CutLocalOffsets(double R_Start, double R_End) { return CutOffsets(LocalOffsets, R_Start, R_End); }
	TArray<FCurveOffset> CutOffsets(double R_Start, double R_End) { return CutOffsets(Offsets, R_Start, R_End); }
	static void CleanOffsets(TArray<FCurveOffset>& Offsets);
	void CleanLocalOffsets() { CleanOffsets(LocalOffsets); }
	void CleanOffsets() { CleanOffsets(Offsets); }
#if WITH_EDITOR
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditUndo() override;
#endif
	UPROPERTY()
	TArray<FCurveOffset> LocalOffsets;

	UPROPERTY()
	TArray<FCurveOffset> Offsets;

	UPROPERTY()
	FPolyline Curve;
};


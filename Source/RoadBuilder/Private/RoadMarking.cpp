// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadMarking.h"
#include "RoadActor.h"

ARoadActor* URoadMarking::GetRoad()
{
	return Cast<ARoadActor>(GetOuter());
}

void UMarkingPoint::BuildMesh(FRoadActorBuilder& Builder)
{
	ARoadActor* Road = GetRoad();
	FVector Pos = Road->GetPos(Point);
	FVector Dir = Road->GetDir(Point.X) * (Point.Y < 0 ? -1 : 1);
	Builder.InstanceBuilder.AddInstance(Mesh, FTransform(Dir.ToOrientationQuat(), Pos), true);
}


FVector FMarkingCurvePoint::GetPos(ARoadActor* Road) { return Road->GetPos(Pos); }
FVector FMarkingCurvePoint::GetInTangent(ARoadActor* Road) { return (Road->GetPos(Pos) - Road->GetPos(Pos + In)) * 4; }
FVector FMarkingCurvePoint::GetOutTangent(ARoadActor* Road) { return (Road->GetPos(Pos + Out) - Road->GetPos(Pos)) * 4; }

void UMarkingCurve::BuildMesh(FRoadActorBuilder& Builder)
{
	ARoadActor* Road = GetRoad();
	FPolyline Curve = CreatePolyline();
	if (MarkStyle)
		MarkStyle->BuildMesh(Road, Builder.MeshBuilder, Curve);
	if (FillStyle && bClosedLoop)
		FillStyle->BuildMesh(this, Builder.MeshBuilder, Curve);
}

void UMarkingCurve::InsertPoint(const FVector2D& Pos, int& Index)
{
	Modify();
	if (Index == Points.Num() - 1)
		Index = Points.AddDefaulted();
	else
		Points.InsertDefaulted(Index);
	FMarkingCurvePoint& Point = Points[Index];
	Point.Pos = Pos;
	if (Index > 0)
	{
		FMarkingCurvePoint& Prev = Points[Index-1];
		FVector2D Dir = (Point.Pos - Prev.Pos).GetSafeNormal() * 200.0;
		Prev.Out = Dir;
		Point.In = -Dir;
	}
}

void UMarkingCurve::DeletePoint(int Index)
{

}

void UMarkingCurve::MakeClose()
{
	Modify();
	FVector2D Dir = (Points[0].Pos - Points.Last().Pos).GetSafeNormal() * 200.0;
	Points.Last().Out = Dir;
	Points[0].In = -Dir;
	bClosedLoop = true;
}

void UMarkingCurve::SetPoints(TArray<FVector2D>& UVs)
{
	Points.Empty();
	for (FVector2D& UV : UVs)
		Points.Add({ UV });
}

FPolyline UMarkingCurve::CreatePolyline()
{
	FPolyline Polyline;
	ARoadActor* Road = GetRoad();
	int Max = Points.Num() - !bClosedLoop;
	for (int i = 0; i < Max; i++)
	{
		FMarkingCurvePoint& Start = Points[i];
		FMarkingCurvePoint& End = Points[(i + 1) % Points.Num()];
		FVector StartPos = Start.GetPos(Road);
		FVector EndPos = End.GetPos(Road);
		FVector StartTangent = Start.GetOutTangent(Road);
		FVector EndTangent = End.GetInTangent(Road);
		double Length = FVector::Distance(StartPos, EndPos);
		int NumSegments = FMath::Max(1, FMath::RoundToInt(Length / 100));
		for (int j = 0; j < NumSegments + (i == Max - 1); j++)
		{
			int NumSegs = 1;
			if (j < NumSegments)
			{
				FVector StartT = FMath::CubicInterpDerivative(StartPos, StartTangent, EndPos, EndTangent, double(j) / NumSegments);
				FVector EndT = FMath::CubicInterpDerivative(StartPos, StartTangent, EndPos, EndTangent, double(j + 1) / NumSegments);
				double StartR = FMath::Atan2(StartT.Y, StartT.X);
				double EndR = FMath::Atan2(EndT.Y, EndT.X);
				NumSegs = FMath::Max(1, FMath::RoundToInt(16 * FMath::Abs(WrapRadian(EndR - StartR)) / DOUBLE_PI));
			}
			for (int k = 0; k < NumSegs; k++)
			{
				FVector Pos = FMath::CubicInterp(StartPos, StartTangent, EndPos, EndTangent, double(j) / NumSegments + double(k) / (NumSegs * NumSegments));
				Polyline.AddPoint(Pos, 0);
			}
		}
	}
	return MoveTemp(Polyline);
}
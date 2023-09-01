// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "LaneMarkStyle.h"
#include "RoadScene.h"

void ULaneMarkStyle::BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve)
{
	TArray<double> DashOffsets, SolidOffsets;
	AJunctionActor* Junction = Cast<AJunctionActor>(Cast<ARoadActor>(Caller)->GetAttachParentActor());
	switch (MarkType)
	{
	case ELaneMarkType::Dash:
		DashOffsets.Add(0);
		break;
	case ELaneMarkType::Solid:
		SolidOffsets.Add(0);
		break;
	case ELaneMarkType::DashDash:
		DashOffsets.Add(-Separation);
		DashOffsets.Add(+Separation);
		break;
	case ELaneMarkType::DashSolid:
		DashOffsets.Add(-Separation);
		SolidOffsets.Add(+Separation);
		break;
	case ELaneMarkType::SolidDash:
		SolidOffsets.Add(-Separation);
		DashOffsets.Add(+Separation);
		break;
	case ELaneMarkType::SolidSolid:
		SolidOffsets.Add(-Separation);
		SolidOffsets.Add(+Separation);
		break;
	}
	if (DashOffsets.Num())
	{
		double Start = Curve.Points[0].Dist;
		double End = Curve.Points.Last().Dist;
		double Length = End - Start;
		int NumSegments = FMath::RoundToInt(Length / (DashLength + DashSpacing));
		double Step = Length / NumSegments;
		double ActualSpacing = Step - DashLength;
		for (int i = 0; i < NumSegments; i++)
		{
			double Base = Start + Step * i;
			FPolyline SubCurve = Curve.SubCurve(Base + ActualSpacing / 2, Base + ActualSpacing / 2 + DashLength);
			for (double Offset : DashOffsets)
			{
				FPolyline LeftCurve = SubCurve.Offset(FVector2D(Offset - Width / 2, 0), true);
				FPolyline RightCurve = SubCurve.Offset(FVector2D(Offset + Width / 2, 0), true);
				if (Junction)
				{
					Junction->FixHeight(LeftCurve);
					Junction->FixHeight(RightCurve);
				}
				Builder.AddStrip(Material, LeftCurve, RightCurve);
			}
		}
	}
	for (double Offset : SolidOffsets)
	{
		FPolyline LeftCurve = Curve.Offset(FVector2D(Offset - Width / 2, 0), true);
		FPolyline RightCurve = Curve.Offset(FVector2D(Offset + Width / 2, 0), true);
		if (Junction)
		{
			Junction->FixHeight(LeftCurve);
			Junction->FixHeight(RightCurve);
		}
		check(!LeftCurve.ContainsNaN());
		check(!RightCurve.ContainsNaN());
		Builder.AddStrip(Material, LeftCurve, RightCurve);
	}
}

void UCrosswalkStyle::BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve)
{
	FPolyline Points = Curve.Resample(DashLength + DashGap);
	AJunctionActor* Junction = Cast<AJunctionActor>(Cast<ARoadActor>(Caller)->GetAttachParentActor());
	for (int i = 0; i < Points.Points.Num(); i++)
	{
		FVector VDir = Points.GetStraightDir(i);
		FVector HDir(-VDir.Y, VDir.X, VDir.Z);
		FVector& Pos = Points.Points[i].Pos;
		FPolyline LeftCurve({ FPolyPoint(Pos + VDir * DashLength / 2 - HDir * Width / 2, 0), FPolyPoint(Pos + VDir * DashLength / 2 + HDir * Width / 2, Width) });
		FPolyline RightCurve({ FPolyPoint(Pos - VDir * DashLength / 2 - HDir * Width / 2, 0), FPolyPoint(Pos - VDir * DashLength / 2 + HDir * Width / 2, Width) });
		if (Junction)
		{
			Junction->FixHeight(LeftCurve);
			Junction->FixHeight(RightCurve);
		}
		Builder.AddStrip(Material, LeftCurve, RightCurve);
	}
}

void UPolygonMarkStyle::BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve)
{
	struct FLine
	{
		FVector GetPoint(double S) const
		{
			return FMath::Lerp(Start, End, S);
		}
		FVector GetDir()
		{
			return (End - Start).GetSafeNormal();
		}
		FVector GetRight()
		{
			FVector Dir = GetDir();
			return FVector(-Dir.Y, Dir.X, Dir.Z);
		}
		FLine LeftLine(double Offset)
		{
			FVector Right = GetRight();
			return { Start - Right * Offset, End - Right * Offset };
		}
		FLine RightLine(double Offset)
		{
			FVector Right = GetRight();
			return { Start + Right * Offset, End + Right * Offset };
		}
		FVector Start;
		FVector End;

	};
	UMarkingCurve* Marking = Cast<UMarkingCurve>(Caller);
	ARoadActor* Road = Marking->GetRoad();
	double Margin = 10.0;
	TArray<FLine> Lines;
	const FVector& Origin = Curve.Points[0].Pos;
	FTransform Trans(FRotator(0, Marking->Orientation, 0), Origin);
	FBox Box(EForceInit::ForceInit);
	for (const FPolyPoint& Point : Curve.Points)
		Box += Trans.InverseTransformPosition(Point.Pos);
	AJunctionActor* Junction = Cast<AJunctionActor>(Road->GetAttachParentActor());
	auto CreateLine = [&Trans](const FVector& Start, const FVector& End)->FLine
	{
		return { Trans.TransformPosition(Start), Trans.TransformPosition(End) };
	};
	switch (MarkType)
	{
	case EPolygonMarkType::Solid:
	{
		TArray<FVector> Points = Curve.GetPositions();
		if (Junction)
			Junction->FixHeight(Points);
		Builder.AddPolygon(Material, nullptr, Points);
		break;
	}
	case EPolygonMarkType::Striped:
	{
		FVector Start = Box.Min - FVector(Margin, Margin, 0);
		FVector Size = Box.GetSize() + FVector(Margin, Margin, 0) * 2;
		int NumRows = FMath::RoundToInt(Size.Y / LineSpace);
		double StepY = Size.Y / NumRows;
		for (int i = 0; i < NumRows; i++)
		{
			Lines.Add(CreateLine(Start, Start + FVector(Size.X, 0, 0)));
			Start.Y += StepY;
		}
		break;
	}
	case EPolygonMarkType::Crosshatch:
	{
		FVector Start = Box.Min - FVector(Margin, Margin, 0);
		FVector Size = Box.GetSize() + FVector(Margin, Margin, 0) * 2;
		int NumCols = FMath::RoundToInt(Size.X / LineSpace);
		int NumRows = FMath::RoundToInt(Size.Y / LineSpace);
		double StepX = Size.X / NumCols;
		double StepY = Size.Y / NumRows;
		for (int i = 0; i < NumRows; i++)
		{
			Lines.Add(CreateLine(Start, Start + FVector(Size.X, 0, 0)));
			Start.Y += StepY;
		}
		Start = Box.Min - FVector(Margin, Margin, 0);
		for (int i = 0; i < NumCols; i++)
		{
			Lines.Add(CreateLine(Start, Start + FVector(0, Size.Y, 0)));
			Start.X += StepX;
		}
		break;
	}
	case EPolygonMarkType::Chevron:
	{
	//	FVector Center =Box.GetCenter();
		FVector Size = Box.GetSize();
		double Tan = FMath::Tan(FMath::DegreesToRadians(ChevrenAngle));
		double OffsetY = Size.X * Tan * 0.5;
		FVector Start = FVector(0, Box.Min.Y - OffsetY, 0);
		Size += FVector(0, OffsetY, 0) + FVector(Margin, Margin, 0) * 2;
		double Sin = FMath::Sin(FMath::DegreesToRadians(ChevrenAngle));
		int NumRows = FMath::RoundToInt(Size.Y / LineSpace);
		double StepY = Size.Y / NumRows;
		for (int i = 0; i < NumRows; i++)
		{
			Lines.Add(CreateLine(Start, Start + FVector(-Size.X / Sin, Size.X, 0)));
			Lines.Add(CreateLine(Start, Start + FVector(Size.X / Sin, Size.X, 0)));
			Start.Y += StepY;
		}
		break;
	}
	}
	for (FLine& Line : Lines)
	{
		TArray<double> Hits = { 0 };
		for (int i = 0; i < Curve.Points.Num() - 1; i++)
		{
			const FVector2D& P0 = (FVector2D&)Curve.Points[i].Pos;
			const FVector2D& P1 = (FVector2D&)Curve.Points[i + 1].Pos;
			double Hit0, Hit1;
			if (DoLineSegmentsIntersect((FVector2D&)Line.Start, (FVector2D&)Line.End, P0, P1, Hit0, Hit1))
				Hits.Add(Hit0);
		}
		auto SolvePoints = [](const FLine& Line, TArray<double>& Hits)->TArray<FVector>
		{
			Hits.Sort();
			int Start = Hits.Num() % 2 ? 1 : 0;
			TArray<FVector> Points;
			for (int i = Start; i < Hits.Num(); i += 2)
			{
				Points.Add(Line.GetPoint(Hits[i]));
				Points.Add(Line.GetPoint(Hits[i + 1]));
			}
			return MoveTemp(Points);
		};
		TArray<FVector> LeftPoints = SolvePoints(Line.LeftLine(5), Hits);
		TArray<FVector> RightPoints = SolvePoints(Line.RightLine(5), Hits);
		if (Junction)
		{
			Junction->FixHeight(LeftPoints);
			Junction->FixHeight(RightPoints);
		}
		for (int i = 0; i < LeftPoints.Num(); i += 2)
			Builder.AddTriangles(Material, { FIndex3i(0,1,2), FIndex3i(2,1,3) }, { RightPoints[i], RightPoints[i + 1], LeftPoints[i], LeftPoints[i + 1] }, FVector::UpVector);
	}
}
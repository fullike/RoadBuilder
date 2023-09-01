// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "LaneShape.h"
#include "Settings.h"

UMaterialInterface* ULaneShape::GetSurfaceMaterial()
{
	for (FLaneCrossSection& CrossSection : CrossSections)
	{
		if (CrossSection.Alignment == ELaneAlignment::Up)
			return CrossSection.Material;
	}
	return nullptr;
}

UMaterialInterface* ULaneShape::GetBackfaceMaterial()
{
	for (FLaneCrossSection& CrossSection : CrossSections)
	{
		if (CrossSection.Alignment == ELaneAlignment::Down)
			return CrossSection.Material;
	}
	return nullptr;
}

void ULaneShape::BuildMesh(FRoadMesh& Builder, const FPolyline& LeftCurve, const FPolyline& RightCurve, bool SkipLeftBorder, bool SkipRightBorder)
{
	for (FLaneCrossSection& CrossSection : CrossSections)
	{
		if (SkipLeftBorder && CrossSection.Alignment == ELaneAlignment::Left)
			continue;
		if (SkipRightBorder && CrossSection.Alignment == ELaneAlignment::Right)
			continue;
		FVector2D UVScale = GetMutableDefault<USettings_Global>()->UVScale * CrossSection.UVScale;
		if (CrossSection.Alignment > ELaneAlignment::Down)
		{
			TArray<FVector> Positions;
			TArray<FVector2D> UVs;
			const FPolyline& Curve = CrossSection.Alignment == ELaneAlignment::Right ? RightCurve : LeftCurve;
			FBox Bounds = Curve.GetBounds();
			Positions.Reset(CrossSection.Points.Num() * Curve.Points.Num());
			UVs.Reset(CrossSection.Points.Num() * Curve.Points.Num());
			for (int i = 0; i < Curve.Points.Num(); i++)
			{
				FTransform Trans = Curve.GetTransform(i);
				double Dist = 0;
				TArray<FVector2D> Points = CrossSection.Points;
				if (CrossSection.ClampZ)
				{
					double LocalMinZ = Bounds.Min.Z - Trans.GetLocation().Z;
					double LocalMaxZ = Bounds.Max.Z - Trans.GetLocation().Z;
					for (FVector2D& Point : Points)
						Point.Y = FMath::Clamp(Point.Y, LocalMinZ, LocalMaxZ);
				}
				for (int j = 0; j < Points.Num(); j++)
				{
					FVector2D& Point = Points[j];
					Positions.Add(Trans.TransformPosition(FVector(0, Point.X, Point.Y)));
					UVs.Add(FVector2D(Curve.Points[i].Dist, Dist) * UVScale);
					if (j + 1 < Points.Num())
						Dist += (Points[j + 1] - Points[j]).Size();
				}
			}
			Builder.AddPolygons(CrossSection.Material, Positions, UVs, CrossSection.Points.Num() - 1, Curve.Points.Num() - 1);
		}
		else if (CrossSection.Points.Num() == 2)
		{
			const FPolyline& StartCurve = (CrossSection.Alignment == ELaneAlignment::Up) ? LeftCurve : RightCurve;
			const FPolyline& EndCurve = (CrossSection.Alignment == ELaneAlignment::Up) ? RightCurve : LeftCurve;
			Builder.AddStrip(CrossSection.Material, StartCurve.Offset(CrossSection.Points[0]), EndCurve.Offset(CrossSection.Points[1]));
		}
	}
}
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshDescriptionBuilder.h"
#include "ProceduralMeshComponent.h"
#include "ConstrainedDelaunay2.h"
#include "Misc/FileHelper.h"
#include "RoadCurve.h"

using namespace UE::Geometry;

class ROADBUILDER_API FStaticRoadMesh
{
public:
	FStaticRoadMesh();
	UStaticMesh* CreateMesh(UObject* Outer, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags);
	FPolygonGroupID GetGroupID(UMaterialInterface* Material);
	void AddStrip(UMaterialInterface* Material, const FPolyline& LeftCurve, const FPolyline& RightCurve);
	void AddPolygon(UMaterialInterface* SurfaceMaterial, UMaterialInterface* BackfaceMaterial, const TArray<FVector>& Points);
	void AddPolygons(UMaterialInterface* Material, const TArray<FVector>& Positions, const TArray<FVector2D>& UVs, int NumCols, int NumRows);
	void AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector>& Vertices, const FVector& Normal);
	void AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector2D>& Vertices, const FVector& Normal)
	{
		TArray<FVector> Positions;
		Positions.AddUninitialized(Vertices.Num());
		for (int i = 0; i < Vertices.Num(); i++)
			Positions[i] = FVector(Vertices[i], 0);
		AddTriangles(Material, Triangles, Positions, Normal);
	}
	void Build(USceneComponent* Component);
	TMap<UMaterialInterface*, FPolygonGroupID> PolygonGroups;
	TSharedPtr<FMeshDescription> MeshDescription;
	TSharedPtr<FStaticMeshAttributes> MeshAttributes;
	TSharedPtr<FMeshDescriptionBuilder> Builder;
};

class ROADBUILDER_API FProcRoadMesh
{
public:
	void AddStrip(UMaterialInterface* Material, const FPolyline& LeftCurve, const FPolyline& RightCurve);
	void AddPolygons(UMaterialInterface* Material, const TArray<FVector>& Positions, const TArray<FVector2D>& UVs, int NumCols, int NumRows);
	void AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector>& Vertices);
	void AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector2D>& Vertices)
	{
		TArray<FVector> Positions;
		Positions.AddUninitialized(Vertices.Num());
		for (int i = 0; i < Vertices.Num(); i++)
			Positions[i] = FVector(Vertices[i], 0);
		AddTriangles(Material, Triangles, Positions);
	}
	void Build(USceneComponent* Component);
	TMap<UMaterialInterface*, FProcMeshSection> Sections;
};

#define USE_PROC_ROAD_MESH	0

#if USE_PROC_ROAD_MESH
	typedef FProcRoadMesh FRoadMesh;
#else
	typedef FStaticRoadMesh FRoadMesh;
#endif

DECLARE_CYCLE_STAT(TEXT("AddStrip"), STAT_AddStrip, STATGROUP_RoadBuilder);
DECLARE_CYCLE_STAT(TEXT("BuildMesh"), STAT_BuildMesh, STATGROUP_RoadBuilder);

class FInstanceBuilder
{
public:
	struct FInstances
	{
		TArray<FTransform> Instances;
		bool bForceISM = false;
	};
	void AttachToActor(AActor* Actor);
	void AddInstance(UStaticMesh* Mesh, const FTransform& Trans, bool bForceISM = false)
	{
		if (!Instances.Contains(Mesh))
			Instances.Add(Mesh).bForceISM = bForceISM;
		Instances[Mesh].Instances.Add(Trans);
	}
	TMap<UStaticMesh*, FInstances> Instances;
};

class FDecalBuilder
{
public:
	struct FDecal
	{
		UMaterialInterface* Material;
		FTransform Trans;
		FVector2D Size;
	};
	void AttachToActor(AActor* Actor);
	void AddDecal(UMaterialInterface* Material, const FTransform& Trans, const FVector2D& Size)
	{
		Decals.Add({ Material, Trans, Size });
	}
	TArray<FDecal> Decals;
};

struct FRoadActorBuilder
{
	FRandomStream Stream;
	FRoadMesh MeshBuilder;
	FInstanceBuilder InstanceBuilder;
	FDecalBuilder DecalBuilder;
};

inline void BuildStrip(const FPolyline& LeftCurve, const FPolyline& RightCurve, TFunction<void(int,int,bool)>&& AddTriangle)
{
	int CurLeft = 0, CurRight = 0;
	bool Ascending = LeftCurve.Points.Last().Dist > LeftCurve.Points[0].Dist || RightCurve.Points.Last().Dist > RightCurve.Points[0].Dist;
	auto Behind = [&]()
	{
		return Ascending ? LeftCurve.Points[CurLeft + 1].Dist < RightCurve.Points[CurRight + 1].Dist : LeftCurve.Points[CurLeft + 1].Dist > RightCurve.Points[CurRight + 1].Dist;
	};
	while (true)
	{
		bool CanLeft = CurLeft + 1 < LeftCurve.Points.Num();
		bool CanRight = CurRight + 1 < RightCurve.Points.Num();
		if (!CanLeft && !CanRight)
			break;
		bool LeftSide = (CanLeft && CanRight) ? Behind() : CanLeft;
		AddTriangle(CurLeft, CurRight, LeftSide);
		if (LeftSide)
		{
			if (CurLeft + 1 < LeftCurve.Points.Num())
				CurLeft++;
		}
		else
		{
			if (CurRight + 1 < RightCurve.Points.Num())
				CurRight++;
		}
	}
}

inline void LoadPoints(TArray<FVector>& Points, const FString& FilePath)
{
	TArray<FString> FileLines;
	FFileHelper::LoadFileToStringArray(FileLines, *FilePath);
	for (FString& FileLine : FileLines)
	{
		TArray<FString> Strs;
		FileLine.ParseIntoArray(Strs, TEXT(","));
		if (Strs.Num() == 3)
		{
			FVector Point;
			for (int i = 0; i < Strs.Num(); i++)
				Point[i] = FCString::Atod(*Strs[i]);
			Points.Add(Point);
		}
	}
}

inline void SavePoints(TArray<FVector>& Points, const FString& FilePath)
{
	TArray<FString> FileLines;
	for (FVector& Point : Points)
		FileLines.Add(FString::Printf(TEXT("%.16f,%.16f,%.16f"), Point.X, Point.Y, Point.Z));
	FFileHelper::SaveStringArrayToFile(FileLines, *FilePath);
}
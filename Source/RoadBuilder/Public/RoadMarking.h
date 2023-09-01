// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadMesh.h"
#include "LaneMarkStyle.h"
#include "Engine/StaticMeshActor.h"
#include "RoadMarking.generated.h"

class ARoadScene;

UCLASS()
class ROADBUILDER_API URoadMarking : public UObject
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(FRoadActorBuilder& Builder) {}
	ARoadActor* GetRoad();
};

UCLASS()
class ROADBUILDER_API UMarkingPoint : public URoadMarking
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(FRoadActorBuilder& Builder);

	UPROPERTY(EditAnywhere, Category = Point)
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, Category = Point)
	FVector2D Point;
};

USTRUCT()
struct FMarkingCurvePoint
{
	GENERATED_USTRUCT_BODY()
	
	FVector GetPos(ARoadActor* Road);
	FVector GetInTangent(ARoadActor* Road);
	FVector GetOutTangent(ARoadActor* Road);
	FVector2D GetUV(int Index)
	{
		switch (Index)
		{
		case 1: return Pos + In;
		case 2: return Pos + Out;
		}
		return Pos;
	}
	void ApplyDelta(int Index, const FVector2D& Delta)
	{
		*(&Pos + Index) += Delta;
	}
	UPROPERTY(EditAnywhere, Category = Point)
	FVector2D Pos;

	UPROPERTY(EditAnywhere, Category = Point)
	FVector2D In = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Point)
	FVector2D Out = FVector2D::ZeroVector;
};

UCLASS()
class ROADBUILDER_API UMarkingCurve : public URoadMarking
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(FRoadActorBuilder& Builder);
	FVector2D Center()
	{
		FVector2D C(0, 0);
		for (FMarkingCurvePoint& Point : Points)
			C += Point.Pos / Points.Num();
		return C;
	}
	bool IsEndPoint(int Index)
	{
		return Index == 0 || Index == Points.Num() - 1;
	}
	void InsertPoint(const FVector2D& Pos, int& Index);
	void DeletePoint(int Index);
	void MakeClose();
	void SetPoints(TArray<FVector2D>& UVs);
	FPolyline CreatePolyline();

	UPROPERTY(EditAnywhere, Category = Style, meta = (DisallowedClasses = "PolygonMarkStyle"))
	UBaseMarkStyle* MarkStyle = nullptr;

	UPROPERTY(EditAnywhere, Category = Style, meta = (EditCondition = "bClosedLoop"))
	UPolygonMarkStyle* FillStyle = nullptr;

	UPROPERTY(EditAnywhere, Category = Style, meta = (EditCondition = "bClosedLoop"))
	double Orientation = 0;

//	UPROPERTY(EditAnywhere, Category = Style, meta = (EditCondition = "bClosedLoop"))
//	FVector2D FillOffset;

	UPROPERTY(EditAnywhere, Category = Curve)
	TArray<FMarkingCurvePoint> Points;

	UPROPERTY(EditAnywhere, Category = Curve)
	bool bClosedLoop = false;
};
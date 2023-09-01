// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadCurve.h"
#include "RoadMesh.h"
#include "LaneShape.generated.h"

UENUM()
enum class ELaneAlignment : uint8
{
	Up,
	Down,
	Right,
	Left,
};

USTRUCT()
struct FLaneCrossSection
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = CrossSection)
	UMaterialInterface* Material = nullptr;

	UPROPERTY(EditAnywhere, Category = CrossSection)
	ELaneAlignment Alignment;

	UPROPERTY(EditAnywhere, Category = CrossSection)
	FVector2D UVScale = FVector2D::UnitVector;

	UPROPERTY(EditAnywhere, Category = CrossSection)
	TArray<FVector2D> Points;

	UPROPERTY(EditAnywhere, Category = CrossSection)
	uint32 ClampZ : 1;
};

UCLASS()
class ROADBUILDER_API ULaneShape : public UObject
{
	GENERATED_BODY()
public:
	UMaterialInterface* GetSurfaceMaterial();
	UMaterialInterface* GetBackfaceMaterial();
	void BuildMesh(FRoadMesh& Builder, const FPolyline& LeftCurve, const FPolyline& RightCurve, bool SkipLeftBorder, bool SkipRightBorder);

	UPROPERTY(EditAnywhere, Category = Shape)
	TArray<FLaneCrossSection> CrossSections;
};
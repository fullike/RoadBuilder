// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadCurve.h"
#include "RoadMesh.h"
#include "Materials/MaterialInterface.h"
#include "LaneMarkStyle.generated.h"

UENUM()
enum class ELaneMarkType : uint8
{
	Dash,
	Solid,
	DashDash,
	DashSolid,
	SolidDash,
	SolidSolid,
};

UCLASS()
class ROADBUILDER_API UBaseMarkStyle : public UObject
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve) {}
	UPROPERTY(EditAnywhere, Category = Style)
	UMaterialInterface* Material = nullptr;
};

UCLASS()
class ROADBUILDER_API ULaneMarkStyle : public UBaseMarkStyle
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve);
	FString GetXodrType()
	{
		if (MarkType == ELaneMarkType::Dash) return TEXT("broken");
		if (MarkType == ELaneMarkType::Solid) return TEXT("solid");
		if (MarkType == ELaneMarkType::DashDash) return TEXT("broken broken");
		if (MarkType == ELaneMarkType::DashSolid) return TEXT("broken solid");
		if (MarkType == ELaneMarkType::SolidDash) return TEXT("solid broken");
		if (MarkType == ELaneMarkType::SolidSolid) return TEXT("solid solid");
		return TEXT("none");
	}
	FString GetXodrColor()
	{
		if (Material)
		{
			FString Name = Material->GetName();
			if (Name.Contains(TEXT("yellow"))) return TEXT("yellow");
			if (Name.Contains(TEXT("white"))) return TEXT("white");
		}
		return TEXT("standard");
	}
	UPROPERTY(EditAnywhere, Category = Style)
	ELaneMarkType MarkType = (ELaneMarkType)0;

	UPROPERTY(EditAnywhere, Category = Style)
	double Width = 12.5f;

	UPROPERTY(EditAnywhere, Category = Style)
	double Separation = 12.5f;

	UPROPERTY(EditAnywhere, Category = Style)
	double DashLength = 150.f;

	UPROPERTY(EditAnywhere, Category = Style)
	double DashSpacing = 150.f;
};

UCLASS()
class ROADBUILDER_API UCrosswalkStyle : public UBaseMarkStyle
{
	GENERATED_BODY()
public:
	virtual void BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve);

	UPROPERTY(EditAnywhere, Category = Style)
	double Width = 250;

	UPROPERTY(EditAnywhere, Category = Style)
	double BorderWidth = 0;

	UPROPERTY(EditAnywhere, Category = Style)
	double DashLength = 50.f;

	UPROPERTY(EditAnywhere, Category = Style)
	double DashGap = 50.f;
};

UENUM()
enum class EPolygonMarkType : uint8
{
	Solid,
	Striped,
	Crosshatch,
	Chevron,
};

UCLASS()
class ROADBUILDER_API UPolygonMarkStyle : public UBaseMarkStyle
{
	GENERATED_BODY()
public:
	void BuildMesh(UObject* Caller, FRoadMesh& Builder, const FPolyline& Curve);

	UPROPERTY(EditAnywhere, Category = Style)
	EPolygonMarkType MarkType;

	UPROPERTY(EditAnywhere, Category = Style)
	double LineSpace = 200;

	UPROPERTY(EditAnywhere, Category = Style, meta = (ClampMin = 30, ClampMax = 60))
	double ChevrenAngle = 45;
};

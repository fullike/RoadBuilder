// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RoadMesh.h"
#include "RoadProps.generated.h"

USTRUCT()
struct FRoadProp
{
	GENERATED_USTRUCT_BODY()

	UObject* GetAsset(FRandomStream& Stream)
	{
		return Assets.Num() ? Assets[Stream.RandHelper(Assets.Num())] : nullptr;
	}
	FVector GetOffset(FRandomStream& Stream)
	{
		return Offset + FVector((Stream.FRand() - 0.5f) * RandomOffset.X, (Stream.FRand() - 0.5f) * RandomOffset.Y, (Stream.FRand() - 0.5f) * RandomOffset.Z);
	}
	FVector GetScale(FRandomStream& Stream)
	{
		return Scale + FVector((Stream.FRand() - 0.5f) * RandomScale.X, (Stream.FRand() - 0.5f) * RandomScale.Y, (Stream.FRand() - 0.5f) * RandomScale.Z);
	}
	FRotator GetRotation(FRandomStream& Stream)
	{
		return FRotator(Rotation.Pitch + (Stream.FRand() - 0.5f) * RandomRotation.Pitch, Rotation.Yaw + (Stream.FRand() - 0.5f) * RandomRotation.Yaw, Rotation.Roll + (Stream.FRand() - 0.5f) * RandomRotation.Roll);
	}

	UPROPERTY(EditAnywhere, Category = Prop)
	TArray<UObject*> Assets;

	UPROPERTY(EditAnywhere, Category = Prop)
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Prop)
	FVector Scale = FVector(1, 1, 1);

	UPROPERTY(EditAnywhere, Category = Prop)
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = Prop)
	FVector RandomOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Prop)
	FVector RandomScale = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Prop)
	FRotator RandomRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = Prop)
	double Spacing = 0;

	UPROPERTY(EditAnywhere, Category = Prop)
	double Start = 0;

	UPROPERTY(EditAnywhere, Category = Prop)
	double End = 1;

	UPROPERTY(EditAnywhere, Category = Prop)
	uint32 Fill : 1;

	UPROPERTY(EditAnywhere, Category = Filter)
	uint8 Select = 1;

	UPROPERTY(EditAnywhere, Category = Filter)
	uint8 Base = 0;

	UPROPERTY(EditAnywhere, Category = Filter)
	uint8 Of = 1;
};

UCLASS()
class ROADBUILDER_API URoadProps : public UObject
{
	GENERATED_BODY()
public:
	void Generate(ARoadActor* Road, const FPolyline& Baseline, FRoadActorBuilder& Builder);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif
	UPROPERTY(EditAnywhere, Category = Props)
	TArray<FRoadProp> Props;
};
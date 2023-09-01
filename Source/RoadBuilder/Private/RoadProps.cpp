// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadProps.h"
#include "RoadScene.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void URoadProps::Generate(ARoadActor* Road, const FPolyline& Baseline, FRoadActorBuilder& Builder)
{
	for (FRoadProp& Prop : Props)
	{
		float Start = Prop.Start;
		float End = Prop.End;
		if (Prop.Spacing > 0)
		{
			FPolyline Line_Resample = Baseline.Resample(Prop.Spacing);
			FPolyline Line_Offset = Line_Resample.Offset(FVector2D(Prop.Offset.Y, 0));
			FPolyline Line = Line_Offset.Resample(Prop.Spacing);
			float Length = Line.Points.Last().Dist;
			int StartIndex = Line.GetPoint(Start * Length);
			int EndIndex = FMath::Min(Line.GetPoint(End * Length), Line.Points.Num() - 2);
			for (int i = StartIndex; i <= EndIndex; i++)
			{
				int Index = (i % Prop.Of - Prop.Base + Prop.Of) % Prop.Of;
				if (Index >= Prop.Select)
					continue;
				FVector Pos0 = Line.Points[i].Pos;
				FVector Pos1 = Line.Points[i + 1].Pos;
				FVector N = Pos1 - Pos0;
				float Size = N.Size();
				N /= Size;
				FVector Scale(Prop.Fill ? Size / Prop.Spacing : 1, 1, 1);
				FVector Up(0, 0, 1);
				FVector Right = (Up ^ N).GetSafeNormal();
				Up = (N ^ Right).GetSafeNormal();
				FVector RandomOffset = FVector(Builder.Stream.FRand() - 0.5f, Builder.Stream.FRand() - 0.5f, Builder.Stream.FRand() - 0.5f) * Prop.RandomOffset;
				FVector NewP = Pos0 + N * ((Prop.Offset.X + RandomOffset.X) * Scale.X) + Right * RandomOffset.Y + Up * (Prop.Offset.Z + RandomOffset.Z);
				FTransform Trans(FQuat(N.Rotation()) * FQuat(Prop.GetRotation(Builder.Stream)), NewP, Prop.GetScale(Builder.Stream) * Scale);
				UObject* Asset = Prop.GetAsset(Builder.Stream);
				if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset))
				{
					Builder.InstanceBuilder.AddInstance(Mesh, Trans);
				}
				else if (UBlueprint* BP = Cast<UBlueprint>(Asset))
				{
					AActor* Actor = GetWorld()->SpawnActor<AActor>(BP->GeneratedClass);
					Actor->SetActorTransform(Trans);
					Actor->AttachToActor(Road, FAttachmentTransformRules::KeepWorldTransform);
				}
				else if (UMaterialInterface* Material = Cast<UMaterialInterface>(Asset))
				{
					Builder.DecalBuilder.AddDecal(Material, Trans, FVector2D(256, 256));
				}
			}
		}
	}
}
#if WITH_EDITOR
void URoadProps::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(URoadProps, Props))
		{
			ARoadScene* Scene = Cast<ARoadScene>(UGameplayStatics::GetActorOfClass(GWorld, ARoadScene::StaticClass()));
			if (Scene)
				Scene->Rebuild();
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
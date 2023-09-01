// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "GroundActor.h"
#include "RoadScene.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

double FGroundPoint::GetDist(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	TArray<FJunctionSlot>& Slots = RoadSlots[Road];
	int Slot = Index / 2;
	return Index % 2 ? (Slot < Slots.Num() ? Slots[Slot].InputDist() : Road->Length()) : (Slot > 0 ? Slots[Slot - 1].OutputDist() : 0);
}

FVector FGroundPoint::GetPos(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	URoadBoundary* RoadEdge = Road->GetRoadBorder(Side);
	return RoadEdge->GetPos(GetDist(RoadSlots));
}

FVector FGroundPoint::GetDir(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	URoadBoundary* RoadEdge = Road->GetRoadBorder(Side);
	return RoadEdge->GetDir(GetDist(RoadSlots)) * (Side ? -1.0 : 1.0);
}

FGroundPoint FGroundPoint::PrevPoint(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	if (SegmentEnd())
		return { Road, Side, Side ? Index + 1 : Index - 1 };
	int SlotIndex = Side ? Index / 2 : Index / 2 - 1;
	TArray<FJunctionSlot>& Slots = RoadSlots[Road];
	if (Slots.IsValidIndex(SlotIndex))
	{
		FJunctionSlot& Slot = Slots[SlotIndex];
		AJunctionActor* Junction = Slot.Junction;
		int GateIndex = Side ? Slot.InputGateIndex() : Slot.OutputGateIndex();
		for (int Idx = 1; Idx < Junction->Gates.Num(); Idx++)
		{
			FJunctionGate& Gate = Junction->Gates[(GateIndex - Idx + Junction->Gates.Num()) % Junction->Gates.Num()];
			int PrevSide = Gate.Sign > 0 ? 1 : 0;
			if (!Gate.Road->bHasGround)
				continue;
			TArray<FJunctionSlot>& PrevSlots = RoadSlots[Gate.Road];
			for (int i = 0; i < PrevSlots.Num(); i++)
				if (PrevSlots[i].Junction == Junction)
					return { Gate.Road, PrevSide, Gate.Sign > 0 ? (i + 1) * 2 : i * 2 + 1 };
		}
	}
	return { nullptr };
}

FGroundPoint FGroundPoint::NextPoint(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	if (SegmentStart())
		return { Road, Side, Side ? Index - 1 : Index + 1 };
	int SlotIndex = Side ? Index / 2 - 1 : Index / 2;
	TArray<FJunctionSlot>& Slots = RoadSlots[Road];
	if (Slots.IsValidIndex(SlotIndex))
	{
		FJunctionSlot& Slot = Slots[SlotIndex];
		AJunctionActor* Junction = Slot.Junction;
		int GateIndex = Side ? Slot.OutputGateIndex() : Slot.InputGateIndex();
		for (int Idx = 1; Idx < Junction->Gates.Num(); Idx++)
		{
			FJunctionGate& Gate = Junction->Gates[(GateIndex + Idx) % Junction->Gates.Num()];
			int NextSide = Gate.Sign > 0 ? 0 : 1;
			if (!Gate.Road->bHasGround)
				continue;
			TArray<FJunctionSlot>& NextSlots = RoadSlots[Gate.Road];
			for (int i = 0; i < NextSlots.Num(); i++)
				if (NextSlots[i].Junction == Junction)
					return { Gate.Road, NextSide, Gate.Sign > 0 ? (i + 1) * 2 : i * 2 + 1 };
		}
	}
	return { nullptr };
}

AGroundActor::AGroundActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if USE_PROC_ROAD_MESH
	RootComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RootComponent"));
#else
	RootComponent = CreateDefaultSubobject<URoadMeshComponent>(TEXT("RootComponent"));
#endif
}

void AGroundActor::AddManualPoint(const FVector& Pos, int& Index)
{
	int Idx = ManualPoints.Add(Pos);
	FGroundPoint NewPt = { nullptr, 0, Idx };
	if (Index)
		Index = Points.Add(NewPt);
	else
		Points.Insert(NewPt, 0);
}

void AGroundActor::Destroy()
{
	ForEachAttachedActors([&](AActor* Actor)->bool
	{
		Actor->Destroy();
		return true;
	});
	AActor::Destroy();
}

void AGroundActor::Join(AGroundActor* Other, int& Index)
{
	if (!Index)
	{
		for (int i = Other->Points.Num() - 1; i >= 0; i--)
		{
			FGroundPoint& P = Other->Points[i];
			if (P.Road)
				Points.Insert(P, 0);
			else
				AddManualPoint(Other->ManualPoints[P.Index], Index);
		}
	}
	else
	{
		for (int i = 0; i < Other->Points.Num(); i++)
		{
			FGroundPoint& P = Other->Points[i];
			if (P.Road)
				Index = Points.Add(P);
			else
				AddManualPoint(Other->ManualPoints[P.Index], Index);
		}
	}
}

void AGroundActor::BuildMesh(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	FRoadMesh Builder;
	if (bClosedLoop)
	{
		TArray<FVector> Vertices = GetVertices(RoadSlots);
		if (!Material)
			Material = GetMutableDefault<USettings_Global>()->DefaultGroundMaterial.LoadSynchronous();
		Builder.AddPolygon(Material, nullptr, Vertices);
		Builder.Build(GetRootComponent());
	}
}

bool AGroundActor::Contains(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots, const TArray<FGroundPoint>& OtherPoints)
{
	for (const FGroundPoint& Point : Points)
		if (Point.Road && Point.Index / 2 > RoadSlots[Point.Road].Num())
			return false;
	for (const FGroundPoint& Point : OtherPoints)
		if (!Points.Contains(Point))
			return false;
//	for (const FGroundPoint& Point : Points)
//		if (!OtherPoints.Contains(Point))
//			return false;
	return true;
}

ARoadScene* AGroundActor::GetScene()
{
	return Cast<ARoadScene>(GetAttachParentActor());
}

TArray<FVector> AGroundActor::GetVertices(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	TArray<FVector> Vertices;
	auto AddPoints = [&](const FPolyline& Curve)
	{
		for (int i = 0; i < Curve.Points.Num(); i++)
		{
			if (i == 0 && Vertices.Num() && Vertices.Last().Equals(Curve.Points[i].Pos))
				continue;
			Vertices.Add(Curve.Points[i].Pos);
		}
	};
	auto AddSpline = [&](const FVector& StartPos, const FVector& EndPos, const FVector& PrevDir, const FVector& NextDir)
	{
		FVector Dir = EndPos - StartPos;
		double Length = Dir.Size();
		Dir /= Length;
		FVector StartTangent = (PrevDir + Dir).GetSafeNormal();
		FVector EndTangent = (NextDir + Dir).GetSafeNormal();
		int NumSegments = FMath::Max(1, FMath::RoundToInt(Length / 1000));
		for (int i = 0; i <= NumSegments; i++)
		{
			int NumSegs = 1;
			if (i < NumSegments)
			{
				FVector StartT = FMath::CubicInterpDerivative(StartPos, StartTangent, EndPos, EndTangent, double(i) / NumSegments);
				FVector EndT = FMath::CubicInterpDerivative(StartPos, StartTangent, EndPos, EndTangent, double(i + 1) / NumSegments);
				double StartR = FMath::Atan2(StartT.Y, StartT.X);
				double EndR = FMath::Atan2(EndT.Y, EndT.X);
				NumSegs = FMath::Max(1, FMath::RoundToInt(16 * FMath::Abs(WrapRadian(EndR - StartR)) / DOUBLE_PI));
			}
			for (int j = 0; j < NumSegs; j++)
			{
				FVector Pos = FMath::CubicInterp(StartPos, StartTangent * Length, EndPos, EndTangent * Length, double(i) / NumSegments + double(j) / (NumSegments * NumSegs));
				if (Vertices.Num() && Vertices.Last().Equals(Pos))
					continue;
				Vertices.Add(Pos);
			}
		}
	};
	for (int i = 0; i < Points.Num() - !bClosedLoop; i++)
	{
		FGroundPoint& Start = Points[i];
		FGroundPoint& End = Points[(i + 1) % Points.Num()];
		if (Start.Road && End.Road)
		{
			TArray<FJunctionSlot>& Slots = RoadSlots[Start.Road];
			int Slot = Start.Index / 2;
			if (Start.Road == End.Road)
			{
				if (Start.Side == End.Side)
				{
					double S = Start.GetDist(RoadSlots);
					double E = End.GetDist(RoadSlots);
					URoadBoundary* RoadEdge = Start.Road->GetRoadBorder(Start.Side);
					if (!FMath::IsNearlyEqual(S, E))
						AddPoints(RoadEdge->CreatePolyline(S, E));
				}
				else
					Vertices.Add(End.GetPos(RoadSlots));
			}
			else if (Start.NextPoint(RoadSlots) == End)
			{
				if (Start.Side)
				{
					if (Slot > 0)
					{
						FJunctionGate& Gate = Slots[Slot - 1].OutputGate();
						if (ARoadActor* Road = Gate.Links[1].Road)
						{
							if (FMath::IsNearlyEqual(Gate.InitDist, Gate.Dist))
								Vertices.Add(End.GetPos(RoadSlots));
							else
								AddPoints(Road->GetRoadBorder(0)->Curve);
						}
					}
				}
				else
				{
					if (Slot < Slots.Num())
					{
						FJunctionGate& Gate = Slots[Slot].InputGate();
						if (ARoadActor* Road = Gate.Links[1].Road)
						{
							if (FMath::IsNearlyEqual(Gate.InitDist, Gate.Dist))
								Vertices.Add(End.GetPos(RoadSlots));
							else
								AddPoints(Road->GetRoadBorder(0)->Curve);
						}
					}
				}
			}
			else
			{
				AddSpline(Start.GetPos(RoadSlots), End.GetPos(RoadSlots), Start.GetDir(RoadSlots), End.GetDir(RoadSlots));
			}
		}
		else
		{
			FVector StartPos, EndPos, PrevDir, NextDir;
			if (Start.Road)
			{
				StartPos = Start.GetPos(RoadSlots);
				PrevDir = Start.GetDir(RoadSlots);
			}
			else
			{
				StartPos = ManualPoints[Start.Index];
				if (i > 0 || bClosedLoop)
				{
					FGroundPoint& Prev = Points[(i - 1 + Points.Num()) % Points.Num()];
					PrevDir = (StartPos - (Prev.Road ? Prev.GetPos(RoadSlots) : ManualPoints[Prev.Index])).GetSafeNormal();
				}
				else
					PrevDir = FVector::ZeroVector;
			}
			if (End.Road)
			{
				EndPos = End.GetPos(RoadSlots);
				NextDir = End.GetDir(RoadSlots);
			}
			else
			{
				EndPos = ManualPoints[End.Index];
				if (i < Points.Num() - 1 || bClosedLoop)
				{
					FGroundPoint& Next = Points[(i + 2) % Points.Num()];
					NextDir = ((Next.Road ? Next.GetPos(RoadSlots) : ManualPoints[Next.Index]) - EndPos).GetSafeNormal();
				}
				else
					NextDir = FVector::ZeroVector;
			}
			AddSpline(StartPos, EndPos, PrevDir, NextDir);
		}
	}
	if (bClosedLoop)
		Vertices.Pop();
	return MoveTemp(Vertices);
}

void AGroundActor::CreatePCGSpline()
{
	TMap<ARoadActor*, TArray<FJunctionSlot>> RoadSlots = GetScene()->GetAllJunctionSlots();
	TArray<FVector> Vertices = GetVertices(RoadSlots);
	AActor* Actor = GetWorld()->SpawnActor<AActor>();
	USceneComponent* RootComp = NewObject<USceneComponent>(Actor);
	Actor->SetRootComponent(RootComp);
	Actor->AddInstanceComponent(RootComp);
	USplineComponent* Spline = NewObject<USplineComponent>(Actor, TEXT("Spline"));
	Spline->SetupAttachment(RootComp);
	Actor->AddInstanceComponent(Spline);
	UPCGComponent* PCGComponent = NewObject<UPCGComponent>(Actor, TEXT("PCG Component"));
	Actor->AddInstanceComponent(PCGComponent);
	Actor->RegisterAllComponents();
	Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Spline->ClearSplinePoints(false);
	for (int i = 0; i < Vertices.Num(); i++)
		Spline->AddPoint(FSplinePoint(i, Vertices[i], ESplinePointType::Linear));
	Spline->SetClosedLoop(true, true);
	Spline->bSplineHasBeenEdited = true;
}
#if WITH_EDITOR
void AGroundActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AGroundActor, Material))
		{
			Cast<UStaticMeshComponent>(GetRootComponent())->SetMaterial(0, Material);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
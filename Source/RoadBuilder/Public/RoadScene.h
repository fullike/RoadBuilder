// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Math/GenericOctree.h"
#include "Settings.h"
#include "GroundActor.h"
#include "RoadScene.generated.h"

#define DefaultJunctionExtent	800.0

struct FRoadOctreeElement
{
	FRoadOctreeElement(URoadBoundary* B, int I) :Boundary(B), Index(I) {}
	FBox GetBounds() const
	{
		return Boundary->Curve.GetSegmentBounds(Index);
	}
	bool operator == (const FRoadOctreeElement& Other) const
	{
		return Boundary == Other.Boundary && Index == Other.Index;
	}
	bool IsBase() const { return Boundary->GetRoad()->BaseCurve == Boundary; }
	bool IsBoundary() const { return Boundary->GetRoad()->BaseCurve != Boundary; }
	bool Adjacent(const FRoadOctreeElement& Other) const
	{
		return Boundary == Other.Boundary && FMath::Abs(Index - Other.Index) <= 1;
	}
	bool Adjacent(URoadBoundary* OtherBoundary, int OtherIndex) const
	{
		return Boundary == OtherBoundary && FMath::Abs(Index - OtherIndex) <= 1;
	}
	int GetSide() { return !Boundary->LeftLane ? 1 : (!Boundary->RightLane ? 0 : -1); }
	URoadBoundary* Boundary;
	int Index;
};

struct FRoadOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FRoadOctreeElement& Element)
	{
		return FBoxCenterAndExtent(Element.GetBounds());
	}

	FORCEINLINE static bool AreElementsEqual(const FRoadOctreeElement& A, const FRoadOctreeElement& B)
	{
		return A == B;
	}
	static void SetElementId(const FRoadOctreeElement& Element, FOctreeElementId2 Id)
	{
		Element.Boundary->OctreeIds.SetNum(Element.Boundary->Curve.Points.Num() - 1);
		Element.Boundary->OctreeIds[Element.Index] = Id;
	}
};

USTRUCT()
struct FJunctionLink
{
	GENERATED_USTRUCT_BODY()
	void CreateRoad(AJunctionActor* Parent);
	void Destroy();
	UPROPERTY()
	ARoadActor* Road = nullptr;

	UPROPERTY()
	ARoadActor* InputRoad = nullptr;

	UPROPERTY()
	ARoadActor* OutputRoad = nullptr;

	UPROPERTY(EditAnywhere, Category = Link)
	double Radius = 1000;
};

USTRUCT()
struct FJunctionGate
{
	GENERATED_USTRUCT_BODY()
	bool operator < (const FJunctionGate& Other) const
	{
		return Radian > Other.Radian;
	}
	bool Contains(double D)
	{
		return (Sign < 0 && D >= Dist && D <= FMath::Max(CornerDists[0], CornerDists[1])) || (Sign > 0 && D <= Dist && D >= FMath::Min(CornerDists[0], CornerDists[1]));
	}
	void Clear()
	{
		for (FJunctionLink& Link : Links)
			Link.Destroy();
		Links.Empty();
	}
	bool IsRampOf(FJunctionGate& Other)
	{
#if 0
		FVector Dir = Road->GetDir(InitDist) * Sign;
		FVector NextDir = Other.Road->GetDir(Other.InitDist) * Other.Sign;
		if ((Dir|NextDir) < -0.996194698)
#else
		if (FMath::Abs(WrapRadian(Other.Radian - Radian)) > DOUBLE_HALF_PI)
#endif
		{
			int SrcIndex = Sign > 0 ? 0 : 1;
			if (Road->ConnectedParents[SrcIndex] == Other.Road)
			{
				int DstIndex = Other.Sign > 0 ? 0 : 1;
				if (Other.Road->ConnectedParents[DstIndex] == Road)
					return Road->Lanes.Num() < Other.Road->Lanes.Num();
				return true;
			}
		}
		return false;
	}
	bool IsConnected(FJunctionGate& Other)
	{
		int SrcIndex = Sign > 0 ? 0 : 1;
		int DstIndex = Other.Sign > 0 ? 0 : 1;
		return Road->ConnectedParents[SrcIndex] == Other.Road && Road == Other.Road->ConnectedParents[DstIndex];
	}
	FVector2D GetCross(int Side)
	{
		return Road->GetPos(CornerDists[Side]);
	}
	bool IsExpired() { return Radian > HALF_WORLD_MAX; }
	bool IsInput() { return Sign < 0; }
	bool IsOutput() { return Sign > 0; }
	void MarkExpired()
	{
		Dist = InitDist;
		Radian = WORLD_MAX;
	}
	void Renew(double D, double S);

	UPROPERTY()
	ARoadActor* Road;

	UPROPERTY()
	double Radian;

	UPROPERTY()
	double Sign;

	UPROPERTY()
	double InitDist;

	UPROPERTY()
	double Dist;

	UPROPERTY()
	double CornerDists[2];

	UPROPERTY()
	double CutDists[2];

	UPROPERTY()
	TArray<FJunctionLink> Links;
};

struct FJunctionSlot
{
	FJunctionGate& InputGate() const;
	FJunctionGate& OutputGate() const;
	int InputGateIndex() const;
	int OutputGateIndex() const;
	bool HasInput() const { return InputGateIndex() != INDEX_NONE; }
	bool HasOutput() const { return OutputGateIndex() != INDEX_NONE; }
	double InputDist() const;
	double OutputDist() const;
	double CrossDist() const;
	void Combine(FJunctionSlot& Other);
	bool IsValid() const
	{
		return Junction && Road;
	}
	bool operator < (const FJunctionSlot& Other) const
	{
		return CrossDist() < Other.CrossDist();
	}
	AJunctionActor* Junction;
	ARoadActor* Road;
	double InitInputDist = -MAX_dbl;
	double InitOutputDist = MAX_dbl;
};

UCLASS()
class ROADBUILDER_API AJunctionActor : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	static const int CornerIndex = 1;
	void AddRoad(ARoadActor* Road, double Dist);
	void Build();
	void BuildGoreMarkings();
	void BuildLink(FJunctionGate& Gate, FJunctionGate& Next, int Index);
	bool Contains(ARoadActor* Road, double Dist);
	void FixHeight(FPolyline& Polyline);
	void FixHeight(TArray<FVector>& Points);
	void Join(AJunctionActor* Junction);
	void Update(TOctree2<FRoadOctreeElement, FRoadOctreeSemantics>& Octree);
	void UpdateCorner(FJunctionGate& SrcGate, double SrcDist, FJunctionGate& DstGate, double DstDist);
	FJunctionGate& AddGate(ARoadActor* Road, double Dist, double Sign);
	int GetGate(const FVector& Pos);
	int GetRampConnection(FJunctionGate& Gate);
	FJunctionSlot GetSlot(ARoadActor* Road, double Dist);
	TArray<FJunctionSlot> GetSlots(ARoadActor* Road);
	ARoadScene* GetScene();
	void ExportXodr(FXmlNode* XmlNode, int& RoadId, int& ObjectId);
	virtual void Destroyed();
	
	UPROPERTY()
	TArray<FJunctionGate> Gates;
	TArray<FVector> DebugPoints;
	TArray<FPolyline> DebugCurves;
};

UCLASS()
class ROADBUILDER_API ARoadScene : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	ARoadActor* AddRoad();
	ARoadActor* AddRoad(URoadStyle* Style, double Height);
	ARoadActor* DuplicateRoad(ARoadActor* Source);
	ARoadActor* PickRoad(const FVector& Pos, ARoadActor* IgnoredRoad = nullptr);
	AGroundActor* AddGround(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots, const TArray<FGroundPoint>& Points);
	AJunctionActor* AddJunction(ARoadActor* R0, double D0, ARoadActor* R1, double D1);
	TMap<ARoadActor*, TArray<FJunctionSlot>> GetAllJunctionSlots();
	TArray<FJunctionSlot> GetJunctionSlots(ARoadActor* Road);
//	FVector2D GetRoadUV(ARoadActor* Road, const FVector& Pos);
	void Rebuild();
	void GenerateGrounds(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots);
	void OctreeAddBoundary(URoadBoundary* Boundary);
	void OctreeRemoveBoundary(URoadBoundary* Boundary);
	void OctreeAddRoad(ARoadActor* Road);
	void OctreeRemoveRoad(ARoadActor* Road);
	void DestroyRoad(ARoadActor* Road);
	virtual void PostLoad() override;
#if WITH_EDITOR
	void ExportXodr();
#endif

	UPROPERTY()
	TArray<ARoadActor*> Roads;

	UPROPERTY()
	TArray<AJunctionActor*> Junctions;

	UPROPERTY()
	TArray<AGroundActor*> Grounds;

	TOctree2<FRoadOctreeElement, FRoadOctreeSemantics> Octree;
};
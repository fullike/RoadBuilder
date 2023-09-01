// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GeoReferencingSystem.h"
#include "RoadCurve.h"
#include "OSMActor.generated.h"

class AOSMActor;
class ARoadActor;
class ARoadScene;

USTRUCT()
struct ROADBUILDER_API FOSMElement
{
public:
	GENERATED_USTRUCT_BODY()
	FOSMElement() {}
	FOSMElement(const FXmlNode* Node);
	/*
	FString GetStr(const TCHAR* Key)const
	{
		if (const FString* Tag = Tags.Find(Key))
			return *Tag;
		return "";
	}
	bool GetBool(const TCHAR* Key)const
	{
		const FString* Tag = Tags.Find(Key);
		return Tag && (*Tag == "yes");
	}

	void SetInt(const TCHAR* Key, int Value)
	{
		Tags.Add(Key, FString::Printf(TEXT("%d"), Value));
	}*/
	int GetInt(const TCHAR* Key)const
	{
		if (const FString* Tag = Tags.Find(Key))
			return FCString::Atoi(**Tag);
		return 0;
	}
	UPROPERTY(EditAnywhere, Category = Element)
	uint64 Id;

	UPROPERTY(EditAnywhere, Category = Element)
	TMap<FName, FString> Tags;
};

USTRUCT()
struct ROADBUILDER_API FOSMNode :public FOSMElement
{
public:
	GENERATED_USTRUCT_BODY()
	FOSMNode() {}
	FOSMNode(const FXmlNode* Node);
	void Build(AOSMActor* OSM);
	
	UPROPERTY(EditAnywhere, Category = Node)
	FVector2D Lonlat;
	FVector Pos;
	TArray<uint64> Ways;
};

USTRUCT()
struct ROADBUILDER_API FOSMWay :public FOSMElement
{
public:
	GENERATED_USTRUCT_BODY()
	FOSMWay() {}
	FOSMWay(const FXmlNode* Node);
	void AttachTo(FOSMWay* MainRoad, int NodeIndex, int RampIndex);
	void Build(AOSMActor* OSM);
	bool IsDrivable();
	double GetInputRadian(AOSMActor* OSM, int Index);
	double GetOutputRadian(AOSMActor* OSM, int Index);
	UPROPERTY(EditAnywhere, Category = Way)
	TArray<uint64> Nodes;
	FPolyline Curve;
	ARoadActor* Road = nullptr;
};

USTRUCT()
struct ROADBUILDER_API FOSMRelation :public FOSMElement
{
public:
	GENERATED_USTRUCT_BODY()
	FOSMRelation() {}
	FOSMRelation(const FXmlNode* Node);

	UPROPERTY(EditAnywhere, Category = Relation)
	TMap<uint64, FName> Nodes;

	UPROPERTY(EditAnywhere, Category = Relation)
	TMap<uint64, FName> Ways;

	UPROPERTY(EditAnywhere, Category = Relation)
	TMap<uint64, FName> Relations;
};

UCLASS()
class ROADBUILDER_API AOSMActor : public AGeoReferencingSystem
{
	GENERATED_UCLASS_BODY()
public:
	void AddNode(const FXmlNode* XmlNode);
	void AddWay(const FXmlNode* XmlNode);
	void AddRelation(const FXmlNode* XmlNode);
	void AnalyzeIntersection(uint64 NodeId, double R, TArray<FOSMWay*>& Inputs, TArray<FOSMWay*>& Outputs);
	void Build();
	void CreateRoad(uint64 Id);
	void CreateRoad(ARoadScene* Scene, uint64 Id);
	void LoadContent(const FString& Content);
	FBox2D GetTileBounds()
	{
		double Half = TileSize / 2;
		return FBox2D(FVector2D(Tile.X * TileSize - Half, Tile.Y * TileSize - Half), FVector2D(Tile.X * TileSize + Half, Tile.Y * TileSize + Half));
	}
#if WITH_EDITOR
	UFUNCTION(CallInEditor)
	void ConvertAll();

	UFUNCTION(CallInEditor)
	void LoadFile();

	UFUNCTION(CallInEditor)
	void LoadTile();
#endif
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, Category = OSM)
	FIntPoint Tile;

	UPROPERTY(EditAnywhere, Category = OSM)
	double TileSize = 51200;

	UPROPERTY(EditAnywhere, Category = OSM)
	TMap<uint64, FOSMNode> Nodes;

	UPROPERTY(EditAnywhere, Category = OSM)
	TMap<uint64, FOSMWay> Ways;

	UPROPERTY(EditAnywhere, Category = OSM)
	TMap<uint64, FOSMRelation> Relations;

	UPROPERTY(EditAnywhere, Category = OSM)
	TArray<uint64> DebugIds;
};

UCLASS()
class ROADBUILDER_API UOSMComponent : public USceneComponent
{
	GENERATED_BODY()
public:
};

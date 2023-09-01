// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "OSMActor.h"
#include "RoadScene.h"
#include "Settings.h"
#include "HttpModule.h"
#include "Engine/World.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "StructDialog.h"

FOSMElement::FOSMElement(const FXmlNode* Node)
{
	Id = FCString::Strtoui64(*Node->GetAttribute(TEXT("id")), nullptr, 16);
	for (const FXmlNode* Child = Node->GetFirstChildNode(); Child; Child = Child->GetNextNode())
		if (Child->GetTag() == TEXT("tag"))
			Tags.Add(*Child->GetAttribute(TEXT("k")), Child->GetAttribute(TEXT("v")));
}

FOSMNode::FOSMNode(const FXmlNode* Node) :FOSMElement(Node)
{
	Lonlat = FVector2D(FCString::Atod(*Node->GetAttribute(TEXT("lon"))), FCString::Atod(*Node->GetAttribute(TEXT("lat"))));
}

void FOSMNode::Build(AOSMActor* OSM)
{
	OSM->GeographicToEngine(FGeographicCoordinates(Lonlat.X, Lonlat.Y, 0), Pos);
}

FOSMWay::FOSMWay(const FXmlNode* Node) :FOSMElement(Node)
{
	for (const FXmlNode* Child = Node->GetFirstChildNode(); Child; Child = Child->GetNextNode())
		if (Child->GetTag() == TEXT("nd"))
			Nodes.Add(FCString::Strtoui64(*Child->GetAttribute(TEXT("ref")), nullptr, 16));
}

void FOSMWay::AttachTo(FOSMWay* MainRoad, int NodeIndex, int RampIndex)
{
	TArray<URoadLane*> Lanes = MainRoad->Road->GetLanes(0, { ELaneType::Driving, ELaneType::Shoulder });
	URoadBoundary* RampBoundary = RampIndex == 0 ? Lanes[RampIndex]->LeftBoundary : Lanes[RampIndex - 1]->RightBoundary;
	FVector2D UV = MainRoad->Road->GetUV(Road->RoadPoints[NodeIndex ? Road->RoadPoints.Num() - 1 : 0].Pos);
	UV.Y = RampBoundary->GetOffset(UV.X);
	Road->AddDirPoint(NodeIndex);
	Road->ConnectTo(MainRoad->Road, UV, NodeIndex);
	FVector Dir = RampBoundary->GetDir(UV.X);
	FPlane Plane(RampBoundary->GetPos(UV.X), FVector(-Dir.Y, Dir.X, Dir.Z));
	for (int i = 2; i < Road->RoadPoints.Num(); i++)
	{
		FRoadPoint& Point = Road->RoadPoints[NodeIndex ? Road->RoadPoints.Num() - 1 - i : i];
		FVector Pos(Point.Pos, 0);
		double Dot = Plane.PlaneDot(Pos);
		if (RampIndex && Dot > 0 || !RampIndex && Dot < 0)
			break;
		Point.Pos = FVector2D(Pos - Dot * Plane);
	}
}

void FOSMWay::Build(AOSMActor* OSM)
{
	double Dist = 0;
	int k = 0;
	for (int i = 1; i < Nodes.Num(); i++)
	{
		FOSMNode* Prev = OSM->Nodes.Find(Nodes[i - 1]);
		FOSMNode* Node = OSM->Nodes.Find(Nodes[i]);
		Dist += FVector2D::Distance((FVector2D&)Prev->Pos, (FVector2D&)Node->Pos);
		if (Node->Ways.Num() > 1 || i == Nodes.Num() - 1)
		{
			double D = 0;
			FOSMNode* Start = OSM->Nodes.Find(Nodes[k]);
			FOSMNode* End = OSM->Nodes.Find(Nodes[i]);
			for (int j = k + 1; j < i; j++)
			{
				Prev = OSM->Nodes.Find(Nodes[j - 1]);
				Node = OSM->Nodes.Find(Nodes[j]);
				D += FVector2D::Distance((FVector2D&)Prev->Pos, (FVector2D&)Node->Pos);
				Node->Pos.Z = FMath::Lerp(Start->Pos.Z, End->Pos.Z, D / Dist);
			}
			k = i;
			Dist = 0;
		}
	}


	Curve = FPolyline();
	for (uint64 NodeId : Nodes)
	{
		if (FOSMNode* Node = OSM->Nodes.Find(NodeId))
		{
			Curve.AddPoint(Node->Pos, 0);
		}
	}
}

bool FOSMWay::IsDrivable()
{
	static TArray<FString> Highways = { TEXT("motorway"), TEXT("trunk"), TEXT("primary"), TEXT("secondary"), TEXT("tertiary") };
	if (FString* Tag = Tags.Find(TEXT("highway")))
	{
		for (FString& Highway : Highways)
			if (Highway == *Tag || Highway + TEXT("_link") == *Tag)
				return true;
	}
	return false;
}

double FOSMWay::GetInputRadian(AOSMActor* OSM, int Index)
{
	int Prev = FMath::Max(0, Index - 1);
	Index = Prev + 1;
	FVector2D Dir = FVector2D(OSM->Nodes[Nodes[Index]].Pos) - FVector2D(OSM->Nodes[Nodes[Prev]].Pos);
	return FMath::Atan2(Dir.Y, Dir.X);
}

double FOSMWay::GetOutputRadian(AOSMActor* OSM, int Index)
{
	int Next = FMath::Min(Nodes.Num() - 1, Index + 1);
	Index = Next - 1;
	FVector2D Dir = FVector2D(OSM->Nodes[Nodes[Next]].Pos) - FVector2D(OSM->Nodes[Nodes[Index]].Pos);
	return FMath::Atan2(Dir.Y, Dir.X);
}

FOSMRelation::FOSMRelation(const FXmlNode* Node) :FOSMElement(Node)
{
	for (const FXmlNode* Child = Node->GetFirstChildNode(); Child; Child = Child->GetNextNode())
	{
		if (Child->GetTag() == TEXT("member"))
		{
			FString Type = Child->GetAttribute(TEXT("type"));
			if (Type == TEXT("node"))
				Nodes.Add(FCString::Strtoui64(*Child->GetAttribute(TEXT("ref")), nullptr, 16), *Child->GetAttribute(TEXT("role")));
			else if (Type == TEXT("way"))
				Ways.Add(FCString::Strtoui64(*Child->GetAttribute(TEXT("ref")), nullptr, 16), *Child->GetAttribute(TEXT("role")));
			else if (Type == TEXT("relation"))
				Relations.Add(FCString::Strtoui64(*Child->GetAttribute(TEXT("ref")), nullptr, 16), *Child->GetAttribute(TEXT("role")));
		}
	}
}

AOSMActor::AOSMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<UOSMComponent>(TEXT("RootComponent"));
	bOriginLocationInProjectedCRS = false;
	OriginLatitude = 31.2215;
	OriginLongitude = 121.3715;
	ProjectedCRS = TEXT("EPSG:32651");
}

void AOSMActor::AddNode(const FXmlNode* XmlNode)
{
	FOSMNode Node(XmlNode);
	Nodes.Add(Node.Id, MoveTemp(Node));
}

void AOSMActor::AddWay(const FXmlNode* XmlNode)
{
	FOSMWay Way(XmlNode);
	Ways.Add(Way.Id, MoveTemp(Way));
}

void AOSMActor::AddRelation(const FXmlNode* XmlNode)
{
	FOSMRelation Relation(XmlNode);
	Relations.Add(Relation.Id, MoveTemp(Relation));
}

void AOSMActor::AnalyzeIntersection(uint64 NodeId, double R, TArray<FOSMWay*>& Inputs, TArray<FOSMWay*>& Outputs)
{
	FOSMNode& Node = Nodes[NodeId];
	TMap<FOSMWay*, double> InputDiffs;
	TMap<FOSMWay*, double> OutputDiffs;
	double MaxRadians = FMath::DegreesToRadians(30);
	for (uint64& WayId : Node.Ways)
	{
		FOSMWay* Way = &Ways[WayId];
		int Index = Way->Nodes.Find(NodeId);
		if (Index < Way->Nodes.Num() - 1)
		{
			double RR = Way->GetOutputRadian(this, Index);
			double Diff = WrapRadian(RR - R);
			if (FMath::Abs(Diff) < MaxRadians)
			{
				OutputDiffs.Add(Way, Diff);
				Outputs.Add(Way);
			}
		}
		if (Index > 0)
		{
			double RR = Way->GetInputRadian(this, Index);
			double Diff = WrapRadian(RR - R);
			if (FMath::Abs(Diff) < MaxRadians)
			{
				InputDiffs.Add(Way, Diff);
				Inputs.Add(Way);
			}
		}
	}
	Inputs.Sort([&](FOSMWay& A, FOSMWay& B)
	{
		return InputDiffs[&A] > InputDiffs[&B];
	});
	Outputs.Sort([&](FOSMWay& A, FOSMWay& B)
	{
		return OutputDiffs[&A] < OutputDiffs[&B];
	});
}

void AOSMActor::Build()
{
	for (TPair<uint64, FOSMWay>& KV : Ways)
		for (uint64& Node : KV.Value.Nodes)
			Nodes[Node].Ways.AddUnique(KV.Key);
	USettings_OSM* Settings = GetMutableDefault<USettings_OSM>();
	for (TPair<uint64, FOSMNode>& KV : Nodes)
	{
		KV.Value.Build(this);
		for (uint64& Way : KV.Value.Ways)
			KV.Value.Pos.Z += Ways[Way].GetInt(TEXT("layer")) * Settings->LayerHeight / KV.Value.Ways.Num();
	}
	for (TPair<uint64, FOSMWay>& KV : Ways)
		KV.Value.Build(this);
}

void AOSMActor::CreateRoad(uint64 Id)
{
	ARoadScene* Scene = Cast<ARoadScene>(UGameplayStatics::GetActorOfClass(GetWorld(), ARoadScene::StaticClass()));
	if (!Scene)
		Scene = GetWorld()->SpawnActor<ARoadScene>();
	CreateRoad(Scene, Id);
	Scene->Rebuild();
}

void AOSMActor::CreateRoad(ARoadScene* Scene, uint64 Id)
{
	int MaxRamps = 0;
	FOSMWay& Way = Ways[Id];
	if (DebugIds.Contains(Way.Id))
	{
		int k = 0;
	}
	for (int i = 0; i < Way.Nodes.Num(); i++)
	{
		FOSMNode& Node = Nodes[Way.Nodes[i]];
		if (Node.Ways.Num() > 1)
		{
			if (i > 0)
			{
				TArray<FOSMWay*> Inputs, Outputs;
				AnalyzeIntersection(Way.Nodes[i], Way.GetInputRadian(this, i), Inputs, Outputs);
				MaxRamps = FMath::Max(MaxRamps, Outputs.Num());
			}
			if (i < Way.Nodes.Num() - 1)
			{
				TArray<FOSMWay*> Inputs, Outputs;
				AnalyzeIntersection(Way.Nodes[i], Way.GetOutputRadian(this, i), Inputs, Outputs);
				MaxRamps = FMath::Max(MaxRamps, Inputs.Num());
			}
		}
	}
	USettings_OSM* Settings = GetMutableDefault<USettings_OSM>();
	Way.Road = Scene->AddRoad(Settings->RoadStyle.LoadSynchronous(), 0);
	TArray<URoadLane*> Lanes = Way.Road->GetLanes(0, { ELaneType::Driving });
	for (int i = Lanes.Num(); i < MaxRamps; i++)
		Way.Road->CopyLane(Lanes[0], 0);
	FPolyline& Curve = Way.Curve;
	FPolyline Resample = Curve.Resample(Settings->Resample);
	double Dist = 0;
	static double Tolerance = 1e-6;
	FRoadSegment Segment = { Dist, 0, FVector2D(Resample.Points[0].Pos), Resample.GetStraightRadian(0) };
	FHeightSegment Height = { Dist, Resample.Points[0].Pos.Z, 0};
	for (int i = 1; i < Resample.Points.Num(); i++)
	{
		FPolyPoint& Prev = Resample.Points[i - 1];
		FPolyPoint& This = Resample.Points[i];
		double Prev_R = Resample.GetStraightRadian(i - 1);
		double This_R = Resample.GetStraightRadian(i);
		double Diff_R = This_R - Prev_R;
		double Diff_D = This.Dist - Prev.Dist;
		double C = Diff_R / Diff_D;
		Segment.Length += Diff_D;
		Dist += Diff_D;
		double Diff_Total = This_R - Segment.StartRadian;
		double C_Avg = Diff_Total / Segment.Length;
		if (!FMath::IsNearlyEqual(C, C_Avg, Tolerance) || i == Resample.Points.Num() - 1)
		{
			Segment.StartCurv = Segment.EndCurv = C_Avg;
			Way.Road->RoadSegments.Add(MoveTemp(Segment));
			Way.Road->HeightSegments.Add(MoveTemp(Height));
			Segment = { Dist, 0, FVector2D(Resample.Points[i].Pos), Resample.GetStraightRadian(i) };
			Height = { Dist, Resample.Points[i].Pos.Z, 0 };
		}
	}
	//TODO: Add Length to HeightSegment???
	Way.Road->HeightSegments.Add(MoveTemp(Height));
	Way.Road->RoadPoints = Way.Road->CalcRoadPoints(0, Dist);
	Way.Road->HeightPoints = Way.Road->CalcHeightPoints(0, Dist);
//	double LayerHeight = Settings->LayerHeight * Way.GetInt(TEXT("layer"));
//	double StartHeight = Nodes[Way.Nodes[0]].Pos.Z;
//	double EndHeight = Nodes[Way.Nodes.Last()].Pos.Z;
//	Way.Road->HeightPoints[0].Height = StartHeight;
//	Way.Road->HeightPoints.Last().Height = EndHeight;
//	if (!FMath::IsNearlyEqual((StartHeight + EndHeight) / 2, LayerHeight))
//		Road->HeightPoints[Road->AddHeight(Road->Length() / 2)].Height = LayerHeight;
	Way.Road->UpdateCurve();
}

void AOSMActor::LoadContent(const FString& Content)
{
	FFileHelper::SaveStringToFile(Content, *(FPaths::ProjectSavedDir() / TEXT("1.osm")));

	FXmlFile XmlFile(Content, EConstructMethod::ConstructFromBuffer);
	FXmlNode* RootNode = XmlFile.GetRootNode();
	for (const FXmlNode* Child = RootNode->GetFirstChildNode(); Child; Child = Child->GetNextNode())
	{
		if (Child->GetTag() == TEXT("node"))
			AddNode(Child);
		else if (Child->GetTag() == TEXT("way"))
			AddWay(Child);
		else if (Child->GetTag() == TEXT("relation"))
			AddRelation(Child);
	}
	Build();
}
#if WITH_EDITOR
void AOSMActor::ConvertAll()
{
	USettings_OSM* Settings = GetMutableDefault<USettings_OSM>();
	if (IsDataDialogProceed(Settings))
	{
		ARoadScene* Scene = Cast<ARoadScene>(UGameplayStatics::GetActorOfClass(GetWorld(), ARoadScene::StaticClass()));
		if (!Scene)
			Scene = GetWorld()->SpawnActor<ARoadScene>();
		for (TPair<uint64, FOSMWay>& KV : Ways)
			if (KV.Value.IsDrivable())
				CreateRoad(Scene, KV.Key);
		for (TPair<uint64, FOSMWay>& KV : Ways)
		{
			FOSMWay* Way = &KV.Value;
			if (!Settings->ConnectRoads)
				continue;
			if (Way->IsDrivable())
			{
				if (DebugIds.Contains(Way->Id))
				{
					int k = 0;
				}
				TArray<ELaneType> DrivingLaneTypes = { ELaneType::Driving, ELaneType::Shoulder };
				if (Way->Road->ConnectedParents[0] == nullptr)
				{
					TArray<FOSMWay*> Inputs, Outputs;
					AnalyzeIntersection(Way->Nodes[0], Way->GetOutputRadian(this, 0), Inputs, Outputs);
					if (Inputs.Num())
					{
						FOSMWay* MainRoad = Inputs[0];
						if (Way == Outputs[0] && MainRoad->Nodes.Last() == Way->Nodes[0] && MainRoad->Road->ConnectedParents[1] == nullptr)
						{
							MainRoad->Road->AddDirPoint(1);
							MainRoad->Road->ConnectTo(Way->Road, FVector2D(0, 0), 1);
							Way->Road->AddDirPoint(0);
							Way->Road->ConnectTo(MainRoad->Road, FVector2D(MainRoad->Road->Length(), 0), 0);
						}
						else
						{
							int RampIndex = Outputs.Find(Way);
							Way->AttachTo(MainRoad, 0, RampIndex);
							/*
							TArray<URoadLane*> Lanes = MainRoad->Road->GetLanes(0, DrivingLaneTypes);
							FVector2D UV = MainRoad->Road->GetUV(Way->Curve.Points[0].Pos);
							URoadBoundary* RampBoundary = RampIndex == 0 ? Lanes[RampIndex]->LeftBoundary : Lanes[RampIndex - 1]->RightBoundary;
							UV.Y = RampBoundary->GetOffset(UV.X);
							Way->Road->AddDirPoint(0);
							Way->Road->ConnectTo(MainRoad->Road, UV, 0);*/
						}
						Way->Road->UpdateCurve();
					}
				}
				if (Way->Road->ConnectedParents[1] == nullptr)
				{
					TArray<FOSMWay*> Inputs, Outputs;
					AnalyzeIntersection(Way->Nodes.Last(), Way->GetInputRadian(this, Way->Nodes.Num() - 1), Inputs, Outputs);
					if (Outputs.Num())
					{
						FOSMWay* MainRoad = Outputs[0];
						if (Way == Inputs[0] && MainRoad->Nodes[0] == Way->Nodes.Last() && MainRoad->Road->ConnectedParents[0] == nullptr)
						{
							Way->Road->AddDirPoint(1);
							Way->Road->ConnectTo(MainRoad->Road, FVector2D(0, 0), 1);
							MainRoad->Road->AddDirPoint(0);
							MainRoad->Road->ConnectTo(Way->Road, FVector2D(Way->Road->Length(), 0), 0);
						}
						else
						{
							int RampIndex = Inputs.Find(Way);
							Way->AttachTo(MainRoad, 1, RampIndex);
							/*
							TArray<URoadLane*> Lanes = MainRoad->Road->GetLanes(0, DrivingLaneTypes);
							FVector2D UV = MainRoad->Road->GetUV(Way->Curve.Points.Last().Pos);
							URoadBoundary* RampBoundary = RampIndex == 0 ? Lanes[RampIndex]->LeftBoundary : Lanes[RampIndex - 1]->RightBoundary;
							UV.Y = RampBoundary->GetOffset(UV.X);
							Way->Road->AddDirPoint(1);
							Way->Road->ConnectTo(MainRoad->Road, UV, 1);*/
						}
						Way->Road->UpdateCurve();
					}
				}
			}
		}
		Scene->Rebuild();
	}
}
#include "DesktopPlatformModule.h"
void AOSMActor::LoadFile()
{
	TArray<FString> Files;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		if (DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Import OSM"),
			TEXT(""),
			TEXT(""),
			TEXT("OSM File (*.osm)|*.osm"),
			EFileDialogFlags::None,
			Files))
		{
			FString Content;
			if (FFileHelper::LoadFileToString(Content, *Files[0]))
			{
				LoadContent(Content);
			}
		}
	}
}

void AOSMActor::LoadTile()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		if (bSucceeded)
		{
			FString Content = HttpResponse->GetContentAsString();
			LoadContent(Content);
		}
	});
	FBox2D Bounds = GetTileBounds();
	FGeographicCoordinates MinLonlat, MaxLonlat;
	EngineToGeographic(FVector(Bounds.Min, 0), MinLonlat);
	EngineToGeographic(FVector(Bounds.Max, 0), MaxLonlat);
//	url = '{left},{bottom},{right},{top}'.format(left = minlon, bottom = maxlat, right = maxlon, top = minlat)
	FString Url = FString::Printf(TEXT("https://api.openstreetmap.org/api/0.6/map?bbox=%lf,%lf,%lf,%lf"), MinLonlat.Longitude, MaxLonlat.Latitude, MaxLonlat.Longitude, MinLonlat.Latitude);
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("GET"));
//	HttpRequest->SetHeader(TEXT("Accept"), TEXT("text/html, application/xhtml+xml, */*"));
	HttpRequest->ProcessRequest();
}
#endif
void AOSMActor::PostLoad()
{
	Super::PostLoad();
	Build();
}
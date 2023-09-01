// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadActor.h"
#include "RoadScene.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"

URoadStyle* URoadStyle::Create(URoadBoundary* SrcBoundary, int SrcSide, URoadBoundary* DstBoundary, int DstSide, bool SkipSidewalks, bool KeepLeftLanes, uint32 LeftLaneMarkingMask, uint32 RightLaneMarkingMask)
{
	URoadStyle* Style = NewObject<URoadStyle>();
	ARoadActor* SrcRoad = SrcBoundary->GetRoad();
	ARoadActor* DstRoad = DstBoundary->GetRoad();
	URoadBoundary* SrcB = SrcBoundary;
	URoadBoundary* DstB = DstBoundary;
	Style->bHasGround = SrcRoad->bHasGround && DstRoad->bHasGround;
	bool SrcMainRoad = DstRoad->Lanes.Num() < SrcRoad->Lanes.Num();
	if (RightLaneMarkingMask & 1)
	{
		FBoundarySegment& Seg = SrcMainRoad ? DstB->Segments[0] : SrcB->Segments.Last();
		Style->BaseCurveMark = Seg.LaneMarking;
		Style->BaseCurveProps = Seg.Props;
	}
	while (true)
	{
		FRoadLaneStyle LaneStyle;
		URoadLane* SrcL = SrcB->GetLane(SrcSide);
		URoadLane* DstL = DstB->GetLane(DstSide);
		if (!SrcL || !DstL)
			break;
		if (SrcL->Segments.Last().LaneType != DstL->Segments[0].LaneType)
			break;
		SrcB = SrcL->GetBoundary(SrcSide);
		DstB = DstL->GetBoundary(DstSide);
		LaneStyle.LaneType = DstL->Segments[0].LaneType;
		if (SkipSidewalks && LaneStyle.LaneType == ELaneType::Sidewalk)
			break;
		LaneStyle.Width = DstB->LocalOffsets[0].Offset;
		LaneStyle.LaneShape = DstL->Segments[0].LaneShape;
		if ((1 << Style->RightLanes.Num()) & RightLaneMarkingMask)
		{
			FBoundarySegment& Seg = SrcMainRoad ? DstB->Segments[0] : SrcB->Segments.Last();
			LaneStyle.LaneMarking = Seg.LaneMarking;
			LaneStyle.Props = Seg.Props;
		}
		Style->RightLanes.Add(LaneStyle);
	}
	if (KeepLeftLanes)
	{
		SrcB = SrcBoundary;
		DstB = DstBoundary;
		if (LeftLaneMarkingMask & 1)
		{
			FBoundarySegment& Seg = SrcMainRoad ? DstB->Segments[0] : SrcB->Segments.Last();
			Style->BaseCurveMark = Seg.LaneMarking;
			Style->BaseCurveProps = Seg.Props;
		}
		while (SrcB != SrcRoad->BaseCurve)
		{
			FRoadLaneStyle LaneStyle;
			URoadLane* SrcL = SrcB->GetLane(!SrcSide);
			URoadLane* DstL = DstB->GetLane(!DstSide);
			if (!SrcL || !DstL)
				break;
			if (SrcL->Segments.Last().LaneType != DstL->Segments[0].LaneType)
				break;
			LaneStyle.LaneType = DstL->Segments[0].LaneType;
		//	Sidewalk never comes from left side
		//	if (SkipSidewalks && LaneStyle.LaneType == ELaneType::Sidewalk)
		//		break;
			LaneStyle.Width = DstB->LocalOffsets[0].Offset;
			LaneStyle.LaneShape = DstL->Segments[0].LaneShape;
			SrcB = SrcL->GetBoundary(!SrcSide);
			DstB = DstL->GetBoundary(!DstSide);
			if ((1 << Style->LeftLanes.Num()) & LeftLaneMarkingMask)
			{
				FBoundarySegment& Seg = SrcMainRoad ? DstB->Segments[0] : SrcB->Segments.Last();
				LaneStyle.LaneMarking = Seg.LaneMarking;
				LaneStyle.Props = Seg.Props;
			}
			Style->LeftLanes.Add(LaneStyle);
		}
	}
	return Style;
}

void FRoadSegment::ExportXodr(FXmlNode* PlanViewNode)
{
	FXmlNode* GeometryNode = XmlNode_CreateChild(PlanViewNode, TEXT("geometry"));
	XmlNode_AddAttribute(GeometryNode, TEXT("s"), Dist * 0.01);
	XmlNode_AddAttribute(GeometryNode, TEXT("x"), StartPos.X * 0.01);
	XmlNode_AddAttribute(GeometryNode, TEXT("y"),-StartPos.Y * 0.01);
	XmlNode_AddAttribute(GeometryNode, TEXT("hdg"),-StartRadian);
	XmlNode_AddAttribute(GeometryNode, TEXT("length"), Length * 0.01);
	if (FMath::IsNearlyEqual(StartCurv, EndCurv))
	{
		if (FMath::IsNearlyZero(StartCurv))
			XmlNode_CreateChild(GeometryNode, TEXT("line"));
		else
		{
			FXmlNode* Node = XmlNode_CreateChild(GeometryNode, TEXT("arc"));
			XmlNode_AddAttribute(Node, TEXT("curvature"), -StartCurv * 100);
		}
	}
	else
	{
		FXmlNode* Node = XmlNode_CreateChild(GeometryNode, TEXT("spiral"));
		XmlNode_AddAttribute(Node, TEXT("curvStart"), -StartCurv * 100);
		XmlNode_AddAttribute(Node, TEXT("curvEnd"), -EndCurv * 100);
	}
}

FRoadSegment FRoadSegment::Reverse()
{
	FRoadSegment S;
	S.Dist = Dist;
	S.Length = Length;
	S.StartPos = GetPos(Length);
	S.StartRadian = WrapRadian(GetR(Length) + DOUBLE_PI);
	S.StartCurv = -EndCurv;
	S.EndCurv = -StartCurv;
	return MoveTemp(S);
}

FRoadSegment FRoadSegment::ApplyOffset(double Offset)
{
	FRoadSegment S;
	S.Dist = Dist;
	FVector2D Dir(-FMath::Sin(StartRadian), FMath::Cos(StartRadian));
	S.StartPos = StartPos + Dir * Offset;
	S.StartRadian = StartRadian;
	S.StartCurv = StartCurv != 0 ? 1.0 / (1.0 / StartCurv - Offset) : 0;
	S.EndCurv = EndCurv != 0 ? 1.0 / (1.0 / EndCurv - Offset) : 0;
	if (S.StartCurv == 0 && S.EndCurv == 0)
		S.Length = Length;
	else
		S.Length = ComputeSpiralLength(StartRadian, GetR(Length), S.StartCurv, S.EndCurv);
	return MoveTemp(S);
}

void FHeightSegment::ExportXodr(FXmlNode* ElevationProfileNode)
{
	double EndHeight = (this + 1)->Height;
	double EndDir = (this + 1)->Dir;
	double Length = (this + 1)->Dist - Dist;
	FVector4 ABCD = CalcABCD(Height * 0.01, Dir * 0.01, EndHeight * 0.01, EndDir * 0.01, Length * 0.01);
	FXmlNode* ElevationNode = XmlNode_CreateChild(ElevationProfileNode, TEXT("elevation"));
	XmlNode_AddAttribute(ElevationNode, TEXT("s"), Dist * 0.01);
	XmlNode_AddABCD(ElevationNode, ABCD);
}

FHeightSegment FHeightSegment::Reverse()
{
	FHeightSegment S;
	S.Dist = Dist;
	S.Height = Height;
	S.Dir = -Dir;
	return MoveTemp(S);
}

FVector2D FConnectInfo::GetPos()
{
	int PointIndex = Index == 0 ? 0 : Child->RoadPoints.Num() - 1;
	return Child->RoadPoints[PointIndex].Pos;
}

void FConnectInfo::UpdateChild(ARoadActor* Parent)
{
	int PointIndex = Index == 0 ? 0 : Child->RoadPoints.Num() - 1;
	int DirPoint = Index == 0 ? 1 : Child->RoadPoints.Num() - 2;
	int HeightIndex = Index == 0 ? 0 : Child->HeightPoints.Num() - 1;
	int DirHeight = Index == 0 ? 1 : Child->HeightPoints.Num() - 2;
//	double Dist = SrcDist * SrcRoad->Length();
	FVector2D Pos(Parent->GetPos(UV.X));
	FVector2D Dir(Parent->GetDir(UV.X));
	FVector2D DstDir(Child->GetDir(Index == 0 ? 0 : Child->Length()));
	double Sign = ((Index == 0) ^ ((Dir|DstDir) > 0)) ? -1 : 1;
	Pos += FVector2D(-Dir.Y, Dir.X) * UV.Y;
	FRoadPoint& Point = Child->RoadPoints[PointIndex];
	FRoadPoint& DirPt = Child->RoadPoints[DirPoint];
	double Length = FVector2D::Distance(Point.Pos, DirPt.Pos) * Sign;
	Point.Pos = Pos;
	DirPt.Pos = Pos + Dir * Length;
	Child->HeightPoints[HeightIndex].Height = Parent->GetHeight(UV.X);
	Child->HeightPoints[DirHeight].Height = Parent->GetHeight(UV.X + Length);
}

double FConnectInfo::ConnectionSign(ARoadActor* Parent)
{
	return FMath::Sign(Parent->GetDir2D(UV.X) | Child->GetDir2D(Index == 0 ? 0 : Child->Length()));
}

ARoadActor::ARoadActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if USE_PROC_ROAD_MESH
	RootComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RootComponent"));
#else
	RootComponent = CreateDefaultSubobject<URoadMeshComponent>(TEXT("RootComponent"));
#endif
}

URoadBoundary* ARoadActor::AddBoundary(double Offset, ULaneMarkStyle* LaneMarking, URoadProps* Props)
{
	URoadBoundary* Boundary = NewObject<URoadBoundary>(this, NAME_None, RF_Transactional);
	Boundary->Segments.Add({ 0, LaneMarking, Props });
	Boundary->LocalOffsets.Add({ 0, Offset });
	Boundary->LocalOffsets.Add({ 0, Offset });
	Boundaries.Add(Boundary);
	return Boundary;
}

URoadBoundary* ARoadActor::GetRoadEdge(int Side)
{
	TArray<URoadLane*> Sorts = GetLanes(Side);
	for (int i = Sorts.Num() - 1; i >= 0; i--)
		if (Sorts[i]->GetSegment(ELaneType::Sidewalk) == INDEX_NONE)
			return Sorts[i]->GetBoundary(Side);
	return BaseCurve;
}

URoadBoundary* ARoadActor::GetRoadBorder(int Side)
{
	URoadBoundary* Boundary = BaseCurve;
	while (URoadLane* Lane = Boundary->GetLane(Side))
		Boundary = Lane->GetBoundary(Side);
	return Boundary;
}

URoadBoundary* ARoadActor::GetBoundary(const FVector2D& UV)
{
	URoadLane* Lane = GetLane(UV);
	double LeftDist = FMath::Abs(Lane->LeftBoundary->GetOffset(UV.X) - UV.Y);
	double RightDist = FMath::Abs(Lane->RightBoundary->GetOffset(UV.X) - UV.Y);
	return LeftDist < RightDist ? Lane->LeftBoundary : Lane->RightBoundary;
}

TArray<URoadBoundary*> ARoadActor::GetBoundaries(const FVector2D& UV)
{
	TArray<URoadBoundary*> Results;
	URoadBoundary* B = BaseCurve;
	while (B->LeftLane)
	{
		B = B->LeftLane->LeftBoundary;
		Results.Insert(B, 0);
	}
	B = BaseCurve;
	Results.Add(B);
	while (B->RightLane)
	{
		B = B->RightLane->RightBoundary;
		Results.Add(B);
	}
	for (int i = 0; i < Results.Num();)
	{
		if (!FMath::IsNearlyEqual(Results[i]->GetOffset(UV.X), UV.Y))
			Results.RemoveAt(i);
		else
			i++;
	}
	return MoveTemp(Results);
}

TArray<URoadBoundary*> ARoadActor::GetBoundaries(URoadBoundary* Boundary, int Side, const TArray<ELaneMarkType>& Types)
{
	TArray<URoadBoundary*> OutBoundaries;
	while (true)
	{
		URoadLane* Lane = Boundary->GetLane(Side);
		if (Types.Num())
		{
			for (ELaneMarkType Type : Types)
			{
				if (Boundary->GetSegment(Type) != INDEX_NONE)
				{
					OutBoundaries.Add(Boundary);
					break;
				}
			}
		}
		else
			OutBoundaries.Add(Boundary);
		if (!Lane)
			break;
		Boundary = Lane->GetBoundary(Side);
		if (Boundary == BaseCurve)
			break;
	}
	return MoveTemp(OutBoundaries);
}

URoadLane* ARoadActor::AddLane(URoadBoundary* LeftBoundary, URoadBoundary* RightBoundary, const FRoadLaneStyle& LaneStyle)
{
	URoadLane* Lane = NewObject<URoadLane>(this, NAME_None, RF_Transactional);
	Lane->Segments.Add({ 0, LaneStyle.LaneShape, LaneStyle.LaneType });
	Lanes.Add(Lane);
	if (LeftBoundary && !LeftBoundary->RightLane)
	{
		Lane->LeftBoundary = LeftBoundary;
		LeftBoundary->RightLane = Lane;
	}
	if (RightBoundary && !RightBoundary->LeftLane)
	{
		Lane->RightBoundary = RightBoundary;
		RightBoundary->LeftLane = Lane;
	}
	if (!Lane->LeftBoundary)
	{
		Lane->LeftBoundary = AddBoundary(LaneStyle.Width, LaneStyle.LaneMarking, LaneStyle.Props);
		Lane->LeftBoundary->RightLane = Lane;
	}
	if (!Lane->RightBoundary)
	{
		Lane->RightBoundary = AddBoundary(LaneStyle.Width, LaneStyle.LaneMarking, LaneStyle.Props);
		Lane->RightBoundary->LeftLane = Lane;
	}
	return Lane;
}

URoadLane* ARoadActor::CopyLane(URoadLane* SrcL, int TargetSide)
{
	Modify();
	URoadLane* Lane = NewObject<URoadLane>(this, NAME_None, RF_Transactional);
	Lane->Segments = SrcL->Segments;
	Lanes.Add(Lane);
	int SrcSide = SrcL->GetSide();
	URoadBoundary* SrcB = SrcL->GetBoundary(SrcSide);
	URoadBoundary* Boundary = NewObject<URoadBoundary>(this, NAME_None, RF_Transactional);
	Boundary->Segments = SrcB->Segments;
	Boundary->LocalOffsets = SrcB->LocalOffsets;
	Boundary->Offsets = SrcB->Offsets;
	Boundaries.Add(Boundary);
	URoadBoundary* TargetB = SrcL->GetBoundary(TargetSide);
	Lane->GetBoundary(!SrcSide) = TargetB;
	Lane->GetBoundary(SrcSide) = Boundary;
	Boundary->GetLane(!SrcSide) = Lane;
	if (SrcSide == TargetSide)
	{
		if (URoadLane* Neighbor = TargetB->GetLane(TargetSide))
		{
			Boundary->GetLane(SrcSide) = Neighbor;
			Neighbor->Modify();
			Neighbor->GetBoundary(!SrcSide) = Boundary;
		}
	}
	else
	{
		Boundary->GetLane(SrcSide) = SrcL;
		SrcL->Modify();
		SrcL->GetBoundary(!SrcSide) = Boundary;
	}
	TargetB->Modify();
	TargetB->GetLane(SrcSide) = Lane;
	return Lane;
}

URoadLane* ARoadActor::GetLane(const FVector2D& UV)
{
	int Side = UV.Y < BaseCurve->GetOffset(UV.X) ? 1 : 0;
	URoadLane* Lane = BaseCurve->GetLane(Side);
	while (Lane)
	{
		URoadBoundary* B = Lane->GetBoundary(Side);
		if ((Side == 0 && B->GetOffset(UV.X) >= UV.Y) || (Side == 1 && B->GetOffset(UV.X) <= UV.Y))
			break;
		Lane = B->GetLane(Side);
	}
	return Lane;
}

FVector2D ARoadActor::GetUV(const FVector2D& Pos)
{
	//It's hard to find a fitful box for octree, optimization is still needed
	FVector2D UV = BaseCurve->Curve.GetUV((const FVector2D&)Pos);
	UV.Y += BaseCurve->GetOffset(UV.X);
	return UV;
}

FVector2D ARoadActor::GetUV(const FVector& Pos)
{
	return GetUV((const FVector2D&)Pos);
}

TArray<URoadLane*> ARoadActor::GetLanes(URoadBoundary* Boundary, int Side, const TArray<ELaneType>& Types)
{
	TArray<URoadLane*> OutLanes;
	while (true)
	{
		URoadLane* Lane = Boundary->GetLane(Side);
		if (!Lane)
			break;
		if (Types.Num())
		{
			for (ELaneType Type : Types)
			{
				if (Lane->GetSegment(Type) != INDEX_NONE)
				{
					OutLanes.Add(Lane);
					break;
				}
			}
		}
		else
			OutLanes.Add(Lane);
		Boundary = Lane->GetBoundary(Side);
		if (Boundary == BaseCurve)
			break;
	}
	return MoveTemp(OutLanes);
}

UMarkingPoint* ARoadActor::AddMarkingPoint(const FVector2D& UV)
{
	UMarkingPoint* Marking = NewObject<UMarkingPoint>(this, NAME_None, RF_Transactional);
	Marking->Point = UV;
	Markings.Add(Marking);
	return Marking;
}

UMarkingCurve* ARoadActor::AddMarkingCurve(bool ClosedLoop)
{
	UMarkingCurve* Marking = NewObject<UMarkingCurve>(this, NAME_None, RF_Transactional);
	Marking->bClosedLoop = ClosedLoop;
	Markings.Add(Marking);
	return Marking;
}

UMarkingCurve* ARoadActor::GetMarkingCurve(UPolygonMarkStyle* FillStyle)
{
	for (URoadMarking* Marking : Markings)
	{
		if (UMarkingCurve* MarkingCurve = Cast<UMarkingCurve>(Marking))
		{
			if (MarkingCurve->FillStyle == FillStyle)
				return MarkingCurve;
		}
	}
	return nullptr;
}

void ARoadActor::DeleteMarking(URoadMarking* Marking)
{
	Marking->ConditionalBeginDestroy();
	Markings.Remove(Marking);
}

void ARoadActor::DeleteAllMarkings()
{
	for (URoadMarking* Marking : Markings)
		Marking->ConditionalBeginDestroy();
	Markings.Empty();
}

void ARoadActor::AddDirPoint(int Index)
{
	//RoadPoints
	{
		int PointIndex = Index == 0 ? 0 : RoadPoints.Num() - 1;
		int DirPoint = Index == 0 ? 1 : RoadPoints.Num() - 2;
		FRoadPoint NewPt = { (RoadPoints[PointIndex].Pos + RoadPoints[DirPoint].Pos) / 2 };
		RoadPoints.Insert(NewPt, DirPoint + (PointIndex > 0));
	}
	//HeightPoints
	{
		int PointIndex = Index == 0 ? 0 : HeightPoints.Num() - 1;
		int DirPoint = Index == 0 ? 1 : HeightPoints.Num() - 2;
		FHeightPoint NewPt = { (HeightPoints[PointIndex].Dist + HeightPoints[DirPoint].Dist) / 2, HeightPoints[PointIndex].Height };
		HeightPoints.Insert(NewPt, DirPoint + (PointIndex > 0));
	}
}

void ARoadActor::DelDirPoint(int Index)
{
	//RoadPoints
	if (RoadPoints.Num() > 2)
	{
		int DirPoint = Index == 0 ? 1 : RoadPoints.Num() - 2;
		RoadPoints.RemoveAt(DirPoint);
	}
	//HeightPoints
	if (HeightPoints.Num() > 2)
	{
		int DirPoint = Index == 0 ? 1 : HeightPoints.Num() - 2;
		HeightPoints.RemoveAt(DirPoint);
	}
}

void ARoadActor::ConnectTo(int& PointIndex, ARoadActor* Parent)
{
	FVector2D UV = Parent->GetUV(RoadPoints[PointIndex].Pos);
	if (!IsUVValid(UV))
		return;
	URoadBoundary* Boundary = Parent->GetBoundary(UV);
	double Sign = Boundary->GetSide() ? -1 : 1;
	FVector ParentDir = Parent->GetDir(UV.X) * Sign;
	FVector Dir = GetDir(PointIndex > 0 ? Length() : 0);
	if ((ParentDir | Dir) < 0.7071)
		return;
	double Snap = 800;
	if ((PointIndex > 0) ^ (Sign < 0))
	{
		if (FMath::IsNearlyEqual(UV.X, 0, Snap))
		{
			UV.X = 0;
			RoadPoints[PointIndex].Pos = FVector2D(Parent->GetPos(UV));
		}
	}
	else
	{
		double Len = Parent->Length();
		if (FMath::IsNearlyEqual(UV.X, Len, Snap))
		{
			UV.X = Len;
			RoadPoints[PointIndex].Pos = FVector2D(Parent->GetPos(UV));
		}
	}
	UV.Y = Boundary->GetOffset(UV.X);
	ConnectTo(Parent, UV, PointIndex > 0);
	if (PointIndex > 0)
		PointIndex = RoadPoints.Num() - 1;
}

void ARoadActor::ConnectTo(ARoadActor* Parent, const FVector2D& UV, int Index)
{
	AddDirPoint(Index);
	ConnectedParents[Index] = Parent;
	FConnectInfo& TargetInfo = Parent->ConnectedChildren[Parent->ConnectedChildren.AddDefaulted()];
	TargetInfo.UV = UV;
	TargetInfo.Child = this;
	TargetInfo.Index = Index;
	TargetInfo.UpdateChild(Parent);
}

void ARoadActor::DisconnectFrom(ARoadActor* Child, int Index)
{
	for (int i = 0; i < ConnectedChildren.Num(); i++)
	{
		FConnectInfo& Info = ConnectedChildren[i];
		if (Info.Child == Child && Info.Index == Index)
		{
			ConnectedChildren.RemoveAt(i);
			return;
		}
	}
}

void ARoadActor::DisconnectAll()
{
	for (int i = 0; i < 2; i++)
	{
		if (ConnectedParents[i])
			ConnectedParents[i]->DisconnectFrom(this, i);
	}
	while (ConnectedChildren.Num())
	{
		int PointIndex = ConnectedChildren[0].Index > 0 ? ConnectedChildren[0].Child->RoadPoints.Num() - 1 : 0;
		ConnectedChildren[0].Child->DisconnectAt(PointIndex);
	}
}

void ARoadActor::DisconnectAll(int& PointIndex)
{
	if (ConnectedParents[PointIndex > 0])
	{
		DisconnectAt(PointIndex);
		if (PointIndex > 0)
			PointIndex = RoadPoints.Num() - 1;
	}
	double Dist = PointIndex ? Length() : 0;
	for (int i = 0; i < ConnectedChildren.Num();)
	{
		if (FMath::IsNearlyEqual(ConnectedChildren[i].UV.X, Dist))
			ConnectedChildren[i].Child->DisconnectAt(ConnectedChildren[i].Index > 0 ? ConnectedChildren[i].Child->RoadPoints.Num() - 1 : 0);
		else
			i++;
	}
}

void ARoadActor::DisconnectAt(int PointIndex)
{
	int Index = PointIndex ? 1 : 0;
	if (ConnectedParents[Index]->ConnectedParents[!Index] != this)
		DelDirPoint(Index);
	ConnectedParents[Index]->DisconnectFrom(this, Index);
	ConnectedParents[Index] = nullptr;
}

void ARoadActor::DeleteBoundary(URoadBoundary* Boundary)
{
	Boundary->Modify();
	GetScene()->OctreeRemoveBoundary(Boundary);
	Boundary->ConditionalBeginDestroy();
	Boundaries.Remove(Boundary);
}

void ARoadActor::DeleteLane(URoadLane* Lane)
{
	Lane->Modify();
	if (Lane->GetSide())
	{
		if (Lane->LeftBoundary->LeftLane)
		{
			Lane->LeftBoundary->LeftLane->Modify();
			Lane->LeftBoundary->LeftLane->RightBoundary = Lane->RightBoundary;
		}
		Lane->RightBoundary->Modify();
		Lane->RightBoundary->LeftLane = Lane->LeftBoundary->LeftLane;
		DeleteBoundary(Lane->LeftBoundary);
	}
	else
	{
		if (Lane->RightBoundary->RightLane)
		{
			Lane->RightBoundary->RightLane->Modify();
			Lane->RightBoundary->RightLane->LeftBoundary = Lane->LeftBoundary;
		}
		Lane->LeftBoundary->Modify();
		Lane->LeftBoundary->RightLane = Lane->RightBoundary->RightLane;
		DeleteBoundary(Lane->RightBoundary);
	}
	Lane->ConditionalBeginDestroy();
	Lanes.Remove(Lane);
}

void ARoadActor::Join(ARoadActor* Road)
{
	ARoadScene* Scene = GetScene();
	Scene->Modify();
	Modify();
	RoadPoints.Pop();
	HeightPoints.Pop();
	RoadPoints.Append(&Road->RoadPoints[1], Road->RoadPoints.Num() - 1);
	HeightPoints.Append(&Road->HeightPoints[1], Road->HeightPoints.Num() - 1);
	JoinLanes(Road);
	UpdateCurve();
	Scene->DestroyRoad(Road);
}

void ARoadActor::JoinLanes(ARoadActor* Road)
{
	TArray<URoadLane*> LeftLanes = GetLanes(1);
	TArray<URoadLane*> RightLanes = GetLanes(0);
	TArray<URoadLane*> OtherLeftLanes = Road->GetLanes(1);
	TArray<URoadLane*> OtherRightLanes = Road->GetLanes(0);
	BaseCurve->Join(Road->BaseCurve);
	for (int i = 0; i < FMath::Min(LeftLanes.Num(), OtherLeftLanes.Num()); i++)
	{
		LeftLanes[i]->Join(OtherLeftLanes[i]);
		LeftLanes[i]->LeftBoundary->Join(OtherLeftLanes[i]->LeftBoundary);
	}
	for (int i = 0; i < FMath::Min(RightLanes.Num(), OtherRightLanes.Num()); i++)
	{
		RightLanes[i]->Join(OtherRightLanes[i]);
		RightLanes[i]->RightBoundary->Join(OtherRightLanes[i]->RightBoundary);
	}
}

void ARoadActor::CutLanes(double R_Start, double R_End)
{
	for (URoadLane* Lane : Lanes)
		Lane->Cut(R_Start, R_End);
	for (URoadBoundary* Boundary : Boundaries)
		Boundary->Cut(R_Start, R_End);
}

ARoadActor* ARoadActor::Chop(double Dist)
{
	ARoadScene* Scene = GetScene();
	Scene->Modify();
	Modify();
	double EndDist = Length();
	ARoadActor* Road = Scene->DuplicateRoad(this);
	RoadPoints = CalcRoadPoints(0, Dist);
	HeightPoints = CalcHeightPoints(0, Dist);
	CutLanes(0, Dist);
	Road->RoadPoints = CalcRoadPoints(Dist, EndDist);
	Road->HeightPoints = CalcHeightPoints(Dist, EndDist);
	Road->CutLanes(Dist, EndDist);
	if (ConnectedParents[1])
		ConnectedParents[1]->GetConnectedChild(this, 1).Child = Road;
	for (int i = 0; i < ConnectedChildren.Num();)
	{
		FConnectInfo& Info = ConnectedChildren[i];
		if (Info.UV.X > Dist)
		{
			Info.Child->ConnectedParents[Info.Index] = Road;
			ConnectedChildren.RemoveAt(i);
		}
		else
			i++;
	}
	for (int i = 0; i < Road->ConnectedChildren.Num();)
	{
		FConnectInfo& Info = Road->ConnectedChildren[i];
		if (Info.UV.X <= Dist)
			Road->ConnectedChildren.RemoveAt(i);
		else
		{
			Info.UV.X -= Dist;
			i++;
		}
	}
	UpdateCurve();
	Road->UpdateCurve();
	ConnectTo(Road, FVector2D(0, 0), 1);
	Road->ConnectTo(this, FVector2D(Dist, 0), 0);
	return Road;
}

ARoadActor* ARoadActor::Split(URoadBoundary* Boundary)
{
	ARoadScene* Scene = GetScene();
	Scene->Modify();
	Modify();
	ARoadActor* Road = Scene->AddRoad();
	int Side = Boundary->GetSide();
	USettings_Global* Settings = GetMutableDefault<USettings_Global>();
	USettings_RoadSplit* SplitSettings = GetMutableDefault<USettings_RoadSplit>();
	URoadStyle* Style = URoadStyle::Create(Boundary, Side, Boundary, Side, false, false, 0xFFFFFFFF, 0xFFFFFFFF);
	Road->InitWithStyle(Style);
	if (Side)
		Road->AddSubRoad(Boundary, Length(), 0, (SplitSettings->CornerRadius + SplitSettings->ShoulderWidth) * 2);
	else
		Road->AddSubRoad(Boundary, 0, Length(), (SplitSettings->CornerRadius + SplitSettings->ShoulderWidth) * 2);
	Road->RoadPoints = Road->CalcRoadPoints(0, Road->Length());
	Road->AddPoint(SplitSettings->JunctionSize);
	Road->AddPoint(Road->Length() - SplitSettings->JunctionSize);
	Road->HeightPoints = HeightPoints;
	for (int i = 0; i < 2; i++)
	{
		if (ARoadActor* Parent = ConnectedParents[i])
		{
			Parent->Modify();
			int Index = (Side ^ i) ? Road->RoadPoints.Num() - 1 : 0;
			FVector2D UV = Parent->GetUV(Road->RoadPoints[Index].Pos);
			Road->ConnectTo(Parent, UV, Index > 0);
		}
	}
	Road->BaseCurve->Segments[0].LaneMarking = Settings->DefaultSolidStyle.LoadSynchronous();
	Road->AddLane(nullptr, Road->BaseCurve, { SplitSettings->ShoulderWidth, ELaneType::Shoulder });
	Road->UpdateCurve();
	URoadLane* Lane = Boundary->GetLane(Side);
	while (Lane)
	{
		URoadBoundary* B = Lane->GetBoundary(Side);
		DeleteLane(Lane);
		Lane = B->GetLane(Side);
	}
	Boundary->Segments[0].LaneMarking = Settings->DefaultSolidStyle.LoadSynchronous();
	if (Side)
		AddLane(nullptr, Boundary, { SplitSettings->ShoulderWidth, ELaneType::Shoulder });
	else
		AddLane(Boundary, nullptr, { SplitSettings->ShoulderWidth, ELaneType::Shoulder });
	UpdateCurve();
	return Road;
}

TArray<FRoadPoint> ARoadActor::CalcRoadPoints(double R_Start, double R_End)
{
	TArray<FRoadPoint> Points;
	for (int i = 0; i < RoadSegments.Num(); i++)
	{
		FRoadSegment& RoadSegment = RoadSegments[i];
		double S_Start = FMath::Max(R_Start, RoadSegment.Dist);
		double S_End = FMath::Min(R_End, RoadSegment.Dist + RoadSegment.Length);
		if (S_Start < S_End)
		{
			double Start = S_Start - RoadSegment.Dist;
			double End = S_End - RoadSegment.Dist;
			FVector2D StartPos = RoadSegment.GetPos(Start);
			FVector2D EndPos = RoadSegment.GetPos(End);
			FVector2D StartDir = RoadSegment.GetDir(Start);
			FVector2D EndDir = RoadSegment.GetDir(End);
			if (S_Start == R_Start)
				Points.Add({ StartPos, RoadSegment.Dist + Start });
			double StartDist, EndDist;
			if (DoLinesIntersect(StartPos, StartDir, EndPos, -EndDir, StartDist, EndDist))
				Points.Add({ StartPos + StartDir * StartDist,  RoadSegment.Dist + StartDist });
			if (S_End == R_End)
				Points.Add({ EndPos, RoadSegment.Dist + End });
		}
	}
	return MoveTemp(Points);
}

TArray<FHeightPoint> ARoadActor::CalcHeightPoints(double R_Start, double R_End)
{
	TArray<FHeightPoint> Points;
	for (int i = 0; i < HeightSegments.Num() - 1; i++)
	{
		FHeightSegment& ThisSeg = HeightSegments[i];
		FHeightSegment& NextSeg = HeightSegments[i + 1];
		double S_Start = FMath::Max(R_Start, ThisSeg.Dist);
		double S_End = FMath::Min(R_End, NextSeg.Dist);
		if (S_Start < S_End)
		{
			double Start = S_Start - ThisSeg.Dist;
			double End = S_End - ThisSeg.Dist;
			double StartH = ThisSeg.Get(Start);
			double EndH = ThisSeg.Get(End);
			double H = ThisSeg.Get((Start + End) / 2);
			if (S_Start == R_Start)
				Points.Add({ S_Start - R_Start, StartH });
			if (!FMath::IsNearlyEqual(StartH, EndH))
				Points.Add({ (S_Start + S_End) / 2 - R_Start, H });
			if (S_End == R_End)
				Points.Add({ S_End - R_Start, EndH });
		}
	}
	return MoveTemp(Points);
}

TArray<FRoadSegment> ARoadActor::CutRoadSegments(double R_Start, double R_End)
{
	TArray<FRoadSegment> Segments;
	for (int i = 0; i < RoadSegments.Num(); i++)
	{
		FRoadSegment& RoadSegment = RoadSegments[i];
		double S_Start = FMath::Max(R_Start, RoadSegment.Dist);
		double S_End = FMath::Min(R_End, RoadSegment.Dist + RoadSegment.Length);
		if (S_Start < S_End)
		{
			double Dist = S_Start - R_Start;
			double Start = S_Start - RoadSegment.Dist;
			double End = S_End - RoadSegment.Dist;
			FRoadSegment& Segment = Segments[Segments.AddDefaulted()];
			Segment.Dist = Dist;
			Segment.Length = End - Start;
			Segment.StartPos = RoadSegment.GetPos(Start);
			Segment.StartRadian = RoadSegment.GetR(Start);
			Segment.StartCurv = RoadSegment.GetC(Start);
			Segment.EndCurv = RoadSegment.GetC(End);
		}
	}
	return MoveTemp(Segments);
}

TArray<FHeightSegment> ARoadActor::CutHeightSegments(double R_Start, double R_End)
{
	TArray<FHeightSegment> Segments;
	for (int i = 0; i < HeightSegments.Num() - 1; i++)
	{
		FHeightSegment& ThisSeg = HeightSegments[i];
		FHeightSegment& NextSeg = HeightSegments[i + 1];
		double S_Start = FMath::Max(R_Start, ThisSeg.Dist);
		double S_End = FMath::Min(R_End, NextSeg.Dist);
		if (S_Start < S_End)
		{
			double Start = S_Start - ThisSeg.Dist;
			double End = S_End - ThisSeg.Dist;
			double Length = NextSeg.Dist - ThisSeg.Dist;
			check(Length > 0);
			FHeightSegment& NewSeg = Segments[Segments.AddDefaulted()];
			NewSeg.Dist = S_Start - R_Start;
			NewSeg.Height = FMath::CubicInterp(ThisSeg.Height, ThisSeg.Dir * Length, NextSeg.Height, NextSeg.Dir * Length, Start / Length);
			NewSeg.Dir = FMath::CubicInterpDerivative(ThisSeg.Height, ThisSeg.Dir * Length, NextSeg.Height, NextSeg.Dir * Length, Start / Length) / Length;
			if (S_End <= NextSeg.Dist)
			{
				FHeightSegment& LastSeg = Segments[Segments.AddDefaulted()];
				LastSeg.Dist = S_End - R_Start;
				LastSeg.Height = FMath::CubicInterp(ThisSeg.Height, ThisSeg.Dir * Length, NextSeg.Height, NextSeg.Dir * Length, End / Length);
				LastSeg.Dir = FMath::CubicInterpDerivative(ThisSeg.Height, ThisSeg.Dir * Length, NextSeg.Height, NextSeg.Dir * Length, End / Length) / Length;
			}
		}
	}
	return MoveTemp(Segments);
}

void ARoadActor::AddArcs(const FVector2D& SrcPos, const FVector2D& SrcDir, const FVector2D& DstPos, const FVector2D& DstDir, double Radius, double StartH, double EndH)
{
	double PrevMinSize, NextMinSize, PrevMaxSize, NextMaxSize;
	double Distance = FVector2D::Distance(SrcPos, DstPos);
	PrevMinSize = NextMinSize = 0;
	PrevMaxSize = NextMaxSize = Distance;
	double PrevSize = (PrevMinSize + PrevMaxSize) / 2;
	double NextSize = (NextMinSize + NextMaxSize) / 2;
	double Half, Radian, PrevDiff, NextDiff;
	double SrcRadian = FMath::Atan2(SrcDir.Y, SrcDir.X);
	double DstRadian = FMath::Atan2(DstDir.Y, DstDir.X);
	for (int i = 0; i < 100; i++)
	{
		FVector2D P0 = SrcPos + SrcDir * PrevSize;
		FVector2D P1 = DstPos - DstDir * NextSize;
		FVector2D Dir = P1 - P0;
		double Len = Dir.Size();
		Dir /= Len;
		Half = Len / 2;
		Radian = FMath::Atan2(Dir.Y, Dir.X);
		PrevDiff = WrapRadian(Radian - SrcRadian);
		NextDiff = WrapRadian(DstRadian - Radian);
		bool PrevOK = FMath::IsNearlyEqual(Half, PrevSize, DOUBLE_KINDA_SMALL_NUMBER);
		bool NextOK = FMath::IsNearlyEqual(Half, NextSize, DOUBLE_KINDA_SMALL_NUMBER);
		if (!PrevOK)
		{
			if (Half > PrevSize)
			{
				PrevMinSize = PrevSize;
				PrevSize = (PrevMinSize + PrevMaxSize) / 2;
			}
			else
			{
				double DesiredSize = Radius * FMath::Abs(FMath::Tan(PrevDiff / 2));
				if (!FMath::IsNearlyEqual(DesiredSize, Half, DOUBLE_KINDA_SMALL_NUMBER))
				{
					if (Half < DesiredSize)
						PrevMaxSize = PrevSize;
					else
						PrevMinSize = PrevSize;
					PrevSize = (PrevMinSize + PrevMaxSize) / 2;
				}
				else
					PrevOK = true;
			}
		}
		if (!NextOK)
		{
			if (Half > NextSize)
			{
				NextMinSize = NextSize;
				NextSize = (NextMinSize + NextMaxSize) / 2;
			}
			else
			{
				double DesiredSize = Radius * FMath::Abs(FMath::Tan(NextDiff / 2));
				if (!FMath::IsNearlyEqual(DesiredSize, Half, DOUBLE_KINDA_SMALL_NUMBER))
				{
					if (Half < DesiredSize)
						NextMaxSize = NextSize;
					else
						NextMinSize = NextSize;
					NextSize = (NextMinSize + NextMaxSize) / 2;
				}
				else
					NextOK = true;
			}
		}
		if (PrevOK && NextOK)
			break;
	}
	double BaseS = Length();
	double S = BaseS;
	double PrevLineSize = PrevSize - Half;
	double NextLineSize = NextSize - Half;
	FVector2D Pos = SrcPos;
	//Start Curv
	{
		if (PrevLineSize > DOUBLE_KINDA_SMALL_NUMBER)
		{
			S += AddRoadSegment(S, PrevLineSize, Pos, SrcRadian, 0, 0).Length;
			Pos += SrcDir * PrevLineSize;
		}
		double C = FMath::Tan(PrevDiff / 2) / Half;
		double ArcLen = FMath::IsNearlyZero(C) ? Distance / 2 : PrevDiff / C;
		S += AddRoadSegment(S, ArcLen, Pos, SrcRadian, C, C).Length;
		Pos = GetSpiralPos(Pos.X, Pos.Y, SrcRadian, C, 0, ArcLen);
	}
	//End Curv
	{
		double C = FMath::Tan(NextDiff / 2) / Half;
		double ArcLen = FMath::IsNearlyZero(C) ? Distance / 2 : NextDiff / C;
		S += AddRoadSegment(S, ArcLen, Pos, Radian, C, C).Length;
		Pos = GetSpiralPos(Pos.X, Pos.Y, Radian, C, 0, ArcLen);
		if (NextLineSize > DOUBLE_KINDA_SMALL_NUMBER)
		{
			S += AddRoadSegment(S, NextLineSize, Pos, DstRadian, 0, 0).Length;
			Pos += DstDir * NextLineSize;
		}
	}
	double SegmentLen = S - BaseS;
	double HDir = (EndH - StartH) / SegmentLen;
	if (!HeightSegments.Num())
		AddHeightSegment(BaseS, StartH, HDir);
	AddHeightSegment(S, EndH, HDir);
}

void ARoadActor::AddSubRoad(URoadBoundary* Boundary, double Start, double End, double BaseOffset)
{
	double Min = FMath::Min(Start, End);
	double Max = FMath::Max(Start, End);
	double Sign = FMath::Sign(End - Start);
	TArray<FCurveOffset> Offsets = Boundary->CutOffsets(Min, Max);
	ARoadActor* BaseRoad = Boundary->GetRoad();
	TArray<FRoadSegment> Roads = BaseRoad->CutRoadSegments(Min, Max);
	TArray<FHeightSegment> Heights = BaseRoad->CutHeightSegments(Min, Max);
	if (Roads.Num() && Heights.Num())
	{
		if (Sign < 0)
		{
			TArray<FRoadSegment> InvRoads;
			TArray<FHeightSegment> InvHeights;
			InvRoads.AddUninitialized(Roads.Num());
			InvHeights.AddUninitialized(Heights.Num());
			for (int i = 0; i < Roads.Num(); i++)
				InvRoads[i] = Roads[Roads.Num() - i - 1].Reverse();
			for (int i = 0; i < Heights.Num(); i++)
				InvHeights[i] = Heights[Heights.Num() - i - 1].Reverse();
			Roads = MoveTemp(InvRoads);
			Heights = MoveTemp(InvHeights);
		}
		double InitS = RoadSegments.Num() ? RoadSegments.Last().Dist + RoadSegments.Last().Length : 0;
		double S = InitS;
		double StartOffset = Sign > 0 ? (Offsets[0].Offset - BaseCurve->LocalOffsets.Last().Offset) : (Offsets.Last().Offset + BaseCurve->LocalOffsets.Last().Offset);
		double ShiftOffset = Sign > 0 ? (Offsets[0].Offset - BaseCurve->LocalOffsets.Last().Offset) : (Offsets.Last().Offset - BaseCurve->LocalOffsets.Last().Offset);
		for (int i = 0; i < Roads.Num(); i++)
		{
			FRoadSegment Road = Roads[i].ApplyOffset(StartOffset * Sign + BaseOffset);
			if (Road.Length > 0)
			{
				AddRoadSegment(S, Road.Length, Road.StartPos, Road.StartRadian, Road.StartCurv, Road.EndCurv);
				S += Road.Length;
			}
		}
		double L = S - InitS;
		double RL = FMath::Max(Heights[0].Dist, Heights.Last().Dist);
		double LScale = L / RL;
		for (FHeightSegment& Height : Heights)
			Height.Dist = Height.Dist * LScale;
		S = InitS;
		for (int i = 0; i < Heights.Num(); i++)
		{
			FHeightSegment& Height = Heights[i];
			AddHeightSegment(S, Height.Height, Height.Dir);
			if (i + 1 < Heights.Num())
				S += FMath::Abs(Heights[i + 1].Dist - Height.Dist);
		}
		check(FMath::IsNearlyEqual(Length(), HeightSegments.Last().Dist));
		//Only Corner link need copy offsets?
		auto CopyOffset = [&](URoadBoundary* SrcB, URoadBoundary* DstB, TArray<FCurveOffset>& LocalOffsets)
		{
			if (Sign < 0)
			{
				Algo::Reverse(LocalOffsets);
				double Length = LocalOffsets[0].Dist;
				for (FCurveOffset& Offset : LocalOffsets)
					Offset.Dist = Length - Offset.Dist;
			}
			for (FCurveOffset& LocalOffset : LocalOffsets)
				LocalOffset.Dist = InitS + LocalOffset.Dist * LScale;
			for (int i = 0; i < DstB->LocalOffsets.Num();)
			{
				if (FMath::IsNearlyEqual(DstB->LocalOffsets[i].Dist, LocalOffsets[0].Dist))
					DstB->LocalOffsets.RemoveAt(i);
				else
					i++;
			}
			DstB->LocalOffsets.Append(LocalOffsets);
			DstB->CleanLocalOffsets();
		};
		for (FCurveOffset& Offset : Offsets)
			Offset.Offset -= ShiftOffset;
		CopyOffset(Boundary, BaseCurve, Offsets);
		auto CopyAllOffset = [&](int SrcSide, int DstSide)
		{
			URoadLane* DstL = BaseCurve->GetLane(DstSide);
			URoadLane* SrcL = Boundary->GetLane(SrcSide);
			while (DstL && SrcL)
			{
				URoadBoundary* DstB = DstL->GetBoundary(DstL->GetSide());
				URoadBoundary* SrcB = SrcL->GetBoundary(SrcL->GetSide());
				TArray<FCurveOffset> LocalOffsets = SrcB->CutLocalOffsets(Min, Max);
				CopyOffset(SrcB, DstB, LocalOffsets);
				DstL = DstL->GetBoundary(DstSide)->GetLane(DstSide);
				SrcL = SrcL->GetBoundary(SrcSide)->GetLane(SrcSide);
			}
		};
		if (true)
		{
			CopyAllOffset(0, Sign > 0 ? 0 : 1);
			CopyAllOffset(1, Sign > 0 ? 1 : 0);
		}
		else
		{
			for (URoadBoundary* DstB : Boundaries)
			{
				if (DstB != BaseCurve)
				{
					DstB->LocalOffsets.Add({ Length(), DstB->LocalOffsets.Last().Offset });
					DstB->CleanLocalOffsets();
				}
			}
		}
	}
}

void ARoadActor::ClearSegments()
{
	RoadSegments.Empty();
	HeightSegments.Empty();
	for (URoadBoundary* Boundary : Boundaries)
	{
		Boundary->LocalOffsets.SetNum(2);
		Boundary->LocalOffsets[0].Dist = Boundary->LocalOffsets[1].Dist = 0;
		if (Boundary == BaseCurve)
			Boundary->LocalOffsets[0].Offset = Boundary->LocalOffsets[1].Offset = 0;
	}
}

void ARoadActor::ClearLanes()
{
	ARoadScene* Scene = GetScene();
	for (URoadBoundary* Boundary : Boundaries)
	{
		Scene->OctreeRemoveBoundary(Boundary);
		Boundary->ConditionalBeginDestroy();
	}
	for (URoadLane* Lane : Lanes)
	{
		Lane->ConditionalBeginDestroy();
	}
	Boundaries.Reset();
	Lanes.Reset();
}

void ARoadActor::Init(double Height)
{
	if (Boundaries.Num())
	{
		for (URoadBoundary* Boundary : Boundaries)
			Boundary->ConditionalBeginDestroy();
		Boundaries.Empty();
	}
	if (Lanes.Num())
	{
		for (URoadLane* Lane : Lanes)
			Lane->ConditionalBeginDestroy();
		Lanes.Empty();
	}
	BaseCurve = AddBoundary(0, nullptr, nullptr);
	if (!HeightPoints.Num())
	{
		HeightPoints.AddDefaulted(2);
		HeightPoints[0].Height = Height;
		HeightPoints[1].Height = Height;
	}
}

void ARoadActor::InitWithStyle(URoadStyle* Style, double Height)
{
	Init(Height);
	URoadLane* Lane = nullptr;
	if (Style)
	{
		bHasGround = Style->bHasGround;
		BaseCurve->Segments[0].LaneMarking = Style->BaseCurveMark;
		BaseCurve->Segments[0].Props = Style->BaseCurveProps;
		for (int i = 0; i < Style->RightLanes.Num(); i++)
			Lane = AddLane(i > 0 ? Lane->RightBoundary : BaseCurve, nullptr, Style->RightLanes[i]);
		for (int i = 0; i < Style->LeftLanes.Num(); i++)
			Lane = AddLane(nullptr, i > 0 ? Lane->LeftBoundary : BaseCurve, Style->LeftLanes[i]);
	}
	/*
	else
	{
		Lane = AddLane(BaseCurve, nullptr, { 350 });
		Lane = AddLane(Lane->RightBoundary, nullptr, { 50 });
		Lane = AddLane(Lane->RightBoundary, nullptr, { 350 });
		Lane = AddLane(nullptr, BaseCurve, { 350 });
		Lane = AddLane(nullptr, Lane->LeftBoundary, { 50 });
		Lane = AddLane(nullptr, Lane->LeftBoundary, { 350 });
	}*/
}

void ARoadActor::InitWithRoads(URoadBoundary* Base, int Side, bool SkipSidewalks, bool KeepLeftLanes, uint32 LeftLaneMarkingMask, uint32 RightLaneMarkingMask)
{
	TArray<FRoadLaneStyle> LeftLanes;
	TArray<FRoadLaneStyle> RightLanes;
	ARoadActor* BaseRoad = Base->GetRoad();
	URoadBoundary* Boundary = Base;
	while (true)
	{
		FRoadLaneStyle LaneStyle;
		URoadLane* Lane = Boundary->GetLane(Side);
		if (!Lane)
			break;
		Boundary = Lane->GetBoundary(Side);
		LaneStyle.LaneType = Lane->Segments[0].LaneType;
		if (SkipSidewalks && LaneStyle.LaneType == ELaneType::Sidewalk)
			break;
		LaneStyle.Width = Boundary->LocalOffsets[0].Offset;
		LaneStyle.LaneShape = Lane->Segments[0].LaneShape;
		if ((1 << RightLanes.Num()) & RightLaneMarkingMask)
		{
			LaneStyle.LaneMarking = Boundary->Segments[0].LaneMarking;
			LaneStyle.Props = Boundary->Segments[0].Props;
		}
		RightLanes.Add(LaneStyle);
	}
	if (KeepLeftLanes)
	{
		Boundary = Base;
		while (Boundary != BaseRoad->BaseCurve)
		{
			FRoadLaneStyle LaneStyle;
			URoadLane* Lane = Boundary->GetLane(!Side);
			LaneStyle.LaneType = Lane->Segments[0].LaneType;
		//	Sidewalk never comes from left side
		//	if (SkipSidewalks && LaneStyle.LaneType == ELaneType::Sidewalk)
		//		break;
			LaneStyle.Width = Boundary->LocalOffsets[0].Offset;
			LaneStyle.LaneShape = Lane->Segments[0].LaneShape;
			Boundary = Lane->GetBoundary(!Side);
			if ((1 << LeftLanes.Num()) & LeftLaneMarkingMask)
			{
				LaneStyle.LaneMarking = Boundary->Segments[0].LaneMarking;
				LaneStyle.Props = Boundary->Segments[0].Props;
			}
			LeftLanes.Add(LaneStyle);
		}
	}
	Init(0);
	URoadLane* Lane = nullptr;
	if (((LeftLaneMarkingMask & 1) || (RightLaneMarkingMask & 1)))
	{
		BaseCurve->Segments[0].LaneMarking = Base->Segments[0].LaneMarking;
		BaseCurve->Segments[0].Props = Base->Segments[0].Props;
	}
	for (int i = 0; i < RightLanes.Num(); i++)
		Lane = AddLane(i > 0 ? Lane->RightBoundary : BaseCurve, nullptr, RightLanes[i]);
	for (int i = 0; i < LeftLanes.Num(); i++)
		Lane = AddLane(nullptr, i > 0 ? Lane->LeftBoundary : BaseCurve, LeftLanes[i]);
}

void ARoadActor::UpdateCurve(TSet<ARoadActor*>& UpdatedRoads)
{
	double S = 0;
	double LastSize = 0;
	double LastLength = Length();
	RoadSegments.Reset();
	for (int i = 0; i < RoadPoints.Num(); i++)
	{
		if (i > 0)
		{
			if (i == RoadPoints.Num() - 1)
			{
				FVector2D PrevDir = RoadPoints[i].Pos - RoadPoints[i - 1].Pos;
				double PrevSize = PrevDir.Size();
				PrevDir /= PrevSize;
				double Radian = FMath::Atan2(PrevDir.Y, PrevDir.X);
				double CutSize = PrevSize - LastSize;
				if (CutSize > DOUBLE_KINDA_SMALL_NUMBER)
					S += AddRoadSegment(S, CutSize, RoadPoints[i - 1].Pos + PrevDir * LastSize, Radian, 0, 0).Length;
				RoadPoints[i].Dist = S;
				LastSize = 0;
			}
			else
			{
				FVector2D PrevDir = RoadPoints[i].Pos - RoadPoints[i - 1].Pos;
				FVector2D NextDir = RoadPoints[i + 1].Pos - RoadPoints[i].Pos;
				double PrevSize = PrevDir.Size();
				double NextSize = NextDir.Size();
				PrevDir /= PrevSize;
				NextDir /= NextSize;
				FVector2D Dir = (PrevDir + NextDir).GetSafeNormal();
				//	double Cos = PrevDir | Dir;
				double PrevRadian = FMath::Atan2(PrevDir.Y, PrevDir.X);
				double NextRadian = FMath::Atan2(NextDir.Y, NextDir.X);
				double Diff = WrapRadian(NextRadian - PrevRadian);
				double Cos = FMath::Cos(Diff / 2);
				NextRadian = PrevRadian + Diff;
				double Size = FMath::Min((i - 1 > 0) ? PrevSize / 2 : PrevSize, (i + 1 < RoadPoints.Num() - 1) ? NextSize / 2 : NextSize);
				double Radius = FMath::Min(RoadPoints[i].MaxRadius, Size / FMath::Abs(FMath::Tan(Diff / 2)));
				FVector2D StartPos = RoadPoints[i].Pos - PrevDir * Size;
				//	FVector EndPos = RoadPoints[i].Pos + NextDir * Size;
				double MinC = 1.0 / Radius * FMath::Sign(Diff);
				double MaxC = 2.0 / Radius * FMath::Sign(Diff);
				double C = MinC;
				double SpiralDiff = Diff / 2 * RoadPoints[i].CurvatureBlend;
				double ArcDiff = Diff / 2 * (1 - RoadPoints[i].CurvatureBlend);
				double SpiralEnd = PrevRadian + SpiralDiff;
				double ArcEnd = SpiralEnd + ArcDiff * 2;
				for (int j = 0; j < 100; j++)
				{
					FVector2D Start = StartPos;
					if (SpiralDiff != 0)
					{
						double SpiralLen = ComputeSpiralLength(PrevRadian, SpiralEnd, 0, C);
						double G = C / SpiralLen;
						Start = GetSpiralPos(Start.X, Start.Y, PrevRadian, 0, G, SpiralLen);
					}
					if (ArcDiff != 0)
					{
						double ArcLen = ArcDiff / C;
						Start = GetSpiralPos(Start.X, Start.Y, SpiralEnd, C, 0, ArcLen);
					}
					double Dist = (Start - FVector2D(RoadPoints[i].Pos)) | FVector2D(Dir);
					if (FMath::IsNearlyZero(Dist, SMALL_NUMBER))
						break;
					if (Dist < 0)
					{
						if (j == 0)
						{
							Size += Dist / Cos;
							StartPos = RoadPoints[i].Pos - PrevDir * Size;
							break;
						}
						MaxC = C;
						C = (MinC + MaxC) / 2;
					}
					else
					{
						MinC = C;
						C = (MinC + MaxC) / 2;
					}
				}
				double CutSize = PrevSize - LastSize - Size;
				if (CutSize > DOUBLE_KINDA_SMALL_NUMBER)
				{
					S += AddRoadSegment(S, CutSize, RoadPoints[i - 1].Pos + PrevDir * LastSize, PrevRadian, 0, 0).Length;
				}
				FVector2D Start = StartPos;
				if (SpiralDiff != 0)
				{
					double SpiralLen = ComputeSpiralLength(PrevRadian, SpiralEnd, 0, C);
					FRoadSegment& Segment = AddRoadSegment(S, SpiralLen, Start, PrevRadian, 0, C);
					S += Segment.Length;
					Start = Segment.GetPos(Segment.Length);
				}
				RoadPoints[i].Dist = S;
				if (ArcDiff != 0)
				{
					double ArcLen = 2 * ArcDiff / C;
					FRoadSegment& Segment = AddRoadSegment(S, ArcLen, Start, SpiralEnd, C, C);
					S += Segment.Length;
					Start = Segment.GetPos(Segment.Length);
				}
				RoadPoints[i].Dist = (RoadPoints[i].Dist + S) / 2;
				if (SpiralDiff != 0)
				{
					double SpiralLen = ComputeSpiralLength(ArcEnd, NextRadian, C, 0);
					FRoadSegment& Segment = AddRoadSegment(S, SpiralLen, Start, ArcEnd, C, 0);
					S += Segment.Length;
					Start = Segment.GetPos(Segment.Length);
				}
				LastSize = Size;
			}
		}
		else
			RoadPoints[i].Dist = S;
	}
	HeightPoints.Last().Dist = Length();
	if (LastLength > 0)
	{
		double Scale = Length() / LastLength;
		for (int i = 1; i < HeightPoints.Num() - 1; i++)
			HeightPoints[i].Dist *= Scale;
	}
	HeightSegments.Reset();
	LastSize = 0;
	for (int i = 1; i < HeightPoints.Num(); i++)
	{
		FHeightPoint& This = HeightPoints[i];
		FHeightPoint& Prev = HeightPoints[i - 1];
		double PrevSize = This.Dist - Prev.Dist;
		double Dir = (This.Height - Prev.Height) / PrevSize;
		if (i < HeightPoints.Num() - 1)
		{
			FHeightPoint& Next = HeightPoints[i + 1];
			double NextSize = Next.Dist - This.Dist;
			double Size = FMath::Min(FMath::Min(PrevSize / 2, This.Range / 2), FMath::Min(NextSize / 2, This.Range / 2));
			double CutSize = PrevSize - Size - LastSize;
			if (CutSize > 0)
				AddHeightSegment(Prev.Dist + LastSize, Prev.Height + Dir * LastSize, Dir);
			AddHeightSegment(Prev.Dist + (PrevSize - Size), Prev.Height + Dir * (PrevSize - Size), Dir);
			LastSize = Size;
		}
		else
		{
			if (Prev.Dist + LastSize < This.Dist)
				AddHeightSegment(Prev.Dist + LastSize, Prev.Height + Dir * LastSize, Dir);
			AddHeightSegment(This.Dist, This.Height, Dir);
		}
	}
	UpdateLanes();
	for (FConnectInfo& Info : ConnectedChildren)
	{
	//	Info.UV.X = Info.UV.X / LastLength * Length();
		FVector2D UV = GetUV(Info.GetPos());
		Info.UV.X = FMath::Clamp(UV.X, 0, Length());
		if (!UpdatedRoads.Contains(Info.Child))
		{
			Info.UpdateChild(this);
			UpdatedRoads.Add(Info.Child);
			Info.Child->UpdateCurve(UpdatedRoads);
		}
	}
}

void ARoadActor::UpdateCurveBySegments()
{
	if (Length() < SMALL_NUMBER)
	{
		int k = 0;
	}
	for (int i = 1; i < HeightSegments.Num() - 1;)
	{
		if (FMath::IsNearlyEqual(HeightSegments[i].Dist, HeightSegments[i + 1].Dist))
			HeightSegments.RemoveAt(i);
		else
			i++;
	}
	UpdateLanes();
}

void ARoadActor::UpdateLanes()
{
	//Only main road which is direct child should be added to octree
	if (ARoadScene* Scene = Cast<ARoadScene>(GetAttachParentActor()))
		Scene->OctreeRemoveRoad(this);
	double Len = Length();
	for (URoadBoundary* Boundary : Boundaries)
		Boundary->LocalOffsets.Last().Dist = Len;
	BaseCurve->UpdateOffsets(nullptr, nullptr, 0);
	for (int Side = 0; Side < 2; Side++)
	{
		URoadLane* Lane = BaseCurve->GetLane(Side);
		while (Lane)
		{
			Lane->Update(Side);
			Lane = Lane->GetBoundary(Side)->GetLane(Side);
		}
	}
	//Only main road which is direct child should be added to octree
	if (ARoadScene* Scene = Cast<ARoadScene>(GetAttachParentActor()))
		Scene->OctreeAddRoad(this);
}

void ARoadActor::BuildMesh(const TArray<FJunctionSlot>& Slots)
{
	TSet<UActorComponent*> Components = GetComponents();
	for (UActorComponent* Component : Components)
	{
		if (Component->IsA<UInstancedStaticMeshComponent>() || Component->IsA<UDecalComponent>())
			Component->DestroyComponent();
	}
	ForEachAttachedActors([&](AActor* Actor)->bool
	{
		Actor->Destroy();
		return true;
	});
	FRoadActorBuilder Builder;
	if (RoadSegments.Num())
	{
		TArray<URoadLane*> LeftLanes = GetLanes(1);
		TArray<URoadLane*> RightLanes = GetLanes(0);
		for (int i = 0; i <= Slots.Num(); i++)
		{
			double R_Start = i > 0 ? Slots[i - 1].OutputDist() : 0;
			double R_End = i < Slots.Num() ? Slots[i].InputDist() : Length();
			//FPolyline::AddPoint use 1e-8 so use 1e-4 here to avoid 1 point polyline
			if (FMath::IsNearlyEqual(R_Start, R_End, DOUBLE_KINDA_SMALL_NUMBER))
				continue;
			for (int j = LeftLanes.Num() - 1; j >= 0; j--)
				LeftLanes[j]->BuildMesh(Builder, j + 1, R_Start, R_End, Length());
			for (int j = 0; j < RightLanes.Num(); j++)
				RightLanes[j]->BuildMesh(Builder, -j - 1, R_Start, R_End, Length());
			for (URoadBoundary* Boundary : Boundaries)
			{
				for (int j = 0; j < Boundary->Segments.Num(); j++)
				{
					double Start = FMath::Max(R_Start, Boundary->SegmentStart(j));
					double End = FMath::Min(R_End, Boundary->SegmentEnd(j));
					if (Start < End)
						Boundary->BuildMesh(Builder, Start, End, j);
				}
			}
		}
	}
	for (URoadMarking* Marking : Markings)
		Marking->BuildMesh(Builder);
	Builder.MeshBuilder.Build(GetRootComponent());
	Builder.InstanceBuilder.AttachToActor(this);
	Builder.DecalBuilder.AttachToActor(this);
}

bool ARoadActor::IsLink()
{
	return GetAttachParentActor()->IsA<AJunctionActor>();
}

bool ARoadActor::IsRamp()
{
	URoadLane* Lane = BaseCurve->LeftLane;
	return !Lane || Lane->GetSegment(ELaneType::Driving) == INDEX_NONE;
}

FVector2D ARoadActor::GetPos(double Dist)
{
	if (RoadSegments.Num())
	{
		FRoadSegment& Segment = RoadSegments[GetElementIndex(RoadSegments, Dist)];
		return Segment.GetPos(Dist - Segment.Dist);
	}
	return RoadPoints[0].Pos;
}

FVector ARoadActor::GetPos(int PointIndex)
{
	double H = GetHeight(RoadPoints[PointIndex].Dist);
	return FVector(RoadPoints[PointIndex].Pos, H);
}

FVector ARoadActor::GetPos(const FVector2D& UV)
{
	return FVector(GetPos(UV.X), GetHeight(UV.X)) + GetRight(UV.X) * UV.Y;
}

FVector2D ARoadActor::GetDir2D(double Dist)
{
	double R = GetRadian(Dist);
	return FVector2D(FMath::Cos(R), FMath::Sin(R));
}

FVector ARoadActor::GetDir(double Dist)
{
	return FVector(GetDir2D(Dist), 0);
}

FVector ARoadActor::GetRight(double Dist)
{
	double R = GetRadian(Dist);
	return FVector(-FMath::Sin(R), FMath::Cos(R), 0);
}

double ARoadActor::GetRadian(double Dist)
{
	if (RoadSegments.Num())
	{
		int Index = GetElementIndex(RoadSegments, Dist);
		FRoadSegment& Segment = RoadSegments[Index];
		return Segment.GetR(Dist - Segment.Dist);
	}
	return 0;
}

double ARoadActor::GetHeight(double Dist)
{
	if (HeightSegments.Num() > 1)
	{
		int Index = GetPointIndex(HeightSegments, Dist);
		FHeightSegment& Segment = HeightSegments[Index];
		return Segment.Get(Dist - Segment.Dist);
	}
	return HeightSegments[0].Height;
}

ARoadScene* ARoadActor::GetScene()
{
	AActor* Parent = GetAttachParentActor();
	while (Parent &&!Parent->IsA<ARoadScene>())
		Parent = Parent->GetAttachParentActor();
	return static_cast<ARoadScene*>(Parent);
}

AJunctionActor* ARoadActor::GetJunction()
{
	return Cast<AJunctionActor>(GetAttachParentActor());
}

void ARoadActor::ExportXodr(FXmlNode* XmlNode, int& RoadId, int& ObjectId, int JunctionId)
{
	if (!Lanes.Num())
		return;
	ARoadScene* Scene = GetScene();
	TArray<FJunctionSlot> Slots = Scene->GetJunctionSlots(this);
	TArray<URoadLane*> LeftLanes = GetLanes(1);
	TArray<URoadLane*> RightLanes = GetLanes(0);
	bool IsLink = GetAttachParentActor()->IsA<AJunctionActor>();
	if (IsLink)
	{
		Algo::Reverse(LeftLanes);
		RightLanes.Insert(LeftLanes, 0);
	}
	TArray<FXmlNode*> ObjectNodes;
	TArray<FVector2D> RoadRange;
	for (int i = 0; i <= Slots.Num(); i++)
	{
		double R_Start = i > 0 ? Slots[i - 1].OutputDist() : 0;
		double R_End = i < Slots.Num() ? Slots[i].InputDist() : Length();
		//FPolyline::AddPoint use 1e-8 so use 1e-4 here to avoid 1 point polyline
		if (FMath::IsNearlyEqual(R_Start, R_End, DOUBLE_KINDA_SMALL_NUMBER))
			continue;
		FXmlNode* RoadNode = XmlNode_CreateChild(XmlNode, TEXT("road"));
		XmlNode_AddAttribute(RoadNode, TEXT("id"), RoadId++);
		XmlNode_AddAttribute(RoadNode, TEXT("junction"), JunctionId);
		XmlNode_AddAttribute(RoadNode, TEXT("length"), (R_End - R_Start) * 0.01);
		FXmlNode* PlanViewNode = XmlNode_CreateChild(RoadNode, TEXT("planView"));
		TArray<FRoadSegment> CutRoads = CutRoadSegments(R_Start, R_End);
		for (int j = 0; j < CutRoads.Num(); j++)
			CutRoads[j].ExportXodr(PlanViewNode);
		FXmlNode* ElevationProfileNode = XmlNode_CreateChild(RoadNode, TEXT("elevationProfile"));
		TArray<FHeightSegment> CutHeights = CutHeightSegments(R_Start, R_End);
		for (int j = 0; j < CutHeights.Num() - 1; j++)
			CutHeights[j].ExportXodr(ElevationProfileNode);
		FXmlNode* LanesNode = XmlNode_CreateChild(RoadNode, TEXT("lanes"));
		if (IsLink)
		{
			TArray<FCurveOffset> Offsets = RightLanes[0]->LeftBoundary->CutOffsets(R_Start, R_End);
			for (int j = 0; j < Offsets.Num(); j++)
			{
				FCurveOffset& This = Offsets[j];
				FXmlNode* WidthNode = XmlNode_CreateChild(LanesNode, TEXT("laneOffset"));
				XmlNode_AddAttribute(WidthNode, TEXT("s"), This.Dist * 0.01);
				FVector4 ABCD;
				if (j + 1 < Offsets.Num())
				{
					FCurveOffset& Next = Offsets[j + 1];
					double Length = (Next.Dist - This.Dist) * 0.01;
					//positive t direction is LEFT in xodr, so inverse everything
					ABCD = CalcABCD(-This.Offset * 0.01, -This.Dir * Length, -Next.Offset * 0.01, -Next.Dir * Length, Length);
				}
				else
					ABCD = FVector4(-This.Offset * 0.01, 0, 0, 0);
				XmlNode_AddABCD(WidthNode, ABCD);
			}
		}
		TArray<double> Keys = {R_Start, R_End};
		for (URoadLane* Lane : Lanes)
		{
			for (FLaneSegment& Segment : Lane->Segments)
			{
				double Dist = Segment.Dist;
				if (Dist > R_Start && Dist < R_End)
					Keys.Add(Dist);
			}
		}
		Keys.Sort();
		for (int j = 1; j < Keys.Num();)
		{
			if (FMath::IsNearlyEqual(Keys[j], Keys[j - 1], 1.0))
				Keys.RemoveAt(j);
			else
				j++;
		}
		for (int j = 0; j < Keys.Num() - 1; j++)
		{
			double Start = Keys[j];
			double End = Keys[j + 1];
			FXmlNode* SectionNode = XmlNode_CreateChild(LanesNode, TEXT("laneSection"));
			XmlNode_AddAttribute(SectionNode, TEXT("s"), (Start - R_Start) * 0.01);
			auto ExportLane = [&](FXmlNode* XmlNode, URoadLane* Lane, URoadBoundary* Boundary, URoadBoundary* WidthBoundary, int LaneId)
			{
				FXmlNode* LaneNode = XmlNode_CreateChild(XmlNode, TEXT("lane"));
				XmlNode_AddAttribute(LaneNode, TEXT("id"), LaneId);
				if (Lane)
					Lane->ExportXodr(LaneNode, Start, End);
				else
					XmlNode_AddAttribute(LaneNode, TEXT("type"), TEXT("none"));
				Boundary->ExportXodr(LaneNode, Start, End);
				if (WidthBoundary)
					WidthBoundary->ExportWidth(LaneNode, Start, End);
			};
			FXmlNode* LeftNode = XmlNode_CreateChild(SectionNode, TEXT("left"));
			FXmlNode* CenterNode = XmlNode_CreateChild(SectionNode, TEXT("center"));
			FXmlNode* RightNode = XmlNode_CreateChild(SectionNode, TEXT("right"));
			if (IsLink)
			{
				ExportLane(CenterNode, nullptr, RightLanes[0]->LeftBoundary, nullptr, 0);
				for (int k = 0; k < RightLanes.Num(); k++)
					ExportLane(RightNode, RightLanes[k], RightLanes[k]->RightBoundary, k < LeftLanes.Num() ? RightLanes[k]->LeftBoundary : RightLanes[k]->RightBoundary, - k - 1);
			}
			else
			{
				for (int k = LeftLanes.Num() - 1; k >= 0; k--)
					ExportLane(LeftNode, LeftLanes[k], LeftLanes[k]->LeftBoundary, LeftLanes[k]->LeftBoundary, k + 1);
				ExportLane(CenterNode, nullptr, BaseCurve, nullptr, 0);
				for (int k = 0; k < RightLanes.Num(); k++)
					ExportLane(RightNode, RightLanes[k], RightLanes[k]->RightBoundary, RightLanes[k]->RightBoundary, -k - 1);
			}
		}
		ObjectNodes.Add(XmlNode_CreateChild(RoadNode, TEXT("objects")));
		RoadRange.Add(FVector2D(R_Start, R_End));
	}
	auto FindNearestSegment = [&](double Dist)->int
	{
		int MinIndex = -1;
		double MinDist = WORLD_MAX;
		for (int i = 0; i < RoadRange.Num(); i++)
		{
			double MinDiff = Dist - RoadRange[i].X;
			double MaxDiff = Dist - RoadRange[i].Y;
			if (MinDiff >= 0 && MaxDiff <= 0)
				return i;
			double Diff = FMath::Min(FMath::Abs(MinDiff), FMath::Abs(MaxDiff));
			if (MinIndex > Diff)
			{
				MinIndex = Diff;
				MinIndex = i;
			}
		}
		return MinIndex;
	};
	for (URoadMarking* Marking : Markings)
	{
#if 0
		FVector2D Center = Object->Center();
		FVector2D Size = Object->Size();
		FVector Dir = BaseCurve->Curve.GetDir(Center.X);
		double Radian = FMath::Atan2(Dir.Y, Dir.X);
		int Index = FindNearestSegment(Center.X);
		FXmlNode* ObjectNode = XmlNode_CreateChild(ObjectNodes[Index], TEXT("object"));
		XmlNode_AddAttribute(ObjectNode, TEXT("id"), ObjectId++);
		XmlNode_AddAttribute(ObjectNode, TEXT("s"), (Center.X - RoadRange[Index].X) * 0.01);
		XmlNode_AddAttribute(ObjectNode, TEXT("t"), Center.Y * 0.01);
		XmlNode_AddAttribute(ObjectNode, TEXT("hdg"), -Radian);
		XmlNode_AddAttribute(ObjectNode, TEXT("width"), Size.Y * 0.01);
		XmlNode_AddAttribute(ObjectNode, TEXT("length"), Size.X * 0.01);
#endif
	}
}

void ARoadActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

#if WITH_EDITOR
#include "Dialogs/DlgPickAssetPath.h"
void ARoadActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ARoadActor::PreEditUndo()
{
	if (ARoadScene* Scene = Cast<ARoadScene>(GetAttachParentActor()))
		Scene->OctreeRemoveRoad(this);
	AActor::PreEditUndo();
}

void ARoadActor::PostEditUndo()
{
	AActor::PostEditUndo();
	if (IsValid(this))
	{
		if (IsLink())
			UpdateCurveBySegments();
		else
			UpdateCurve();
	}
}

void ARoadActor::CreateStyle()
{
	TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget = SNew(SDlgPickAssetPath).Title(FText::FromString(TEXT("Choose Asset Path")));
	if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
	{
		// Get the full name of where we want to create the physics asset.
		FString PackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
		FString AssetName = PickAssetPathWidget->GetAssetName().ToString();
		UPackage* Package = CreatePackage(*PackageName);
		URoadStyle* Style = NewObject<URoadStyle>(Package, FName(*AssetName), RF_Public | RF_Standalone);
		Style->BaseCurveMark = BaseCurve->Segments[0].LaneMarking;
		Style->bHasGround = bHasGround;
		URoadBoundary* Boundary = BaseCurve;
		while (URoadLane* Lane = Boundary->RightLane)
		{
			Boundary = Lane->RightBoundary;
			FRoadLaneStyle& LaneStyle = Style->RightLanes[Style->RightLanes.AddDefaulted()];
			LaneStyle.Width = Boundary->LocalOffsets[0].Offset;
			LaneStyle.LaneType = Lane->Segments[0].LaneType;
			LaneStyle.LaneShape = Lane->Segments[0].LaneShape;
			LaneStyle.LaneMarking = Boundary->Segments[0].LaneMarking;
			LaneStyle.Props = Boundary->Segments[0].Props;
		}
		Boundary = BaseCurve;
		while (URoadLane* Lane = Boundary->LeftLane)
		{
			Boundary = Lane->LeftBoundary;
			FRoadLaneStyle& LaneStyle = Style->LeftLanes[Style->LeftLanes.AddDefaulted()];
			LaneStyle.Width = Boundary->LocalOffsets[0].Offset;
			LaneStyle.LaneType = Lane->Segments[0].LaneType;
			LaneStyle.LaneShape = Lane->Segments[0].LaneShape;
			LaneStyle.LaneMarking = Boundary->Segments[0].LaneMarking;
			LaneStyle.Props = Boundary->Segments[0].Props;
		}
		Package->SetDirtyFlag(true);
	}
}

void URoadMeshComponent::PostEditUndo()
{
	SetStaticMesh(nullptr);
	UStaticMeshComponent::PostEditUndo();
}
#endif
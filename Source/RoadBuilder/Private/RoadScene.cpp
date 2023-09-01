// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadScene.h"
#include "XmlFile.h"
#include "Engine/Level.h"
#include "Engine/World.h"

void FJunctionLink::CreateRoad(AJunctionActor* Parent)
{
	Road = Parent->GetWorld()->SpawnActor<ARoadActor>();
	Road->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
}

void FJunctionLink::Destroy()
{
	if (Road)
	{
		Road->DeleteAllMarkings();
		Road->Destroy();
		Road = nullptr;
	}
	Radius = 1000;
}

void FJunctionGate::Renew(double D, double S)
{
	InitDist = Dist = D;
	CornerDists[0] = CornerDists[1] = D;
	CutDists[0] = CutDists[1] = D;
	Radian = 0;
	Sign = S;
}

FJunctionGate& FJunctionSlot::InputGate() const
{
	return Junction->Gates[InputGateIndex()];
}

FJunctionGate& FJunctionSlot::OutputGate() const
{
	return Junction->Gates[OutputGateIndex()];
}

int FJunctionSlot::InputGateIndex() const
{
	for (int i = 0; i < Junction->Gates.Num(); i++)
	{
		FJunctionGate& Gate = Junction->Gates[i];
		if (Gate.Road == Road && Gate.Sign < 0 && Gate.InitDist == InitInputDist)
			return i;
	}
	return INDEX_NONE;
}

int FJunctionSlot::OutputGateIndex() const
{
	for (int i = 0; i < Junction->Gates.Num(); i++)
	{
		FJunctionGate& Gate = Junction->Gates[i];
		if (Gate.Road == Road && Gate.Sign > 0 && Gate.InitDist == InitOutputDist)
			return i;
	}
	return INDEX_NONE;
}

double FJunctionSlot::InputDist() const
{
	return HasInput() ? InputGate().Dist : 0;
}

double FJunctionSlot::OutputDist() const
{
	return HasOutput() ? OutputGate().Dist : Road->Length();
}

double FJunctionSlot::CrossDist() const
{
	double Input = FMath::Max(0, InitInputDist);
	double Output = FMath::Min(Road->Length(), InitOutputDist);
	return (Input + Output) / 2;
}

void FJunctionSlot::Combine(FJunctionSlot& Other)
{
	for (FJunctionGate& Gate : Other.Junction->Gates)
	{
		Gate.Clear();
		if (Gate.Road != Road)
		{
			bool AddGate = true;
			for (FJunctionGate& G : Junction->Gates)
			{
				if (G.Road == Gate.Road && G.Sign == Gate.Sign)
				{
					if (G.Sign < 0)
					{
						if (G.Dist > Gate.Dist)
						{
							G = Gate;
							InitInputDist = G.InitDist;
						}
					}
					else
					{
						if (G.Dist < Gate.Dist)
						{
							G = Gate;
							InitOutputDist = G.InitDist;
						}
					}
					AddGate = false;
					break;
				}
			}
			if (AddGate)
				Junction->Gates.Add(Gate);
		}
	}
	ARoadScene* Scene = Junction->GetScene();
	Other.Junction->Destroy();
	Scene->Junctions.Remove(Other.Junction);
}

AJunctionActor::AJunctionActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if USE_PROC_ROAD_MESH
	RootComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RootComponent"));
#else
	RootComponent = CreateDefaultSubobject<URoadMeshComponent>(TEXT("RootComponent"));
#endif
}

void AJunctionActor::AddRoad(ARoadActor* Road, double Dist)
{
	FJunctionSlot Slot = GetSlot(Road, Dist);
//	if (Dist > DefaultJunctionExtent)
	{
		if (Slot.IsValid() && Slot.HasInput())
		{
			FJunctionGate& Input = Slot.InputGate();
			if (Input.IsExpired())
				Input.Renew(Dist, -1);
			else
				Input.InitDist = FMath::Min(Input.InitDist, Dist);
		}
		else
			AddGate(Road, Dist, -1);
	}
//	if (Dist < Road->Length() - DefaultJunctionExtent)
	{
		if (Slot.IsValid() && Slot.HasOutput())
		{
			FJunctionGate& Output = Slot.OutputGate();
			if (Output.IsExpired())
				Output.Renew(Dist, 1);
			else
				Output.InitDist = FMath::Max(Output.InitDist, Dist);
		}
		else
			AddGate(Road, Dist, 1);
	}
}

void AJunctionActor::Update(TOctree2<FRoadOctreeElement, FRoadOctreeSemantics>& Octree)
{
	FVector Center(0, 0, 0);
	ARoadScene* Scene = GetScene();
	for (int i = 0; i < Gates.Num();)
	{
		FJunctionGate& Gate = Gates[i];
		if (Gate.Sign < 0 && Gate.InitDist < DefaultJunctionExtent || Gate.Sign > 0 && Gate.InitDist > Gate.Road->Length() - DefaultJunctionExtent)
		{
			Gate.Clear();
			Gates.RemoveAt(i);
		}
		else
			i++;
	}
	for (FJunctionGate& Gate : Gates)
		Center += Gate.Road->BaseCurve->GetPos(Gate.InitDist) / Gates.Num();
	for (FJunctionGate& Gate : Gates)
	{
		double Offset = Gate.Sign > 0 ? FMath::Min(12800, Gate.Road->Length() - Gate.InitDist) : FMath::Max(-12800, -Gate.InitDist);
		FVector End = Gate.Road->BaseCurve->GetPos(Gate.InitDist + Offset);
		FVector Dir = (End - Center).GetSafeNormal();
		Gate.Radian = FMath::Atan2(Dir.Y, Dir.X);
	}
	Gates.Sort();
	for (int i = 0; i < Gates.Num(); i++)
	{
		FJunctionGate& Gate = Gates[i];
		for (int j = 0; j < Gate.Links.Num(); j++)
		{
			if (j < Gates.Num())
			{
				FJunctionGate& Next = Gates[(i + j) % Gates.Num()];
				if (Gate.Links[j].InputRoad != Gate.Road || Gate.Links[j].OutputRoad != Next.Road)
					Gate.Links[j].Destroy();
			}
			else
				Gate.Links[j].Destroy();
		}
		Gate.Links.SetNum(Gates.Num());
		FJunctionGate& Next = Gates[(i + 1) % Gates.Num()];
		bool SrcRamp = Gate.IsRampOf(Next);
		bool DstRamp = Next.IsRampOf(Gate);
		if (SrcRamp || DstRamp)
		{
			double Dist1, Dist2;
			if (SrcRamp)
			{
				FVector2D UV = Next.Road->GetUV((FMath::IsNearlyZero(Gate.InitDist) ? Gate.Road->BaseCurve->Curve.Points[0] : Gate.Road->BaseCurve->Curve.Points.Last()).Pos);
				Dist1 = Gate.InitDist;
				Dist2 = UV.X;
			}
			else
			{
				FVector2D UV = Gate.Road->GetUV((FMath::IsNearlyZero(Next.InitDist) ? Next.Road->BaseCurve->Curve.Points[0] : Next.Road->BaseCurve->Curve.Points.Last()).Pos);
				Dist1 = UV.X;
				Dist2 = Next.InitDist;
			}
			UpdateCorner(Gate, Dist1, Next, Dist2);
		}
		else
		{
			int SrcSide = Gate.Sign > 0 ? 1 : 0;
			int DstSide = Next.Sign > 0 ? 0 : 1;
			URoadBoundary* SrcBoundary = Gate.Road->GetRoadEdge(SrcSide);
			URoadBoundary* DstBoundary = Next.Road->GetRoadEdge(DstSide);
			FPolyline& SrcCurve = SrcBoundary->Curve;
			FPolyline& DstCurve = DstBoundary->Curve;
#if 0
			for (int j = 0; j < SrcCurve.Points.Num() - 1; j++)
			{
				FBox SrcBox = SrcBoundary->Curve.GetSegmentBounds(j);
				for (int k = 0; k < DstCurve.Points.Num() - 1; k++)
				{
					FBox DstBox = DstBoundary->Curve.GetSegmentBounds(k);
					if (SrcBox.Intersect(DstBox))
					{
						double Seg1, Seg2;
						if (DoLineSegmentsIntersect((const FVector2D&)SrcCurve.Points[j].Pos, (const FVector2D&)SrcCurve.Points[j + 1].Pos, (const FVector2D&)DstCurve.Points[k].Pos, (const FVector2D&)DstCurve.Points[k + 1].Pos, Seg1, Seg2))
						{
							double Dist1 = FMath::Lerp(SrcCurve.Points[j].Dist, SrcCurve.Points[j + 1].Dist, Seg1);
							double Dist2 = FMath::Lerp(DstCurve.Points[k].Dist, DstCurve.Points[k + 1].Dist, Seg2);
							UpdateCorner(Gate, Dist1, Next, Dist2);
							goto _NextGate;
						}
					}
				}
			}
#else
			double EndDist = Gate.Sign > 0 ? (Gate.InitDist + Gate.Road->Length()) / 2 : Gate.InitDist / 2;
			int StartSegment = SrcCurve.GetPoint(Gate.InitDist);
			int EndSegment = SrcCurve.GetPoint(EndDist);
			int Dir = FMath::Sign(EndSegment - StartSegment);
			EndSegment += Dir;
			for (int j = StartSegment; j != EndSegment; j += Dir)
			{
				bool ResultFound = false;
				FRoadOctreeElement SrcSegment(SrcBoundary, j);
				FBox Box = SrcSegment.GetBounds();
				Box.Min.Z -= 1000;
				Box.Max.Z += 1000;
				Octree.FindElementsWithBoundsTest(Box, [&](const FRoadOctreeElement& DstSegment)
				{
					//The two boundaries may be the same one so adjacent checking is still needed
					if (ResultFound || DstSegment.Boundary != DstBoundary || SrcSegment.Adjacent(DstSegment))
						return;
					double Seg1, Seg2;
					if (DoLineSegmentsIntersect((const FVector2D&)SrcCurve.Points[SrcSegment.Index].Pos, (const FVector2D&)SrcCurve.Points[SrcSegment.Index + 1].Pos, (const FVector2D&)DstCurve.Points[DstSegment.Index].Pos, (const FVector2D&)DstCurve.Points[DstSegment.Index + 1].Pos, Seg1, Seg2))
					{
						double Dist1 = FMath::Lerp(SrcCurve.Points[SrcSegment.Index].Dist, SrcCurve.Points[SrcSegment.Index + 1].Dist, Seg1);
						double Dist2 = FMath::Lerp(DstCurve.Points[DstSegment.Index].Dist, DstCurve.Points[DstSegment.Index + 1].Dist, Seg2);
						UpdateCorner(Gate, Dist1, Next, Dist2);
						ResultFound = true;
					}
				});
				if (ResultFound)
					goto _NextGate;
			}
		//	UE_LOG(LogRoadBuilder, Warning, TEXT("Can't solve intersection"));
			Gate.CutDists[SrcSide] = Gate.CornerDists[SrcSide] = -1;
			Next.CutDists[DstSide] = Next.CornerDists[DstSide] = -1;
		_NextGate:;
		}
#endif
	}
	for (FJunctionGate& Gate : Gates)
	{
		for (int i = 0; i < 2; i++)
		{
			if (Gate.CornerDists[i] < 0)
			{
				if (Gate.CornerDists[!i] >= 0)
					Gate.CornerDists[i] = Gate.CornerDists[!i];
				else
					Gate.CornerDists[i] = ((Gate.Sign > 0) ^ i) ? 0 : Gate.Road->Length();
			}
			if (Gate.CutDists[i] < 0)
			{
				if (Gate.CutDists[!i] >= 0)
					Gate.CutDists[i] = Gate.CutDists[!i];
				else
					Gate.CutDists[i] = ((Gate.Sign > 0) ^ i) ? 0 : Gate.Road->Length();
			}
		}
	}
	for (FJunctionGate& Gate : Gates)
	{
		if (Gate.Sign > 0)
			Gate.Dist = FMath::Max(Gate.CutDists[0], Gate.CutDists[1]);
		else
			Gate.Dist = FMath::Min(Gate.CutDists[0], Gate.CutDists[1]);
	}
}

void AJunctionActor::BuildLink(FJunctionGate& Gate, FJunctionGate& Next, int Index)
{
	int SrcSide = Gate.Sign > 0 ? 1 : 0;
	int DstSide = Next.Sign > 0 ? 0 : 1;
	bool SrcRamp = Gate.IsRampOf(Next);
	bool DstRamp = Next.IsRampOf(Gate);
	double SrcCorner, DstCorner;
	URoadBoundary *SrcBoundary, *DstBoundary;
	bool SkipSidewalks = Index != CornerIndex;
	uint32 LeftLaneMarkingMask = 0;
	uint32 RightLaneMarkingMask = 0;
	FJunctionLink& Link = Gate.Links[Index];
	Link.InputRoad = Gate.Road;
	Link.OutputRoad = Next.Road;
	TArray<ELaneType> DrivingLaneTypes = { ELaneType::Driving, ELaneType::Shoulder };
	if (Index == CornerIndex)
	{
		SrcCorner = Gate.CutDists[SrcSide];
		DstCorner = Next.CutDists[DstSide];
		SrcBoundary = Gate.Road->GetRoadEdge(SrcSide);
		DstBoundary = Next.Road->GetRoadEdge(DstSide);
		if (SrcRamp)
			LeftLaneMarkingMask = (1 << (Gate.Road->GetLanes(SrcBoundary, !SrcSide, DrivingLaneTypes).Num() + 1)) - 1;
		else if (DstRamp)
			LeftLaneMarkingMask = (1 << (Next.Road->GetLanes(DstBoundary, !DstSide, DrivingLaneTypes).Num() + 1)) - 1;
		else if (Gate.Road == Next.Road || Gate.IsConnected(Next))
			LeftLaneMarkingMask = (1 << (Gate.Road->GetLanes(SrcBoundary, !SrcSide, DrivingLaneTypes).Num() + 1)) - 1;
		else
			LeftLaneMarkingMask = 1;
	}
	else if (Index == 0)
	{
		URoadLane* TurnLane = Next.Road->BaseCurve->GetLane(!SrcSide);
		if (!TurnLane)
		{
			Link.Destroy();
			return;
		}
		SrcCorner = Gate.Dist;
		DstCorner = Next.Dist;
		SrcBoundary = Gate.Road->BaseCurve;
		DstBoundary = TurnLane->GetBoundary(!SrcSide);
	}
	else
	{
		SrcCorner = SrcRamp ? (Gate.Sign > 0 ? 0 : Gate.Road->Length()) : Gate.Dist;
		DstCorner = DstRamp ? (Next.Sign > 0 ? 0 : Next.Road->Length()) : Next.Dist;
		SrcBoundary = Gate.Road->BaseCurve;
		DstBoundary = Next.Road->BaseCurve;
		if (Gate.Road == Next.Road || Gate.IsConnected(Next) || SrcRamp || DstRamp)
		{
			int Idx = ((&Next - Gates.GetData()) - 1 + Gates.Num()) % Gates.Num();
			if (Gates[Idx].IsRampOf(Next) || Gates[Idx].IsRampOf(Gate) || SrcRamp || DstRamp)
			{
				TArray<URoadLane*> SrcLanes = Gate.Road->GetLanes(SrcBoundary, SrcSide, DrivingLaneTypes);
				TArray<URoadLane*> DstLanes = Next.Road->GetLanes(DstBoundary, DstSide, DrivingLaneTypes);
				RightLaneMarkingMask = (1 << (FMath::Min(SrcLanes.Num(), DstLanes.Num()) - 1)) - 1;
			}
		}
	}
	if (!Link.Road)
	{
		URoadStyle* Style = URoadStyle::Create(SrcBoundary, SrcSide, DstBoundary, DstSide, SkipSidewalks, true, LeftLaneMarkingMask, RightLaneMarkingMask);
		if (Style->NumDrivingLanes() > 0 || Index == CornerIndex)
		{
			Link.CreateRoad(this);
			Link.Road->InitWithStyle(Style);
		}
	}
	else
		Link.Road->ClearSegments();
	if (Link.Road)
	{
		if (SrcBoundary == DstBoundary)
		{
			if (Gate.Dist != Next.Dist)
				Link.Road->AddSubRoad(SrcBoundary, Gate.Dist, Next.Dist);
			else
				Link.Destroy();
		}
		else
		{
			if (!FMath::IsNearlyEqual(Gate.Dist, SrcCorner))
				Link.Road->AddSubRoad(SrcBoundary, Gate.Dist, SrcCorner, SrcRamp);
			FVector2D SrcPos(SrcBoundary->GetPos(SrcCorner));
			FVector2D SrcDir(SrcBoundary->GetDir(SrcCorner) * (-Gate.Sign));
			FVector2D DstPos(DstBoundary->GetPos(DstCorner));
			FVector2D DstDir(DstBoundary->GetDir(DstCorner) * (Next.Sign));
			if (!SrcPos.Equals(DstPos) && !SrcRamp && !DstRamp)
				Link.Road->AddArcs(SrcPos, SrcDir, DstPos, DstDir, Link.Radius, Gate.Road->GetHeight(SrcCorner), Next.Road->GetHeight(DstCorner));
			if (!FMath::IsNearlyEqual(Next.Dist, DstCorner))
				Link.Road->AddSubRoad(DstBoundary, DstCorner, Next.Dist, DstRamp);
			if (!Link.Road->RoadSegments.Num())
				Link.Destroy();
		}
	}
	if (Link.Road)
		Link.Road->UpdateCurveBySegments();
}

void AJunctionActor::Build()
{
	for (int i = 0; i < Gates.Num(); i++)
	{
		FJunctionGate& Gate = Gates[i];
		for (int j = 0; j < Gates.Num(); j++)
		{
			FJunctionGate& Next = Gates[(i + j) % Gates.Num()];
			BuildLink(Gate, Next, j);
		}
	}
	BuildGoreMarkings();
	FRoadMesh Builder;
	TArray<FVector> Points;
	TArray<FVector> CornerPoints;
	TMap<ULaneShape*, int> Shapes;
	for (int i = 0; i < Gates.Num(); i++)
	{
		FJunctionGate& Gate = Gates[i];
		if (Gate.Links[1].Road)
		{
			FPolyline& Corner = Gate.Links[1].Road->BaseCurve->Curve;
			for (FPolyPoint& Point : Corner.Points)
				Points.Add(Point.Pos);
			CornerPoints.Add(Corner.Points[0].Pos);
			CornerPoints.Add(Corner.Points.Last().Pos);
		}
		for (int j = 0; j < 2; j++)
		{
			TArray<URoadLane*> Lanes = Gate.Road->GetLanes(j, { ELaneType::Driving });
			for (URoadLane* Lane : Lanes)
				if (ULaneShape* Shape = Gate.Sign > 0 ? Lane->Segments[0].LaneShape : Lane->Segments.Last().LaneShape)
					Shapes.FindOrAdd(Shape)++;
		}
	}
	if (!FMath::IsNearlyZero(CalcOBB(CornerPoints).GetSize().Y))
	{
		Shapes.ValueSort([](int A, int B)->bool {return A > B; });
		ULaneShape* Shape = TMap<ULaneShape*, int>::TIterator(Shapes)->Key;
		Builder.AddPolygon(Shape->GetSurfaceMaterial(), Shape->GetBackfaceMaterial(), Points);
		Builder.Build(GetRootComponent());
		//Markings depend on junction Mesh so build later
		for (FJunctionGate& Gate : Gates)
		{
			for (FJunctionLink& Link : Gate.Links)
				if (Link.Road)
					Link.Road->BuildMesh(TArray<FJunctionSlot>());
		}
	}
}

void AJunctionActor::BuildGoreMarkings()
{
	DebugCurves.Empty();
	for (int i = 0; i < Gates.Num(); i++)
	{
		int j = (i + 1) % Gates.Num();
		FJunctionGate& Gate = Gates[i];
		FJunctionGate& Next = Gates[j];
		if (Gate.IsRampOf(Next) || Next.IsRampOf(Gate))
			continue;
		int SrcConn = GetRampConnection(Gate);
		int DstConn = GetRampConnection(Next);
		if (SrcConn != INDEX_NONE || DstConn != INDEX_NONE)
		{
			int k = SrcConn != INDEX_NONE ? SrcConn : DstConn;
			double Sign = SrcConn != INDEX_NONE ? Gate.Sign : Next.Sign;
			FJunctionGate& Conn = Gates[k];
			FJunctionLink& Corner = Gate.Links[1];
			TArray<URoadBoundary*> CornerBoundaries = Corner.Road->GetBoundaries(1, { ELaneMarkType::Solid });
			FJunctionLink& SrcLink = Sign > 0 ? Gates[k].Links[(i - k + Gates.Num()) % Gates.Num()] : Gate.Links[(k - i + Gates.Num()) % Gates.Num()];
			FJunctionLink& DstLink = Sign > 0 ? Gates[k].Links[(j - k + Gates.Num()) % Gates.Num()] : Next.Links[(k - j + Gates.Num()) % Gates.Num()];
			if (!SrcLink.Road || !DstLink.Road)
				continue;
			TArray<URoadBoundary*> SrcBoundaries = SrcLink.Road->GetBoundaries(Sign > 0 ? 1 : 0, { ELaneMarkType::Solid });
			TArray<URoadBoundary*> DstBoundaries = DstLink.Road->GetBoundaries(Sign > 0 ? 0 : 1, { ELaneMarkType::Solid });
			if (CornerBoundaries.Num() && SrcBoundaries.Num() && DstBoundaries.Num())
			{
				URoadBoundary* CornerBoundary = CornerBoundaries.Last();
				URoadBoundary* SrcBoundary = SrcBoundaries.Last();
				URoadBoundary* DstBoundary = DstBoundaries.Last();
				DebugCurves.Add(SrcBoundary->Curve);
				DebugCurves.Add(DstBoundary->Curve);
				double Dist1, Dist2;
				if (SrcBoundary->Curve.SolveIntersection(DstBoundary->Curve, Dist1, Dist2))
				{
					TArray<FVector2D> Points;
					ARoadActor* MarkingRoad = SrcConn != INDEX_NONE ? DstBoundary->GetRoad() : SrcBoundary->GetRoad();
					auto AddPoints = [&](const FPolyline& Curve)
					{
						for (int i = 0; i < Curve.Points.Num() - 1; i++)
						{
							FVector2D UV = MarkingRoad->GetUV(Curve.Points[i].Pos);
							if (IsUVValid(UV))
								Points.Add(UV);
						}
					};
					int NoneIndex = Sign > 0 ? 0 : 1;
					if (SrcBoundary->Segments.Num() < 2)
					{
						SrcBoundary->AddSegment(Dist1);
						SrcBoundary->Segments[NoneIndex].LaneMarking = SrcConn == INDEX_NONE ? GetMutableDefault<USettings_Global>()->DefaultDashStyle.LoadSynchronous() : nullptr;
					}
					else
						SrcBoundary->Segments[1].Dist = Dist1;
					double Start = Sign > 0 ? SrcBoundary->SegmentStart(!NoneIndex) : SrcBoundary->SegmentEnd(!NoneIndex);
					double End = Sign > 0 ? SrcBoundary->SegmentEnd(!NoneIndex) : SrcBoundary->SegmentStart(!NoneIndex);
					FPolyline SrcCurve = SrcBoundary->Curve.SubCurve(Start, End);
					AddPoints(SrcCurve);
					AddPoints(CornerBoundary->Curve);
					if (DstBoundary->Segments.Num() < 2)
					{
						DstBoundary->AddSegment(Dist2);
						DstBoundary->Segments[NoneIndex].LaneMarking = DstConn == INDEX_NONE ? GetMutableDefault<USettings_Global>()->DefaultDashStyle.LoadSynchronous() : nullptr;
					}
					else
						DstBoundary->Segments[1].Dist = Dist2;
					Start = Sign > 0 ? DstBoundary->SegmentEnd(!NoneIndex) : DstBoundary->SegmentStart(!NoneIndex);
					End = Sign > 0 ? DstBoundary->SegmentStart(!NoneIndex) : DstBoundary->SegmentEnd(!NoneIndex);
					FPolyline DstCurve = DstBoundary->Curve.SubCurve(Start, End);
					AddPoints(DstCurve);
					UPolygonMarkStyle* FillStyle = GetMutableDefault<USettings_Global>()->DefaultGoreMarking.LoadSynchronous();
					UMarkingCurve* Marking = MarkingRoad->GetMarkingCurve(FillStyle);
					if (!Marking)
					{
						Marking = MarkingRoad->AddMarkingCurve(true);
						Marking->FillStyle = FillStyle;
					}
					Marking->SetPoints(Points);
					FVector SrcDir = ((SrcCurve.Points.Last().Pos - SrcCurve.Points[0].Pos).GetSafeNormal() + (SrcCurve.Points[1].Pos - SrcCurve.Points[0].Pos).GetSafeNormal()).GetSafeNormal();
					FVector DstDir = ((DstCurve.Points[0].Pos - DstCurve.Points.Last().Pos).GetSafeNormal() + (DstCurve.Points[DstCurve.Points.Num() - 2].Pos - DstCurve.Points.Last().Pos).GetSafeNormal()).GetSafeNormal();
					FVector Dir = (SrcDir + DstDir).GetSafeNormal();
					Marking->Orientation = FMath::RadiansToDegrees(FMath::Atan2(-Dir.X, Dir.Y));
				}
			}
		}
	}
}

bool AJunctionActor::Contains(ARoadActor* Road, double Dist)
{
	return GetSlot(Road, Dist).IsValid();
}

void AJunctionActor::FixHeight(FPolyline& Polyline)
{
	FVector Delta(0, 0, 10000);
	UStaticMeshComponent* MC = Cast<UStaticMeshComponent>(RootComponent);
	for (FPolyPoint& Point : Polyline.Points)
	{
		FHitResult Hit;
		if (MC->LineTraceComponent(Hit, Point.Pos + Delta, Point.Pos - Delta, FCollisionQueryParams()))
			Point.Pos = Hit.Location;
	}
}

void AJunctionActor::FixHeight(TArray<FVector>& Points)
{
	FVector Delta(0, 0, 10000);
	UStaticMeshComponent* MC = Cast<UStaticMeshComponent>(RootComponent);
	for (FVector& Point : Points)
	{
		FHitResult Hit;
		if (MC->LineTraceComponent(Hit, Point + Delta, Point - Delta, FCollisionQueryParams()))
			Point = Hit.Location;
	}
}

void AJunctionActor::UpdateCorner(FJunctionGate& SrcGate, double SrcDist, FJunctionGate& DstGate, double DstDist)
{
	FJunctionLink& Corner = SrcGate.Links[CornerIndex];
	int SrcSide = SrcGate.Sign > 0 ? 1 : 0;
	int DstSide = DstGate.Sign > 0 ? 0 : 1;
	SrcGate.CornerDists[SrcSide] = SrcDist;
	DstGate.CornerDists[DstSide] = DstDist;
	bool SrcRamp = SrcGate.IsRampOf(DstGate);
	bool DstRamp = DstGate.IsRampOf(SrcGate);
	if (SrcRamp || DstRamp)
	{
		Corner.Radius = 0;
		SrcGate.CutDists[SrcSide] = SrcDist;
		DstGate.CutDists[DstSide] = DstDist;
		return;
	}
	URoadBoundary* SrcBoundary = SrcGate.Road->GetRoadEdge(SrcSide);
	URoadBoundary* DstBoundary = DstGate.Road->GetRoadEdge(DstSide);
	double MinSize = 0;
	double MaxSize = FMath::Min(FMath::Min(SrcGate.Sign > 0 ? SrcBoundary->Length() - SrcDist : SrcDist, DstGate.Sign > 0 ? DstBoundary->Length() - DstDist : DstDist), 12800);
	double Size = (MinSize + MaxSize) / 2;
	double R = 0;
	for (int i = 0; i < 100; i++)
	{
		SrcGate.CutDists[SrcSide] = SrcDist + Size * SrcGate.Sign;
		DstGate.CutDists[DstSide] = DstDist + Size * DstGate.Sign;
		FVector SrcPos = SrcBoundary->GetPos(SrcGate.CutDists[SrcSide]);
		FVector SrcDir = SrcBoundary->GetDir(SrcGate.CutDists[SrcSide]) * (-SrcGate.Sign);
		FVector DstPos = DstBoundary->GetPos(DstGate.CutDists[DstSide]);
		FVector DstDir = DstBoundary->GetDir(DstGate.CutDists[DstSide]) * (DstGate.Sign);
		double Dist1, Dist2;
		if (DoLinesIntersect((const FVector2D&)SrcPos, (const FVector2D&)SrcDir, (const FVector2D&)DstPos, -(const FVector2D&)DstDir, Dist1, Dist2))
		{
			double Dist = FMath::Min(Dist1, Dist2);
			double Diff = WrapRadian(FMath::Atan2(DstDir.Y, DstDir.X) - FMath::Atan2(SrcDir.Y, SrcDir.X));
			double Tan = FMath::Abs(FMath::Tan(Diff / 2));
			if (FMath::IsNearlyZero(Tan))
				return;
			R = Dist / Tan;
			if (FMath::IsNearlyEqual(R, Corner.Radius))
				break;
			if (R < Corner.Radius)
			{
				MinSize = Size;
				Size = (MinSize + MaxSize) / 2;
			}
			else
			{
				MaxSize = Size;
				Size = (MinSize + MaxSize) / 2;
			}
		}
		else
		{
			MaxSize = Size;
			Size = (MinSize + MaxSize) / 2;
		}
	}
	Corner.Radius = R;
}

FJunctionGate& AJunctionActor::AddGate(ARoadActor* Road, double Dist, double Sign)
{
	FJunctionGate& Gate = Gates[Gates.AddDefaulted()];
	Gate.Road = Road;
	Gate.InitDist = Gate.Dist = Dist;
	Gate.CornerDists[0] = Gate.CornerDists[1] = Dist;
	Gate.CutDists[0] = Gate.CutDists[1] = Dist;
	Gate.Radian = 0;
	Gate.Sign = Sign;
	return Gate;
}

int AJunctionActor::GetGate(const FVector& Pos)
{
	for (int i = 0; i < Gates.Num(); i++)
	{
		FVector2D UV = Gates[i].Road->GetUV(Pos);
		if (FMath::IsNearlyEqual(Gates[i].Dist, UV.X))
			return i;
	}
	return INDEX_NONE;
}

int AJunctionActor::GetRampConnection(FJunctionGate& Gate)
{
	ARoadActor* Parent = nullptr;
	double Sign = 1;
	if (Gate.Sign > 0)
	{
		if (Gate.Road->ConnectedParents[0] && GetScene()->GetJunctionSlots(Gate.Road)[0].Junction == this)
		{
			if (Gate.Road->ConnectedParents[0]->ConnectedParents[1] != Gate.Road)
			{
				Parent = Gate.Road->ConnectedParents[0];
				Sign = Parent->GetConnectedChild(Gate.Road, 0).ConnectionSign(Parent);
			}
		}
	}
	else
	{
		if (Gate.Road->ConnectedParents[1] && GetScene()->GetJunctionSlots(Gate.Road).Last().Junction == this)
		{
			if (Gate.Road->ConnectedParents[1]->ConnectedParents[0] != Gate.Road)
			{
				Parent = Gate.Road->ConnectedParents[1];
				Sign = Parent->GetConnectedChild(Gate.Road, 1).ConnectionSign(Parent);
			}
		}
	}
	if (Parent)
	{
		for (int i = 0; i < Gates.Num(); i++)
		{
			FJunctionGate& G = Gates[i];
			if (G.Road == Parent && G.Sign * Gate.Sign * Sign < 0)
				return i;
		}
	}
	return INDEX_NONE;
}

FJunctionSlot AJunctionActor::GetSlot(ARoadActor* Road, double Dist)
{
	const double MaxEditingDelta = 1600.0;
	TArray<FJunctionSlot> Slots = GetSlots(Road);
	for (FJunctionSlot& Slot : Slots)
		if (Slot.InputDist() - MaxEditingDelta <= Dist && Slot.OutputDist() + MaxEditingDelta >= Dist)
			return Slot;
	return FJunctionSlot();
}

TArray<FJunctionSlot> AJunctionActor::GetSlots(ARoadActor* Road)
{
	TArray<FJunctionSlot> Slots;
	TArray<FJunctionGate*> Sorts;
	for (FJunctionGate& Gate : Gates)
		if (Gate.Road == Road)
			Sorts.Add(&Gate);
	Sorts.Sort([](const FJunctionGate& A, const FJunctionGate& B)
	{
		return A.Dist < B.Dist;
	});
	for (int i = 0; i < Sorts.Num();)
	{
		if (i + 1 < Sorts.Num())
		{
			if (Sorts[i]->IsInput() && Sorts[i + 1]->IsOutput())
				Slots.Add({ this, Road, Sorts[i]->InitDist, Sorts[i + 1]->InitDist });
			else
				Slots.Add({ this, Road, Sorts[i + 1]->InitDist, Sorts[i]->InitDist });
			i += 2;
			continue;
		}
		if (Sorts[i]->IsInput())
			Slots.Add({ this, Road, Sorts[i]->InitDist, MAX_dbl });
		else
			Slots.Add({ this, Road, -MAX_dbl, Sorts[i]->InitDist });
		i++;
	}
	return MoveTemp(Slots);
}

ARoadScene* AJunctionActor::GetScene()
{
	return Cast<ARoadScene>(GetAttachParentActor());
}

void AJunctionActor::ExportXodr(FXmlNode* XmlNode, int& RoadId, int& ObjectId)
{
	int JunctionId = RoadId++;
	FXmlNode* JunctionNode = XmlNode_CreateChild(XmlNode, TEXT("junction"));
	XmlNode_AddAttribute(JunctionNode, TEXT("id"), JunctionId);
	for (FJunctionGate& Gate : Gates)
		for (FJunctionLink& Link : Gate.Links)
			if (Link.Road)
				Link.Road->ExportXodr(XmlNode, RoadId, ObjectId, JunctionId);
}

void AJunctionActor::Destroyed()
{
	for (FJunctionGate& Gate : Gates)
		Gate.Clear();
	AActor::Destroyed();
}

ARoadScene::ARoadScene(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	Octree = TOctree2<FRoadOctreeElement, FRoadOctreeSemantics>(FVector::ZeroVector, HALF_WORLD_MAX);
}

ARoadActor* ARoadScene::AddRoad()
{
	ARoadActor* Road = GetWorld()->SpawnActor<ARoadActor>();
	Road->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Roads.Add(Road);
	return Road;
}

ARoadActor* ARoadScene::AddRoad(URoadStyle* Style, double Height)
{
	ARoadActor* Road = AddRoad();
	Road->InitWithStyle(Style, Height);
	return Road;
}

ARoadActor* ARoadScene::DuplicateRoad(ARoadActor* Source)
{
	ULevel* Level = GetWorld()->GetCurrentLevel();
	ARoadActor* Road = CastChecked<ARoadActor>(StaticDuplicateObject(Source, Level));
	Road->RegisterAllComponents();
	Road->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Roads.Add(Road);
#if WITH_EDITOR
	Level->AddLoadedActor(Road);
	GEditor->BroadcastLevelActorAdded(Road);
#endif
	return Road;
}

ARoadActor* ARoadScene::PickRoad(const FVector& Pos, ARoadActor* IgnoredRoad)
{
	double BaseOffset = 0;
	FVector2D BestUV(0, MAX_dbl);
	ARoadActor* Result = nullptr;
	Octree.FindElementsWithBoundsTest(FBoxCenterAndExtent(Pos, FVector(DefaultJunctionExtent, DefaultJunctionExtent, DefaultJunctionExtent)), [&](const FRoadOctreeElement& Element)
	{
		ARoadActor* Road = Element.Boundary->GetRoad();
		if (Road != IgnoredRoad)
		{
			FVector2D UV = Element.Boundary->Curve.GetUV((const FVector2D&)Pos, Element.Index);
			if (UV.X >= 0 && UV.X <= Road->Length() && FMath::Abs(BestUV.Y) > FMath::Abs(UV.Y))
			{
				BestUV = UV;
				Result = Road;
				BaseOffset = Element.Boundary->GetOffset(UV.X);
			}
		}
	});
	if (Result && Result->GetRoadEdge(0)->GetOffset(BestUV.X) >= (BaseOffset + BestUV.Y) && Result->GetRoadEdge(1)->GetOffset(BestUV.X) <= (BaseOffset + BestUV.Y))
		return Result;
	return nullptr;
}

AGroundActor* ARoadScene::AddGround(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots, const TArray<FGroundPoint>& Points)
{
	for (AGroundActor* Ground : Grounds)
	{
		if (Ground->Contains(RoadSlots, Points))
		{
			Ground->Renew();
			return Ground;
		}
	}
	AGroundActor* Ground = GetWorld()->SpawnActor<AGroundActor>();
	Ground->Points = Points;
	Ground->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Grounds.Add(Ground);
	return Ground;
}

AJunctionActor* ARoadScene::AddJunction(ARoadActor* R0, double D0, ARoadActor* R1, double D1)
{
	for (AJunctionActor* Junction : Junctions)
	{
		bool C0 = Junction->Contains(R0, D0);
		bool C1 = Junction->Contains(R1, D1);
		if (C0 && C1)
		{
			Junction->AddRoad(R0, D0);
			Junction->AddRoad(R1, D1);
			return Junction;
		}
	}
	AJunctionActor* Junction = GetWorld()->SpawnActor<AJunctionActor>();
	Junction->AddRoad(R0, D0);
	Junction->AddRoad(R1, D1);
	Junction->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Junctions.Add(Junction);
	return Junction;
}

#if 0
UMarkingCurve* ARoadScene::GetMarkingCurve(TArray<FCurveCoordinate>& Coordinates)
{
	for (URoadMarking* Marking : Markings)
	{
		if (UMarkingCurve* MarkingCurve = Cast<UMarkingCurve>(Marking))
		{
			if (MarkingCurve->Match(Coordinates))
				return MarkingCurve;
		}
	}
	return nullptr;
}

TArray<URoadMarking*> ARoadScene::GetMarkings(ARoadActor* Road)
{
	TArray<URoadMarking*> Results;
	for (URoadMarking* Marking : Markings)
	{
		if (Marking->GetRoad() == Road)
			Results.Add(Marking);
	}
	return MoveTemp(Results);
}
#endif
TMap<ARoadActor*, TArray<FJunctionSlot>> ARoadScene::GetAllJunctionSlots()
{
	TMap<ARoadActor*, TArray<FJunctionSlot>> RoadSlots;
	for (ARoadActor* Road : Roads)
		RoadSlots.Add(Road, GetJunctionSlots(Road));
	return MoveTemp(RoadSlots);
}

TArray<FJunctionSlot> ARoadScene::GetJunctionSlots(ARoadActor* Road)
{
	TArray<FJunctionSlot> Results;
	for (AJunctionActor* Junction : Junctions)
		Results.Append(Junction->GetSlots(Road));
	Results.Sort();
	return MoveTemp(Results);
}
/*
FVector2D ARoadScene::GetRoadUV(ARoadActor* SelectedRoad, const FVector& Pos)
{
	double MinDist = MAX_FLT;
	double BestU = 0;
	Octree.FindElementsWithBoundsTest(FBoxCenterAndExtent(Pos, FVector(DefaultJunctionExtent, DefaultJunctionExtent, 10000)), [&](const FRoadOctreeElement& Element)
	{
		ARoadActor* Road = Element.Boundary->GetRoad();
		if (SelectedRoad == Road && Element.Boundary == Road->BaseCurve)
			Element.Boundary->Curve.GetUV((const FVector2D&)Pos, Element.Index, MinDist, BestU);
	});
	return FVector2D(BestU, MinDist);
}
*/
void ARoadScene::Rebuild()
{
	for (AJunctionActor* Junction : Junctions)
	{
		Junction->Modify();
		for (FJunctionGate& Gate : Junction->Gates)
			Gate.MarkExpired();
	}
	for (ARoadActor* Road : Roads)
	{
		if (GetMutableDefault<USettings_Global>()->BuildJunctions)
		{
			for (int i = 0; i < Road->BaseCurve->Curve.Points.Num() - 1; i++)
			{
				FRoadOctreeElement Segment(Road->BaseCurve, i);
				FPolyline& C = Road->BaseCurve->Curve;
				FVector StartPos = C.Points[i].Pos;
				double StartDist = C.Points[i].Dist;
				FVector EndPos = C.Points[i + 1].Pos;
				double EndDist = C.Points[i + 1].Dist;
				Octree.FindElementsWithBoundsTest(Segment.GetBounds(), [&](const FRoadOctreeElement& Element)
				{
					if (Element.IsBoundary() || Element.Adjacent(Road->BaseCurve, i))
						return;
					double Seg1, Seg2;
					FPolyline& Curve = Element.Boundary->Curve;
					if (DoLineSegmentsIntersect((const FVector2D&)StartPos, (const FVector2D&)EndPos, (const FVector2D&)Curve.Points[Element.Index].Pos, (const FVector2D&)Curve.Points[Element.Index + 1].Pos, Seg1, Seg2))
					{
						double Dist1 = FMath::Lerp(StartDist, EndDist, Seg1);
						double Dist2 = FMath::Lerp(Curve.Points[Element.Index].Dist, Curve.Points[Element.Index + 1].Dist, Seg2);
						AddJunction(Road, Dist1, Element.Boundary->GetRoad(), Dist2);
					}
				});
			}
		}
		for (int i = 0; i < 2; i++)
		{
			if (ARoadActor* Parent = Road->ConnectedParents[i])
			{
				FConnectInfo& Info = Parent->GetConnectedChild(Road, i);
				AddJunction(Parent, Info.UV.X, Road, i ? Road->Length() : 0);
			}
		}
	}
	for (int i = 0; i < Junctions.Num();)
	{
		AJunctionActor* Junction = Junctions[i];
		TSet<ARoadActor*> Starts, Ends;
		for (FJunctionGate& Gate : Junction->Gates)
		{
			if (FMath::IsNearlyZero(Gate.Dist))
				Starts.Add(Gate.Road);
			if (FMath::IsNearlyEqual(Gate.Dist, Gate.Road->Length()))
				Ends.Add(Gate.Road);
		}
		TSet<ARoadActor*> RemoveRoads = Starts.Intersect(Ends);
		for (int j = 0; j < Junction->Gates.Num();)
		{
			if (RemoveRoads.Contains(Junction->Gates[j].Road))
			{
				Junction->Gates[j].Clear();
				Junction->Gates.RemoveAt(j);
			}
			else if (Junction->Gates[j].IsExpired())
			{
				/*
				for (int k = 0; k < Junction->Gates.Num(); k++)
				{
					if (k != j && Junction->Gates[k].Links.Num())
					{
						Junction->Gates[k].Links[j].Destroy();
						Junction->Gates[k].Links.RemoveAt(j);
					}
				}*/
				Junction->Gates[j].Clear();
				Junction->Gates.RemoveAt(j);
			}
			else
				j++;
		}
		if (!Junction->Gates.Num())
		{
			Junction->Destroy();
			Junctions.RemoveAt(i);
		}
		else
			i++;
	}
	ForEachAttachedActors([&](AActor* Actor)->bool
	{
		if (Actor->IsA<AJunctionActor>())
		{
			if (!Junctions.Contains(Actor))
				Actor->Destroy();
		}
		if (Actor->IsA<ARoadActor>())
		{
			if (!Roads.Contains(Actor))
				Actor->Destroy();
		}
		return true;
	});
	TMap<ARoadActor*, TArray<FJunctionSlot>> RoadSlots;
	auto ReplaceJunctionWith = [&](AJunctionActor* Junction, AJunctionActor* With = nullptr, ARoadActor* CurrentRoad = nullptr)
	{
		for (auto& KV : RoadSlots)
		{
			if (KV.Key == CurrentRoad)
				continue;
			TArray<FJunctionSlot>& Slots = KV.Value;
			for (int i = 0; i < Slots.Num();)
			{
				if (Slots[i].Junction == Junction)
				{
					if (With)
					{
						Slots[i].Junction = With;
						i++;
					}
					else
						Slots.RemoveAt(i);
				}
				else
					i++;
			}
		}
	};
	while (true)
	{
		RoadSlots = GetAllJunctionSlots();
		for (AJunctionActor* Junction : Junctions)
			Junction->Update(Octree);
		bool ReSolve = false;
		for (auto& Pair : RoadSlots)
		{
			TArray<FJunctionSlot>& Slots = Pair.Value;
			for (int i = 1; i < Slots.Num();)
			{
				if (Slots[i - 1].Junction == Slots[i].Junction)
					Slots.RemoveAt(i);
				else
				{
					double InputDist = Slots[i].InputDist();
					double OutputDist = Slots[i - 1].OutputDist();
					if (OutputDist >= InputDist)
					{
						ReplaceJunctionWith(Slots[i].Junction, Slots[i - 1].Junction, Pair.Key);
						Slots[i - 1].Combine(Slots[i]);
						Slots.RemoveAt(i);
						ReSolve = true;
					}
					else
						i++;
				}
			}
		}
		if (!ReSolve)
			break;
	}
	for (int i = 0; i < Junctions.Num();)
	{
		AJunctionActor* Junction = Junctions[i];
		if (Junction->Gates.Num() < 3)
		{
			ReplaceJunctionWith(Junction);
			Junctions[i]->Destroy();
			Junctions.RemoveAt(i);
		}
		else
			i++;
	}
	for (AJunctionActor* Junction : Junctions)
		Junction->Build();
	for (ARoadActor* Road : Roads)
		Road->BuildMesh(RoadSlots[Road]);
	GenerateGrounds(RoadSlots);
	for (AGroundActor* Ground : Grounds)
	{
		Ground->BuildMesh(RoadSlots);
	}
}

void ARoadScene::GenerateGrounds(TMap<ARoadActor*, TArray<FJunctionSlot>>& RoadSlots)
{
	TSet<FGroundPoint> VisitedGroundPoints;
	TMap<ARoadActor*, int> PrevSlots;
	//Check slot count match
	for (AGroundActor* Ground : Grounds)
	{
		for (FGroundPoint& Point : Ground->Points)
		{
			if (Point.Road)
			{
				int Slot = Point.Index / 2;
				if (int* SlotPtr = PrevSlots.Find(Point.Road))
					*SlotPtr = FMath::Max(*SlotPtr, Slot);
				else
					PrevSlots.Add(Point.Road, Slot);
			}
		}
	}
	for (int i = 0; i < Grounds.Num();)
	{
		bool Delete = false;
		AGroundActor* Ground = Grounds[i];
		for (FGroundPoint& Point : Ground->Points)
		{
			if (Point.Road && (!RoadSlots.Contains(Point.Road) || PrevSlots[Point.Road] > RoadSlots[Point.Road].Num()))
			{
				Delete = true;
				break;
			}
		}
		if (Delete)
		{
			Ground->Destroy();
			Grounds.RemoveAt(i);
		}
		else
		{
			Ground->Modify();
			Ground->MarkExpired();
			i++;
		}
	}
	for (ARoadActor* Road : Roads)
	{
		if (!Road->bHasGround || FMath::IsNearlyZero(Road->Length()))
			continue;
		TArray<FJunctionSlot>& Slots = RoadSlots[Road];
		for (int Side = 0; Side < 2; Side++)
		{
			for (int i = 0; i <= Slots.Num(); i++)
			{
				if (Slots.Num())
				{
					if (i == 0 && !Slots[i].HasInput() || i == Slots.Num() && !Slots[i - 1].HasOutput())
						continue;
				}
				for (int j = 0; j < 2; j++)
				{
					FGroundPoint Point = { Road, Side, i*2+j };
					if (VisitedGroundPoints.Contains(Point))
						continue;
					TArray<FGroundPoint> Points = { Point };
					VisitedGroundPoints.Add(Point);
					while (true)
					{
						FGroundPoint Prev = Points[0].PrevPoint(RoadSlots);
						if (!Prev.Road || VisitedGroundPoints.Contains(Prev))
							break;
						Points.Insert(Prev, 0);
						VisitedGroundPoints.Add(Prev);
					}
					while (true)
					{
						FGroundPoint Next = Points.Last().NextPoint(RoadSlots);
						if (!Next.Road || VisitedGroundPoints.Contains(Next))
							break;
						Points.Add(Next);
						VisitedGroundPoints.Add(Next);
					}
					AGroundActor* Ground = AddGround(RoadSlots, Points);
					if (Points.Last().NextPoint(RoadSlots) == Points[0])
						Ground->bClosedLoop = true;
				}
			}
		}
	}
	for (int i = 0; i < Grounds.Num();)
	{
		AGroundActor* Ground = Grounds[i];
		if (Ground->IsExpired())
		{
			Ground->Destroy();
			Grounds.RemoveAt(i);
		}
		else
			i++;
	}
}

void ARoadScene::OctreeAddBoundary(URoadBoundary* Boundary)
{
	for (int i = 0; i < Boundary->Curve.Points.Num() - 1; i++)
		Octree.AddElement(FRoadOctreeElement(Boundary, i));
}

void ARoadScene::OctreeRemoveBoundary(URoadBoundary* Boundary)
{
	for (int i = 0; i < Boundary->OctreeIds.Num(); i++)
	{
		Octree.RemoveElement(Boundary->OctreeIds[i]);
	}
	Boundary->OctreeIds.Empty();
}

void ARoadScene::OctreeAddRoad(ARoadActor* Road)
{
	TSet<URoadBoundary*> Boundaries = { Road->BaseCurve, Road->GetRoadEdge(0), Road->GetRoadEdge(1) };
	for (URoadBoundary* Boundary : Boundaries)
		OctreeAddBoundary(Boundary);
}

void ARoadScene::OctreeRemoveRoad(ARoadActor* Road)
{
	TSet<URoadBoundary*> Boundaries = { Road->BaseCurve, Road->GetRoadEdge(0), Road->GetRoadEdge(1) };
	for (URoadBoundary* Boundary : Boundaries)
		OctreeRemoveBoundary(Boundary);
}

void ARoadScene::DestroyRoad(ARoadActor* Road)
{
	OctreeRemoveRoad(Road);
	Road->DeleteAllMarkings();
	Road->DisconnectAll();
	Road->Destroy();
	Roads.Remove(Road);
}

void ARoadScene::PostLoad()
{
	AActor::PostLoad();
	for (ARoadActor* Road : Roads)
		OctreeAddRoad(Road);
}
#if WITH_EDITOR
#include "DesktopPlatformModule.h"
void ARoadScene::ExportXodr()
{
	TArray<FString> Files;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		if (DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Export OpenDRIVE"),
			TEXT(""),
			TEXT(""),
			TEXT("OpenDRIVE File (*.xodr)|*.xodr"),
			EFileDialogFlags::None,
			Files))
		{
			FXmlFile XmlFile(TEXT("<OpenDRIVE/>"), EConstructMethod::ConstructFromBuffer);
			FXmlNode* RootNode = XmlFile.GetRootNode();
			int RoadId = 0;
			int ObjectId = 200;
			for (ARoadActor* Road : Roads)
				Road->ExportXodr(RootNode, RoadId, ObjectId, -1);
			for (AJunctionActor* Junction : Junctions)
				Junction->ExportXodr(RootNode, RoadId, ObjectId);
			XmlFile.Save(*Files[0]);
		}
	}
}
#endif
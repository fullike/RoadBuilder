// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadEdMode.h"
#include "EditorModes.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Editor/TransBuffer.h"
#include "DynamicMeshBuilder.h"
#include "ScopedTransaction.h"
#include "RoadEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"

IMPLEMENT_HIT_PROXY(HRoadProxy, HHitProxy);
IMPLEMENT_HIT_PROXY(HGroundProxy, HHitProxy);
IMPLEMENT_HIT_PROXY(HJunctionProxy, HHitProxy);
IMPLEMENT_HIT_PROXY(HRoadCurveProxy, HHitProxy);
IMPLEMENT_HIT_PROXY(HRoadMarkingProxy, HHitProxy);

#define LOCTEXT_NAMESPACE "RoadBuilder"

bool FRoadTool::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Event == IE_Pressed)
	{
		if (Key == EKeys::Escape)
		{
			SelectParent();
			return true;
		}
	}
	return false;
}

bool FRoadTool::EndModify()
{
	if (LazyRebuild)
	{
		GetScene()->Rebuild();
		LazyRebuild = false;
	}
	return true;
}

void FRoadTool::Reset()
{
	FEditorViewportClient* Client = GLevelEditorModeTools().GetFocusedViewportClient();
	Client->Invalidate();
	GetEditWidget()->SetEditNone();
}

ARoadScene* FRoadTool::GetScene() const
{
	return FEdModeRoad::Get()->Scene;
}

ARoadActor*& FRoadTool::GetSelectedRoad() const
{
	return FEdModeRoad::Get()->SelectedRoad;
}

AGroundActor*& FRoadTool::GetSelectedGround() const
{
	return FEdModeRoad::Get()->SelectedGround;
}

AJunctionActor*& FRoadTool::GetSelectedJunction() const
{
	return FEdModeRoad::Get()->SelectedJunction;
}

SRoadEdit* FRoadTool::GetEditWidget() const
{
	TSharedPtr<SWidget> Widget = GLevelEditorModeTools().GetActiveMode(FEdModeRoad::GetModeID())->GetToolkit()->GetInlineContent();
	return (SRoadEdit*)Widget.Get();
}

FRay FRoadTool::GetRay(FEditorViewportClient* ViewportClient) const
{
	FViewport* Viewport = ViewportClient->Viewport;
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags).SetRealtimeUpdate(ViewportClient->IsRealtime()));
	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	FViewportCursorLocation MouseViewportRay(View, ViewportClient, Viewport->GetMouseX(), Viewport->GetMouseY());
	return FRay(MouseViewportRay.GetOrigin(), MouseViewportRay.GetDirection());
}

FVector FRoadTool::LineTrace(const FRay& Ray, AActor* IgnoredActor) const
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	if (IgnoredActor)
		Params.AddIgnoredActor(IgnoredActor);
	if (GetScene()->GetWorld()->LineTraceSingleByChannel(Hit, Ray.Origin, Ray.Origin + Ray.Direction * 1000000.f, ECollisionChannel::ECC_Visibility, Params))
		return Hit.Location;
	return FVector(WORLD_MAX, WORLD_MAX, WORLD_MAX);
}

FVector FRoadTool::LineTrace(FEditorViewportClient* ViewportClient, AActor* IgnoredActor) const
{
	return LineTrace(GetRay(ViewportClient), IgnoredActor);
}

void FRoadTool::SelectParent()
{
	ARoadActor*& SelectedRoad = GetSelectedRoad();
	AJunctionActor*& SelectedJunction = GetSelectedJunction();
	if (SelectedRoad)
	{
		if (AJunctionActor* Junction = Cast<AJunctionActor>(SelectedRoad->GetAttachParentActor()))
			SelectedJunction = Junction;
		SelectedRoad = nullptr;
	}
	else if (SelectedJunction)
		SelectedJunction = nullptr;
	Reset();
}

bool FRoadTool::HandleClickRoad(HHitProxy* HitProxy, const FViewportClick& Click, int* PointIndex)
{
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		ARoadActor*& SelectedRoad = GetSelectedRoad();
		if (HRoadProxy* Proxy = HitProxyCast<HRoadProxy>(HitProxy))
		{
			SelectedRoad = Proxy->Road;
			if (PointIndex)
				*PointIndex = Proxy->Index;
			return true;
		}
		/*
		if (!HitProxy || HitProxy->IsA(HActor::StaticGetType()))
		{
			Reset();
			return true;
		}*/
	}
	return false;
}

bool FRoadTool::HandleClickJunction(HHitProxy* HitProxy, const FViewportClick& Click, int* GateIndex, int* LinkIndex)
{
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		AJunctionActor*& SelectedJunction = GetSelectedJunction();
		if (HJunctionProxy* Proxy = HitProxyCast<HJunctionProxy>(HitProxy))
		{
			SelectedJunction = Proxy->Junction;
			if (GateIndex)
				*GateIndex = Proxy->Index;
			if (LinkIndex)
				*LinkIndex = Proxy->SubId;
			return true;
		}
		/*
		if (!HitProxy || HitProxy->IsA(HActor::StaticGetType()))
		{
			Reset();
			return true;
		}*/
	}
	return false;
}

void FRoadTool::DrawCurve(FPrimitiveDrawInterface* PDI, const FPolyline& Curve, FColor Color, float Thickness, float DepthBias)
{
	for (int i = 0; i < Curve.Points.Num() - 1; i++)
	{
		const FVector& Start = Curve.Points[i].Pos;
		const FVector& End = Curve.Points[i + 1].Pos;
		PDI->DrawLine(Start, End, Color, SDPG_Foreground, Thickness, DepthBias, true);
	}
}

void FRoadTool::DrawPoint(FPrimitiveDrawInterface* PDI, URoadCurve* Curve, double Dist, FColor Color)
{
	FVector Point = Curve->GetPos(Dist);
	PDI->DrawPoint(Point, Color, Size_Point, SDPG_Foreground);
}

void FRoadTool::DrawDivider(FPrimitiveDrawInterface* PDI, URoadLane* Lane, double Dist, FColor Color)
{
	FVector Start = Lane->RightBoundary->GetPos(Dist);
	FVector End = Lane->LeftBoundary->GetPos(Dist);
	PDI->DrawLine(Start, End, Color, SDPG_Foreground);
}

void FRoadTool::DrawRoads(FPrimitiveDrawInterface* PDI, bool DrawLinks)
{
	ARoadActor* SelectedRoad = GetSelectedRoad();
	ARoadScene* Scene = GetScene();
	for (ARoadActor* Road : Scene->Roads)
	{
		PDI->SetHitProxy(new HRoadProxy(Road));
		FColor Color = Road == SelectedRoad ? Color_Select : Color_Road;
		if (Road->RoadPoints.Num() == 1)
			PDI->DrawPoint(Road->GetPos(0), Color, Size_Point, SDPG_Foreground);
		else
			DrawCurve(PDI, Road->BaseCurve->Curve, Color, Thickness_Road, Road == SelectedRoad ? DepthBias_Select : 0);
	}
	if (DrawLinks)
	{
		for (AJunctionActor* Junction : Scene->Junctions)
		{
			for (FJunctionGate& Gate : Junction->Gates)
			{
				for (int i = 0; i < Gate.Links.Num(); i++)
				{
					if (ARoadActor* Road = Gate.Links[i].Road)
					{
						if (URoadCurve* Curve = (i == 1) ? (URoadCurve*)Road->BaseCurve : (URoadCurve*)Road->BaseCurve->RightLane)
						{
							PDI->SetHitProxy(new HRoadProxy(Road));
							DrawCurve(PDI, Curve->Curve, (Road == SelectedRoad) ? Color_Select : Color_Road, Thickness_Road, Road == SelectedRoad ? DepthBias_Select : 0);
						}
					}
				}
			}
		}
	}
}

void FRoadTool::DrawJunction(FPrimitiveDrawInterface* PDI, AJunctionActor* Junction, FColor Color)
{
	TArray<FJunctionGate>& Gates = Junction->Gates;
	PDI->SetHitProxy(new HJunctionProxy(Junction));
	for (int i = 0; i < Gates.Num(); i++)
	{
		FJunctionGate& Gate = Gates[i];
		FJunctionGate& Next = Gates[(i + 1) % Gates.Num()];
		int SrcSide = Gate.Sign > 0 ? 0 : 1;
		int DstSide = Next.Sign > 0 ? 1 : 0;
		URoadBoundary* SrcBoundary = Gate.Road->GetRoadEdge(SrcSide);
		URoadBoundary* PrevBoundary = Gate.Road->GetRoadEdge(!SrcSide);
		URoadBoundary* DstBoundary = Next.Road->GetRoadEdge(DstSide);
		PDI->DrawLine(PrevBoundary->GetPos(Gate.Dist), SrcBoundary->GetPos(Gate.Dist), Color, SDPG_Foreground, Thickness_Road, 0, true);
		if (Gate.Links[1].Road)
			DrawCurve(PDI, Gate.Links[1].Road->BaseCurve->Curve, Color, Thickness_Road);
	}
	USettings_Global* Settings = GetMutableDefault<USettings_Global>();
	if (Settings->DisplayGateRadianPoints)
	{
#if 0
		FVector Center(0, 0, 0);
		for (FJunctionGate& Gate : Junction->Gates)
		{
			FVector Pos = Gate.Road->BaseCurve->GetPos(Gate.Dist);
			Center += Pos / Gates.Num();
		}
		PDI->DrawPoint(Center, FColor::Red, Size_Point, SDPG_Foreground);
		for (FJunctionGate& Gate : Junction->Gates)
		{
			FVector Pos = Gate.Road->BaseCurve->GetPos(Gate.Dist + Gate.Sign * DefaultJunctionExtent);
			PDI->DrawPoint(Pos, FColor::Blue, Size_Point, SDPG_Foreground);
		}
#elif 0
		for (FVector2D& Crossing : Junction->DebugCrossings)
		{
			//	FVector Pos = Gate.Road->BaseCurve->GetPos(Gate.Dist + Gate.Sign * DefaultJunctionExtent);
			PDI->DrawPoint(FVector(Crossing, 0), FColor::Blue, Size_Point, SDPG_Foreground);
		}
#elif 0
		for (int i = 0; i < Junction->DebugCurves.Num(); i++)
		{
			DrawCurve(PDI, Junction->DebugCurves[i], i % 2 ? FColor::Green : FColor::Red, Thickness_Road);
		}
#else
		PDI->SetHitProxy(nullptr);
		for (int i = 0; i < Junction->DebugPoints.Num(); i++)
		{
			FVector& Start = Junction->DebugPoints[i];
			FVector& End = Junction->DebugPoints[(i + 1) % Junction->DebugPoints.Num()];
			FVector Dir = (End - Start).GetSafeNormal();
			FVector N(-Dir.Y, Dir.X, Dir.Z);
			FVector Center = (Start + End) / 2;
			FVector Left = Center - N * 50 - Dir * 100;
			FVector Right = Center + N * 50 - Dir * 100;
			PDI->DrawLine(Start, End, FColor::Blue, SDPG_Foreground, Thickness_Line, 0, true);
			PDI->DrawLine(Left, Center, FColor::Blue, SDPG_Foreground, Thickness_Line, 0, true);
			PDI->DrawLine(Right, Center, FColor::Blue, SDPG_Foreground, Thickness_Line, 0, true);
		}
#endif
	}
}

void FRoadTool::DrawJunctions(FPrimitiveDrawInterface* PDI)
{
	ARoadScene* Scene = GetScene();
	AJunctionActor* SelectedJunction = GetSelectedJunction();
	for (AJunctionActor* Junction : Scene->Junctions)
		DrawJunction(PDI, Junction, (Junction == SelectedJunction) ? Color_Select : Color_Road);
}

FVector FRoadTool_RoadPlan::GetWidgetLocation() const
{
	ARoadActor* Road = GetSelectedRoad();
	if (Road && PointIndex != INDEX_NONE)
		return Road->GetPos(PointIndex);
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_RoadPlan::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	ARoadActor*& Road = GetSelectedRoad();
	if (HandleClickRoad(HitProxy, Click, &PointIndex))
	{
		GetEditWidget()->SetEditRoadPoint(Road, PointIndex);
		return true;
	}
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		Reset();
		return true;
	}
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		const FScopedTransaction Transaction(LOCTEXT("RoadPlan", "RoadPlan"));
		USettings_RoadPlan* Data = GetMutableDefault<USettings_RoadPlan>();
		ARoadScene* Scene = GetScene();
		FRay Ray = GetRay(InViewportClient);
		FVector Pos = FMath::RayPlaneIntersection(Ray.Origin, Ray.Direction, FPlane(FVector(0, 0, Data->BaseHeight), FVector::UpVector));
		if (HRoadProxy* Proxy = HitProxyCast<HRoadProxy>(HitProxy))
		{
			Road = Proxy->Road;
			FVector2D UV = Road->GetUV(Pos);
			Road->Modify();
			Road->AddPoint(UV.X);
		}
		else
		{
			if (!Road)
			{
				Scene->Modify();
				Road = Scene->AddRoad(Data->Style.LoadSynchronous(), Data->BaseHeight);
			}
			Road->Modify();
			Road->InsertPoint((FVector2D&)Pos, PointIndex);
		}
		Road->UpdateCurve();
		Scene->Rebuild();
		GetEditWidget()->SetEditRoadPoint(Road, PointIndex);
		return true;
	}
	return false;
}

bool FRoadTool_RoadPlan::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed)
	{
		if (Key == EKeys::Delete)
		{
			if (ARoadActor* SelectedRoad = GetSelectedRoad())
			{
				ARoadScene* Scene = GetScene();
				if (PointIndex != INDEX_NONE)
				{
					SelectedRoad->RoadPoints.RemoveAt(PointIndex);
					PointIndex = INDEX_NONE;
					SelectedRoad->UpdateCurve();
				}
				else
				{
					Scene->DestroyRoad(SelectedRoad);
					Reset();
				}
				Scene->Rebuild();
			}
			return true;
		}
	}
	return false;
}

bool FRoadTool_RoadPlan::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (ARoadActor* SelectedRoad = GetSelectedRoad())
		{
			SelectedRoad->Modify();
			SelectedRoad->RoadPoints[PointIndex].Pos += (FVector2D&)InDrag;
			if ((PointIndex == 0 || PointIndex == SelectedRoad->RoadPoints.Num() - 1))
			{
				SelectedRoad->DisconnectAll(PointIndex);
				int HeightIndex = PointIndex ? SelectedRoad->HeightPoints.Num() - 1 : 0;
			//	FVector Pos(SelectedRoad->RoadPoints[PointIndex].Pos, SelectedRoad->HeightPoints[HeightIndex].Height);
			//	FVector Location = LineTrace(FRay(InViewportClient->GetViewLocation(), (Pos - InViewportClient->GetViewLocation()).GetSafeNormal()));
				ARoadActor* HoveredRoad = GetScene()->PickRoad(FVector(SelectedRoad->RoadPoints[PointIndex].Pos, SelectedRoad->HeightPoints[HeightIndex].Height), SelectedRoad);
				if (HoveredRoad && HoveredRoad != SelectedRoad)
				{
				//	SelectedRoad->HeightPoints[HeightIndex].Height = Location.Z;
					SelectedRoad->ConnectTo(PointIndex, HoveredRoad);
				}
			}
			SelectedRoad->UpdateCurve();
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_RoadPlan::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	DrawRoads(PDI, false);
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		for (int i = 0; i < SelectedRoad->RoadPoints.Num(); i++)
		{
			PDI->SetHitProxy(new HRoadProxy(SelectedRoad, i));
			PDI->DrawPoint(SelectedRoad->GetPos(i), i == PointIndex ? Color_Select : Color_Road, Size_Point, SDPG_Foreground);
		}
	}
}

FVector FRoadTool_RoadHeight::GetWidgetLocation() const
{
	ARoadActor* Road = GetSelectedRoad();
	if (Road && PointIndex != INDEX_NONE)
		return Road->BaseCurve->GetPos(Road->HeightPoints[PointIndex].Dist);
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_RoadHeight::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	ARoadActor* Road = GetSelectedRoad();
	if (Road && PointIndex != INDEX_NONE)
	{
		InMatrix = FRotationMatrix(Road->BaseCurve->GetDir(Road->HeightPoints[PointIndex].Dist).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_RoadHeight::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	ARoadActor*& SelectedRoad = GetSelectedRoad();
	if (HandleClickRoad(HitProxy, Click, &PointIndex))
	{
		GetEditWidget()->SetEditHeightPoint(SelectedRoad, PointIndex);
		return true;
	}
	if (SelectedRoad)
	{
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			FVector2D UV = SelectedRoad->GetUV(LineTrace(InViewportClient));
			if (HRoadProxy* Proxy = HitProxyCast<HRoadProxy>(HitProxy))
			{
				const FScopedTransaction Transaction(LOCTEXT("RoadHeight", "RoadHeight"));
				SelectedRoad->Modify();
				PointIndex = SelectedRoad->AddHeight(UV.X);
				GetEditWidget()->SetEditHeightPoint(SelectedRoad, PointIndex);
				SelectedRoad->UpdateCurve();
				GetScene()->Rebuild();
				return true;
			}
		}
	}
	return false;
}

bool FRoadTool_RoadHeight::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed)
	{
		if (Key == EKeys::Delete)
		{
			ARoadActor* SelectedRoad = GetSelectedRoad();
			if (SelectedRoad && PointIndex != INDEX_NONE)
			{
				SelectedRoad->HeightPoints.RemoveAt(PointIndex);
				PointIndex = INDEX_NONE;
				GetScene()->Rebuild();
			}
			return true;
		}
	}
	return false;
}

bool FRoadTool_RoadHeight::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (ARoadActor* SelectedRoad = GetSelectedRoad())
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			SelectedRoad->HeightPoints[PointIndex].Dist += LocalDrag.X;
			SelectedRoad->UpdateCurve();
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_RoadHeight::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	DrawRoads(PDI, false);
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		for (int i = 0; i < SelectedRoad->HeightPoints.Num(); i++)
		{
			PDI->SetHitProxy(new HRoadProxy(SelectedRoad, i));
			PDI->DrawPoint(SelectedRoad->BaseCurve->GetPos(SelectedRoad->HeightPoints[i].Dist), i == PointIndex ? Color_Select : Color_Road, Size_Point, SDPG_Foreground);
		}
	}
}

bool FRoadTool_RoadChop::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	ARoadActor*& SelectedRoad = GetSelectedRoad();
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (SelectedRoad)
	{
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			if (HRoadProxy* Proxy = HitProxyCast<HRoadProxy>(HitProxy))
			{
				const FScopedTransaction Transaction(LOCTEXT("RoadChop", "RoadChop"));
				FVector2D UV = SelectedRoad->GetUV(LineTrace(InViewportClient));
				if (SelectedRoad == Proxy->Road)
					SelectedRoad->Chop(UV.X);
				else
					SelectedRoad->Join(Proxy->Road);
				GetScene()->Rebuild();
				return true;
			}
		}
	}
	return false;
}

void FRoadTool_RoadChop::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	DrawRoads(PDI, false);
}

bool FRoadTool_RoadSplit::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	ARoadActor*& SelectedRoad = GetSelectedRoad();
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (SelectedRoad)
	{
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				const FScopedTransaction Transaction(LOCTEXT("RoadSplit", "RoadSplit"));
				SelectedRoad->Split(Cast<URoadBoundary>(Proxy->Curve));
				GetScene()->Rebuild();
				return true;
			}
		}
	}
	return false;
}

void FRoadTool_RoadSplit::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (SelectedRoad->Length() > 0)
		{
			for (URoadBoundary* Boundary : SelectedRoad->Boundaries)
			{
				PDI->SetHitProxy(new HRoadCurveProxy(Boundary));
				DrawCurve(PDI, Boundary->Curve, Color_Line, Thickness_Line);
			}
		}
	}
	else
		DrawRoads(PDI, false);
}

bool FRoadTool_JunctionLink::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	AJunctionActor*& Junction = GetSelectedJunction();
	if (HandleClickJunction(HitProxy, Click, &GateIndex, &LinkIndex))
	{
		GetEditWidget()->SetEditLink(Junction, GateIndex, LinkIndex);
		return true;
	}
	return false;
}

void FRoadTool_JunctionLink::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (AJunctionActor* Junction = GetSelectedJunction())
	{
		for (int i = 0; i < Junction->Gates.Num(); i++)
		{
			FJunctionGate& Gate = Junction->Gates[i];
			for (int j = 0; j < Gate.Links.Num(); j++)
			{
				if (ARoadActor* Road = Gate.Links[j].Road)
				{
					if (URoadCurve* Curve = (j == 1) ? (URoadCurve*)Road->BaseCurve : (URoadCurve*)Road->BaseCurve->RightLane)
					{
						PDI->SetHitProxy(new HJunctionProxy(Junction, i, j));
						if (i == GateIndex && j == LinkIndex)
							DrawCurve(PDI, Curve->Curve, Color_Select, Thickness_Road, DepthBias_Select);
						else
							DrawCurve(PDI, Curve->Curve, Color_Road, Thickness_Road);
					}
				}
			}
		}
	}
	else
		DrawJunctions(PDI);
}

FVector FRoadTool_LaneEdit::GetWidgetLocation() const
{
	if (CurrentLane)
		return CurrentLane->GetPos(CurrentLane->SegmentStart(SegmentIndex));
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_LaneEdit::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (CurrentLane)
	{
		InMatrix = FRotationMatrix(CurrentLane->GetDir(CurrentLane->SegmentStart(SegmentIndex)).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_LaneEdit::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		FVector2D UV = SelectedRoad->GetUV(LineTrace(InViewportClient));
		URoadLane* Lane = SelectedRoad->GetLane(UV);
		if (Click.GetKey() == EKeys::LeftMouseButton)
		{
			CurrentLane = Lane;
			SegmentIndex = CurrentLane ? CurrentLane->GetSegment(UV.X) : INDEX_NONE;
			GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			return true;
		}
		if (Click.GetKey() == EKeys::RightMouseButton && CurrentLane)
		{
			const FScopedTransaction Transaction(LOCTEXT("LaneEdit", "LaneEdit"));
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				SelectedRoad->CopyLane(CurrentLane, Proxy->Curve == CurrentLane->LeftBoundary);
				SelectedRoad->UpdateLanes();
				GetScene()->Rebuild();
			}
			else if (CurrentLane == Lane)
			{
				CurrentLane->Modify();
				SegmentIndex = CurrentLane->AddSegment(UV.X);
				GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			}
			else
			{
				CurrentLane = Lane;
				SegmentIndex = CurrentLane ? CurrentLane->GetSegment(UV.X) : INDEX_NONE;
				GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			}
			return true;
		}
	}
	return false;
}

bool FRoadTool_LaneEdit::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed && CurrentLane && SegmentIndex != INDEX_NONE)
	{
		ARoadActor* SelectedRoad = GetSelectedRoad();
		if (Key == EKeys::Home)
		{
			if (CurrentLane->LeftBoundary->LeftLane)
			{
				CurrentLane = CurrentLane->LeftBoundary->LeftLane;
				SegmentIndex = FMath::Min(SegmentIndex, CurrentLane->Segments.Num() - 1);
				GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			}
			return true;
		}
		if (Key == EKeys::End)
		{
			if (CurrentLane->RightBoundary->RightLane)
			{
				CurrentLane = CurrentLane->RightBoundary->RightLane;
				SegmentIndex = FMath::Min(SegmentIndex, CurrentLane->Segments.Num() - 1);
				GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			}
			return true;
		}
		if (Key == EKeys::Tab)
		{
			SegmentIndex = (SegmentIndex + 1) % CurrentLane->Segments.Num();
			GetEditWidget()->SetEditLaneSegment(CurrentLane, SegmentIndex);
			return true;
		}
		if (Key == EKeys::Delete)
		{	
			CurrentLane->DeleteSegment(SegmentIndex);
			SelectedRoad->UpdateLanes();
			SelectedRoad->GetScene()->Rebuild();
			Reset();
			return true;
		}
	}
	return false;
}

bool FRoadTool_LaneEdit::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (CurrentLane)
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			CurrentLane->Modify();
			CurrentLane->SegmentStart(SegmentIndex) += LocalDrag.X;
			CurrentLane->SnapSegment(SegmentIndex);
			CurrentLane->GetRoad()->UpdateLanes();
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_LaneEdit::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (SelectedRoad->Length() > 0)
		{
			PDI->SetHitProxy(nullptr);
			for (URoadBoundary* Boundary : SelectedRoad->Boundaries)
			{
				if (!CurrentLane || Boundary != CurrentLane->LeftBoundary && Boundary != CurrentLane->RightBoundary)
					DrawCurve(PDI, Boundary->Curve, Color_Line, Thickness_Line);
			}
			for (URoadLane* Lane : SelectedRoad->Lanes)
			{
				for (int i = 0; i < Lane->Segments.Num(); i++)
				{
					double Start = Lane->SegmentStart(i);
					double End = Lane->SegmentEnd(i);
					FColor BoundaryColor = (Lane == CurrentLane && i == SegmentIndex) ? Color_Select : Color_Line;
					FColor Color = (Lane == CurrentLane && (i == SegmentIndex || i - 1 == SegmentIndex)) ? Color_Select : Color_Line;
					if (Lane == CurrentLane)
					{
						PDI->SetHitProxy(new HRoadCurveProxy(Lane->RightBoundary));
						DrawCurve(PDI, Lane->RightBoundary->CreatePolyline(Start, End), BoundaryColor, Thickness_Line, DepthBias_Select);
						PDI->SetHitProxy(new HRoadCurveProxy(Lane->LeftBoundary));
						DrawCurve(PDI, Lane->LeftBoundary->CreatePolyline(Start, End), BoundaryColor, Thickness_Line, DepthBias_Select);
						PDI->SetHitProxy(nullptr);
					}
					DrawDivider(PDI, Lane, Start, Color);
					if (i + 1 == Lane->Segments.Num())
						DrawDivider(PDI, Lane, End, BoundaryColor);
				}
			}
		}
	}
	else
		DrawRoads(PDI, false);
}

bool FRoadTool_LaneCarve::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				FVector2D EndUV = SelectedRoad->GetUV(LineTrace(InViewportClient));
				EndUV.Y = Proxy->Curve->GetOffset(EndUV.X);
				if (StartUV.X < 0)
					StartUV = EndUV;
				else
				{
					double StartOffset = StartUV.X;
					double EndOffset = EndUV.X;
					TArray<URoadBoundary*> StartBs = SelectedRoad->GetBoundaries(StartUV);
					TArray<URoadBoundary*> EndBs = SelectedRoad->GetBoundaries(EndUV);
					URoadLane* Lane = nullptr;
					int Side = INDEX_NONE;
					if (EndOffset < StartOffset)
					{
						Swap(StartBs, EndBs);
						Swap(StartOffset, EndOffset);
					}
					if (StartBs[0]->LeftLane == EndBs.Last()->RightLane)
					{
						Lane = StartBs[0]->LeftLane;
						Side = 0;
					}
					else if (StartBs.Last()->RightLane == EndBs[0]->LeftLane)
					{
						Lane = EndBs[0]->LeftLane;
						Side = 1;
					}
					if (Side != -1)
					{
						int LaneSide = Lane->GetSide();
						double LaneWidth = Lane->GetWidth((StartOffset + EndOffset) / 2);
						const FScopedTransaction Transaction(LOCTEXT("LaneCarve", "LaneCarve"));
						SelectedRoad->Modify();
						URoadLane* ForkLane = nullptr;
						if (URoadLane* SideLane = Lane->GetBoundary(Side)->GetLane(Side))
							if (FMath::IsNearlyZero(SideLane->GetWidth(StartOffset)) && FMath::IsNearlyZero(SideLane->GetWidth(EndOffset)))
								ForkLane = SideLane;
						if (!ForkLane)
							ForkLane = SelectedRoad->CopyLane(Lane, Side);
						{
							Lane->Modify();
						//	int Seg = Lane->AddSegment(EndOffset);
						//	Lane->Segments[Seg].LaneType = ELaneType::None;
							URoadBoundary* B = Lane->GetBoundary(LaneSide);
							B->Modify();
							int PrevIdx = B->AddLocalOffset(StartOffset);
							int NextIdx = B->AddLocalOffset(EndOffset);
						//	B->LocalOffsets[PrevIdx - 1].Offset = LaneWidth;
						//	B->LocalOffsets[PrevIdx].Offset = LaneWidth;
							B->LocalOffsets[NextIdx].Offset = 0;
							B->LocalOffsets[NextIdx + 1].Offset = 0;
							B->AddSegment(StartOffset);
							B->AddSegment(EndOffset);
						}
						{
							ForkLane->Modify();
						//	int Seg = ForkLane->AddSegment(StartOffset);
						//	ForkLane->Segments[Seg - 1].LaneType = ELaneType::None;
							URoadBoundary* B = ForkLane->GetBoundary(LaneSide);
							B->Modify();
							int PrevIdx = B->AddLocalOffset(StartOffset);
							int NextIdx = B->AddLocalOffset(EndOffset);
							B->LocalOffsets[PrevIdx - 1].Offset = 0;
							B->LocalOffsets[PrevIdx].Offset = 0;
							B->LocalOffsets[NextIdx].Offset = LaneWidth;
							B->LocalOffsets[NextIdx + 1].Offset = LaneWidth;
							B->AddSegment(StartOffset);
							B->AddSegment(EndOffset);
						}
						SelectedRoad->UpdateLanes();
						GetScene()->Rebuild();
					}
					Reset();
				}
				return true;
			}
		}
	}
	return false;
}

void FRoadTool_LaneCarve::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (SelectedRoad->Length() > 0)
		{
			for (URoadBoundary* Boundary : SelectedRoad->Boundaries)
			{
				PDI->SetHitProxy(new HRoadCurveProxy(Boundary));
				DrawCurve(PDI, Boundary->Curve, Color_Line, Thickness_Line);
			}
			if (StartUV.X >= 0)
			{
				FVector StartPos = SelectedRoad->GetPos(StartUV);
				PDI->DrawPoint(StartPos, Color_Line, Size_Point, SDPG_Foreground);
				FVector Pos = LineTrace(static_cast<FEditorViewportClient*>(Viewport->GetClient()));
				if (Pos.X < WORLD_MAX)
				{
					FVector2D EndUV = SelectedRoad->GetUV(Pos);
					FVector EndPos = SelectedRoad->GetPos(EndUV);
					PDI->DrawLine(StartPos, EndPos, Color_Line, SDPG_Foreground, Thickness_Line, 0, true);
				}
			}
		}
	}
	else
		DrawRoads(PDI, false);
}

FVector FRoadTool_LaneWidth::GetWidgetLocation() const
{
	if (CurrentBoundary && OffsetIndex != INDEX_NONE)
		return CurrentBoundary->GetPos(CurrentBoundary->LocalOffsets[OffsetIndex].Dist);
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_LaneWidth::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (CurrentBoundary && OffsetIndex != INDEX_NONE)
	{
		InMatrix = FRotationMatrix(CurrentBoundary->GetDir(CurrentBoundary->LocalOffsets[OffsetIndex].Dist).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_LaneWidth::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		FVector2D UV = SelectedRoad->GetUV(LineTrace(InViewportClient));
		if (Click.GetKey() == EKeys::LeftMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				CurrentBoundary = Cast<URoadBoundary>(Proxy->Curve);
				OffsetIndex = Proxy->Index;
				GetEditWidget()->SetEditBoundaryOffset(CurrentBoundary, OffsetIndex);
			}
			else
				Reset();
			return true;
		}
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				const FScopedTransaction Transaction(LOCTEXT("LaneWidth", "LaneWidth"));
				CurrentBoundary = Cast<URoadBoundary>(Proxy->Curve);
				CurrentBoundary->Modify();
				OffsetIndex = CurrentBoundary->AddLocalOffset(UV.X);
				GetEditWidget()->SetEditBoundaryOffset(CurrentBoundary, OffsetIndex);
				return true;
			}
		}
	}
	return false;
}

bool FRoadTool_LaneWidth::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed && CurrentBoundary && OffsetIndex != INDEX_NONE)
	{
		ARoadActor* SelectedRoad = GetSelectedRoad();
		if (Key == EKeys::Delete)
		{
			CurrentBoundary->DeleteOffset(OffsetIndex);
			OffsetIndex = INDEX_NONE;
			SelectedRoad->UpdateLanes();
			SelectedRoad->GetScene()->Rebuild();		
			return true;
		}
	}
	return false;
}

bool FRoadTool_LaneWidth::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (CurrentBoundary)
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			CurrentBoundary->Modify();
			CurrentBoundary->LocalOffsets[OffsetIndex].Dist += LocalDrag.X;
			CurrentBoundary->LocalOffsets[OffsetIndex].Offset += LocalDrag.Y;
			CurrentBoundary->SnapOffset(OffsetIndex);
			CurrentBoundary->GetRoad()->UpdateLanes();
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_LaneWidth::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (SelectedRoad->Length() > 0)
		{
			for (URoadBoundary* Boundary : SelectedRoad->Boundaries)
			{
				if (Boundary != CurrentBoundary)
				{
					PDI->SetHitProxy(new HRoadCurveProxy(Boundary));
					DrawCurve(PDI, Boundary->Curve, Color_Line, Thickness_Line);
				}
			}
			if (CurrentBoundary)
			{
				PDI->SetHitProxy(new HRoadCurveProxy(CurrentBoundary));
				DrawCurve(PDI, CurrentBoundary->Curve, Color_Select, Thickness_Line, DepthBias_Select);
				for (int i = 0; i < CurrentBoundary->LocalOffsets.Num(); i++)
				{
					double Dist = CurrentBoundary->LocalOffsets[i].Dist;
					FColor Color = i == OffsetIndex ? Color_Select : Color_Line;
					PDI->SetHitProxy(new HRoadCurveProxy(CurrentBoundary, i));
					PDI->DrawPoint(CurrentBoundary->GetPos(Dist), Color, Size_Point, SDPG_Foreground);
					if (CurrentBoundary != SelectedRoad->BaseCurve)
					{
						URoadLane* Lane = CurrentBoundary->GetSide() ? CurrentBoundary->RightLane : CurrentBoundary->LeftLane;
						DrawDivider(PDI, Lane, Dist, Color);
					}
				}
			}
		}
	}
	else
		DrawRoads(PDI, false);
}

FVector FRoadTool_MarkingLane::GetWidgetLocation() const
{
	if (CurrentBoundary)
		return CurrentBoundary->GetPos(CurrentBoundary->SegmentStart(SegmentIndex));
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_MarkingLane::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (CurrentBoundary)
	{
		InMatrix = FRotationMatrix(CurrentBoundary->GetDir(CurrentBoundary->SegmentStart(SegmentIndex)).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_MarkingLane::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		FVector2D UV = SelectedRoad->GetUV(LineTrace(InViewportClient));
		if (Click.GetKey() == EKeys::LeftMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				CurrentBoundary = Cast<URoadBoundary>(Proxy->Curve);
				SegmentIndex = Proxy->Index;
				GetEditWidget()->SetEditBoundarySegment(CurrentBoundary, SegmentIndex);
			}
			else
				Reset();
			return true;
		}
		if (Click.GetKey() == EKeys::RightMouseButton)
		{
			if (HRoadCurveProxy* Proxy = HitProxyCast<HRoadCurveProxy>(HitProxy))
			{
				const FScopedTransaction Transaction(LOCTEXT("MarkingLane", "MarkingLane"));
				CurrentBoundary = Cast<URoadBoundary>(Proxy->Curve);
				CurrentBoundary->Modify();
				SegmentIndex = CurrentBoundary->AddSegment(UV.X);
				GetEditWidget()->SetEditBoundarySegment(CurrentBoundary, SegmentIndex);
				return true;
			}
		}
	}
	return false;
}

bool FRoadTool_MarkingLane::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed && CurrentBoundary && SegmentIndex != INDEX_NONE)
	{
		ARoadActor* SelectedRoad = GetSelectedRoad();
		if (Key == EKeys::Home)
		{
			if (CurrentBoundary->LeftLane->LeftBoundary)
			{
				CurrentBoundary = CurrentBoundary->LeftLane->LeftBoundary;
				SegmentIndex = FMath::Min(SegmentIndex, CurrentBoundary->Segments.Num() - 1);
				GetEditWidget()->SetEditBoundarySegment(CurrentBoundary, SegmentIndex);
			}
			return true;
		}
		if (Key == EKeys::End)
		{
			if (CurrentBoundary->RightLane->RightBoundary)
			{
				CurrentBoundary = CurrentBoundary->RightLane->RightBoundary;
				SegmentIndex = FMath::Min(SegmentIndex, CurrentBoundary->Segments.Num() - 1);
				GetEditWidget()->SetEditBoundarySegment(CurrentBoundary, SegmentIndex);
			}
			return true;
		}
		if (Key == EKeys::Tab)
		{
			SegmentIndex = (SegmentIndex + 1) % CurrentBoundary->Segments.Num();
			GetEditWidget()->SetEditBoundarySegment(CurrentBoundary, SegmentIndex);
			return true;
		}
		if (Key == EKeys::Delete)
		{
			CurrentBoundary->DeleteSegment(SegmentIndex);
			SelectedRoad->UpdateLanes();
			SelectedRoad->GetScene()->Rebuild();
			Reset();
			return true;
		}
	}
	return false;
}

bool FRoadTool_MarkingLane::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (CurrentBoundary)
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			CurrentBoundary->Modify();
			CurrentBoundary->SegmentStart(SegmentIndex) += LocalDrag.X;
			CurrentBoundary->SnapSegment(SegmentIndex);
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_MarkingLane::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
	{
		if (SelectedRoad->Length() > 0)
		{
			for (URoadBoundary* Boundary : SelectedRoad->Boundaries)
			{
				for (int i = 0; i < Boundary->Segments.Num(); i++)
				{
					if (Boundary != SelectedRoad->BaseCurve && Boundary->IsZeroOffset(i))
						continue;
					double Start = Boundary->SegmentStart(i);
					double End = Boundary->SegmentEnd(i);
					PDI->SetHitProxy(new HRoadCurveProxy(Boundary, i));
					FColor Color = (Boundary == CurrentBoundary && i == SegmentIndex) ? Color_Select : Color_Line;
					float DepthBias = (Boundary == CurrentBoundary && i == SegmentIndex) ? DepthBias_Select : 0;
					DrawCurve(PDI, Boundary->CreatePolyline(Start, End), Color, Thickness_Line, DepthBias);
					DrawPoint(PDI, Boundary, Start, Color);
					if (i + 1 == Boundary->Segments.Num())
						DrawPoint(PDI, Boundary, End, Color);
				}
			}
		}
	}
	else
		DrawRoads(PDI, true);
}

FVector FRoadTool_MarkingPoint::GetWidgetLocation() const
{
	if (CurrentMarking)
		return CurrentMarking->GetRoad()->GetPos(CurrentMarking->Point);
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_MarkingPoint::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (CurrentMarking)
	{
		InMatrix = FRotationMatrix(CurrentMarking->GetRoad()->GetDir(CurrentMarking->Point.X).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_MarkingPoint::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	ARoadActor* SelectedRoad = GetSelectedRoad();
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		if (HRoadMarkingProxy* Proxy = HitProxyCast<HRoadMarkingProxy>(HitProxy))
		{
			CurrentMarking = Cast<UMarkingPoint>(Proxy->Marking);
			GetEditWidget()->SetEditMarkingPoint(CurrentMarking);
		}
		else
			Reset();
		return true;
	}
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		if (SelectedRoad)
		{
			const FScopedTransaction Transaction(LOCTEXT("MarkingPoint", "MarkingPoint"));
			SelectedRoad->Modify();
			CurrentMarking = SelectedRoad->AddMarkingPoint(SelectedRoad->GetUV(LineTrace(InViewportClient)));
			GetEditWidget()->SetEditMarkingPoint(CurrentMarking);
		}
		return true;
	}
	return false;
}

bool FRoadTool_MarkingPoint::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed && CurrentMarking)
	{
		if (Key == EKeys::Delete)
		{
			ARoadActor* SelectedRoad = GetSelectedRoad();
			SelectedRoad->DeleteMarking(CurrentMarking);
			CurrentMarking = nullptr;
			SelectedRoad->GetScene()->Rebuild();
			return true;
		}
	}
	return false;
}

bool FRoadTool_MarkingPoint::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (CurrentMarking)
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			CurrentMarking->Modify();
			CurrentMarking->Point += (FVector2D&)LocalDrag;
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_MarkingPoint::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	auto Draw = [&](ARoadActor* Road)
	{
		for (URoadMarking* Marking : Road->Markings)
		{
			if (UMarkingPoint* MarkingPoint = Cast<UMarkingPoint>(Marking))
			{
				PDI->SetHitProxy(new HRoadMarkingProxy(MarkingPoint));
				PDI->DrawPoint(MarkingPoint->GetRoad()->GetPos(MarkingPoint->Point), Marking == CurrentMarking ? Color_Select : Color_Line, Size_Point, SDPG_Foreground);
			}
		}
	};
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
		Draw(SelectedRoad);
	else
		DrawRoads(PDI, true);
}

FVector FRoadTool_MarkingCurve::GetWidgetLocation() const
{
	if (CurrentMarking)
		return CurrentMarking->GetRoad()->GetPos(PointIndex != INDEX_NONE ? CurrentMarking->Points[PointIndex].GetUV(SubIndex) : CurrentMarking->Center());
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_MarkingCurve::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (CurrentMarking)
	{
		InMatrix = FRotationMatrix(CurrentMarking->GetRoad()->GetDir(PointIndex != INDEX_NONE ? CurrentMarking->Points[PointIndex].GetUV(SubIndex).X : CurrentMarking->Center().X).Rotation());
		return true;
	}
	return false;
}

bool FRoadTool_MarkingCurve::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HandleClickRoad(HitProxy, Click))
		return true;
	ARoadActor* SelectedRoad = GetSelectedRoad();
	auto ClickProxy = [&](HRoadMarkingProxy* Proxy)
	{
		CurrentMarking = Cast<UMarkingCurve>(Proxy->Marking);
		PointIndex = Proxy->Index;
		SubIndex = Proxy->SubId;
		GetEditWidget()->SetEditMarkingCurve(CurrentMarking, PointIndex);
	};
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		if (HRoadMarkingProxy* Proxy = HitProxyCast<HRoadMarkingProxy>(HitProxy))
			ClickProxy(Proxy);
		else
			Reset();
		return true;
	}
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		const FScopedTransaction Transaction(LOCTEXT("MarkingCurve", "MarkingCurve"));
		HRoadMarkingProxy* Proxy = HitProxyCast<HRoadMarkingProxy>(HitProxy);
		if (CurrentMarking && CurrentMarking->IsEndPoint(PointIndex))
		{
			if (Proxy)
			{
				if (Proxy->Marking == CurrentMarking && Proxy->Index != PointIndex && CurrentMarking->IsEndPoint(Proxy->Index))
					CurrentMarking->MakeClose();
				else
					ClickProxy(Proxy);
			}
			else
				CurrentMarking->InsertPoint(SelectedRoad->GetUV(LineTrace(InViewportClient)), PointIndex);
		}
		else if (Proxy)
			ClickProxy(Proxy);
		else if (SelectedRoad)
		{
			SelectedRoad->Modify();
			CurrentMarking = SelectedRoad->AddMarkingCurve();
			PointIndex = INDEX_NONE;
			CurrentMarking->InsertPoint(SelectedRoad->GetUV(LineTrace(InViewportClient)), PointIndex);
			GetEditWidget()->SetEditMarkingCurve(CurrentMarking, PointIndex);
		}
		return true;
	}
	return false;
}

bool FRoadTool_MarkingCurve::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (FRoadTool::InputKey(ViewportClient, Viewport, Key, Event))
		return true;
	if (Event == IE_Pressed && CurrentMarking)
	{
		if (Key == EKeys::Delete)
		{
			ARoadActor* SelectedRoad = GetSelectedRoad();
			SelectedRoad->DeleteMarking(CurrentMarking);
			CurrentMarking = nullptr;
			SelectedRoad->GetScene()->Rebuild();
			return true;
		}
	}
	return false;
}

bool FRoadTool_MarkingCurve::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		if (CurrentMarking)
		{
			FMatrix Mat = GLevelEditorModeTools().GetCustomDrawingCoordinateSystem();
			FVector LocalDrag = Mat.InverseTransformVector(InDrag);
			CurrentMarking->Modify();
			if (PointIndex != INDEX_NONE)
				CurrentMarking->Points[PointIndex].ApplyDelta(SubIndex, (FVector2D&)LocalDrag);
			else
			{
				for (FMarkingCurvePoint& Point : CurrentMarking->Points)
					Point.Pos += (FVector2D&)LocalDrag;
			}
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_MarkingCurve::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	auto Draw = [&](ARoadActor* Road)
	{
		for (URoadMarking* Marking : Road->Markings)
		{
			if (UMarkingCurve* MarkingCurve = Cast<UMarkingCurve>(Marking))
			{
				PDI->SetHitProxy(new HRoadMarkingProxy(Marking, INDEX_NONE));
				DrawCurve(PDI, MarkingCurve->CreatePolyline(), CurrentMarking == Marking ? Color_Select : Color_Line, Thickness_Road);
				if (CurrentMarking == MarkingCurve)
				{
					for (int i = 0; i < MarkingCurve->Points.Num(); i++)
					{
						for (int j = 0; j < (i == PointIndex ? 3 : 1); j++)
						{
							FVector Pos = MarkingCurve->GetRoad()->GetPos(MarkingCurve->Points[i].GetUV(j));
							PDI->SetHitProxy(new HRoadMarkingProxy(Marking, i, j));
							PDI->DrawPoint(Pos, i == PointIndex ? Color_Select : Color_Line, Size_Point, SDPG_Foreground);
							if (j > 0)
								PDI->DrawLine(Pos, MarkingCurve->GetRoad()->GetPos(MarkingCurve->Points[i].GetUV(0)), Color_Select, SDPG_Foreground, Thickness_Line);
						}
					}
				}
			}
		}
	};
	if (ARoadActor* SelectedRoad = GetSelectedRoad())
		Draw(SelectedRoad);
	else
		DrawRoads(PDI, true);
}

FVector FRoadTool_GroundEdit::GetWidgetLocation() const
{
	AGroundActor* Ground = GetSelectedGround();
	if (PointIndex != INDEX_NONE && Ground->Points[PointIndex].Road == nullptr)
		return Ground->ManualPoints[Ground->Points[PointIndex].Index];
	return FRoadTool::GetWidgetLocation();
}

bool FRoadTool_GroundEdit::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	ARoadScene* Scene = GetScene();
	AGroundActor*& SelectedGround = GetSelectedGround();
	if (Click.GetKey() == EKeys::LeftMouseButton)
	{
		if (HGroundProxy* Proxy = HitProxyCast<HGroundProxy>(HitProxy))
		{
			SelectedGround = Proxy->Ground;
			PointIndex = Proxy->Index;
			GetEditWidget()->SetEditGround(SelectedGround, PointIndex);
		}
		return true;
	}
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		if (SelectedGround && SelectedGround->IsEndPoint(PointIndex))
		{
			const FScopedTransaction Transaction(LOCTEXT("GroundEdit", "GroundEdit"));
			if (HGroundProxy* Proxy = HitProxyCast<HGroundProxy>(HitProxy))
			{
				if (Proxy->Ground != SelectedGround)
				{
					if (Proxy->Ground->IsEndPoint(Proxy->Index))
					{
						SelectedGround->Modify();
						SelectedGround->Join(Proxy->Ground, PointIndex);
						Scene->Grounds.Remove(Proxy->Ground);
						Proxy->Ground->Destroy();
						Scene->Rebuild();
						GetEditWidget()->SetEditGround(SelectedGround, PointIndex);
					}
				}
				else if (Proxy->Index != PointIndex)
				{
					if (Proxy->Ground->IsEndPoint(Proxy->Index))
					{
						SelectedGround->Modify();
						SelectedGround->bClosedLoop = true;
						Scene->Rebuild();
						GetEditWidget()->SetEditGround(SelectedGround, PointIndex);
					}
				}
			}
			else
			{
				FGroundPoint& Point = SelectedGround->Points[PointIndex];
				TMap<ARoadActor*, TArray<FJunctionSlot>> RoadSlots = Scene->GetAllJunctionSlots();
				FVector P = Point.Road ? Point.GetPos(RoadSlots) : SelectedGround->ManualPoints[Point.Index];
				FRay Ray = GetRay(InViewportClient);
				FVector Pos = FMath::RayPlaneIntersection(Ray.Origin, Ray.Direction, FPlane(FVector(0, 0, P.Z), FVector::UpVector));
				SelectedGround->Modify();
				SelectedGround->AddManualPoint(Pos, PointIndex);
				Scene->Rebuild();
				GetEditWidget()->SetEditGround(SelectedGround, PointIndex);
			}
		}
		return true;
	}
	return false;
}

bool FRoadTool_GroundEdit::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		AGroundActor* Ground = GetSelectedGround();
		if (PointIndex != INDEX_NONE && Ground->Points[PointIndex].Road == nullptr)
		{
			Ground->Modify();
			Ground->ManualPoints[Ground->Points[PointIndex].Index] += InDrag;
			LazyRebuild = true;
		}
		return true;
	}
	return false;
}

void FRoadTool_GroundEdit::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	ARoadScene* Scene = GetScene();
	TMap<ARoadActor*, TArray<FJunctionSlot>> RoadSlots = Scene->GetAllJunctionSlots();
	AGroundActor* SelectedGround = GetSelectedGround();
	for (AGroundActor* Ground : Scene->Grounds)
	{
		TArray<FVector> Vertices = Ground->GetVertices(RoadSlots);
		FColor Color = (Ground == SelectedGround) ? Color_Select : Color_Road;
		PDI->SetHitProxy(new HGroundProxy(Ground));
		for (int i = 0; i < Vertices.Num() - !Ground->bClosedLoop; i++)
		{
			const FVector& Start = Vertices[i];
			const FVector& End = Vertices[(i + 1) % Vertices.Num()];
			PDI->DrawLine(Start, End, Color, SDPG_Foreground, Thickness_Road, 0, true);
#if 0
			FVector Dir = (End - Start).GetSafeNormal();
			FVector N(-Dir.Y, Dir.X, Dir.Z);
			FVector Center = (Start + End) / 2;
			FVector Left = Center - N * 50 - Dir * 100;
			FVector Right = Center + N * 50 - Dir * 100;
			PDI->DrawLine(Left, Center, FColor::Blue, SDPG_Foreground, Thickness_Line, 0, true);
			PDI->DrawLine(Right, Center, FColor::Blue, SDPG_Foreground, Thickness_Line, 0, true);
#endif
		}
		for (int i = 0; i < Ground->Points.Num(); i++)
		{
			FGroundPoint& Point = Ground->Points[i];
			PDI->SetHitProxy(new HGroundProxy(Ground, i));
			FVector Pos = Point.Road ? Point.GetPos(RoadSlots) : Ground->ManualPoints[Point.Index];
			PDI->DrawPoint(Pos, (Ground == SelectedGround && PointIndex == i) ? Color_Select : Color_Road, Size_Point, SDPG_Foreground);
		}
	}
}

FEdModeRoad::FEdModeRoad()
{
	Tools.Add(new FRoadTool_File);
	Tools.Add(new FRoadTool_RoadPlan);
	Tools.Add(new FRoadTool_RoadHeight);
	Tools.Add(new FRoadTool_RoadChop);
	Tools.Add(new FRoadTool_RoadSplit);
	Tools.Add(new FRoadTool_JunctionLink);
	Tools.Add(new FRoadTool_LaneEdit);
	Tools.Add(new FRoadTool_LaneCarve);
	Tools.Add(new FRoadTool_LaneWidth);
	Tools.Add(new FRoadTool_MarkingLane);
	Tools.Add(new FRoadTool_MarkingPoint);
	Tools.Add(new FRoadTool_MarkingCurve);
	Tools.Add(new FRoadTool_GroundEdit);
	Tools.Add(new FRoadTool_Settings);
}

FEdModeRoad::~FEdModeRoad()
{
	for (FModeTool* Tool : Tools)
		delete Tool;
}

int FEdModeRoad::GetToolIndex()
{
	return Tools.Find(CurrentTool);
}

void FEdModeRoad::SetToolIndex(int Index)
{
	static_cast<FRoadTool*>(Tools[Index])->Reset();
	SetCurrentTool(Tools[Index]);
	FEditorViewportClient* Client = GLevelEditorModeTools().GetFocusedViewportClient();
	Client->Invalidate();
}

void FEdModeRoad::OnUndo(const FTransactionContext& InTransactionContext, bool bSucceeded)
{
	Scene->Rebuild();
}

void FEdModeRoad::OnRedo(const FTransactionContext& InTransactionContext, bool bSucceeded)
{
	Scene->Rebuild();
}

void FEdModeRoad::Enter()
{
	FEdMode::Enter();
	UTransBuffer* TransBuffer = CastChecked<UTransBuffer>(GEditor->Trans);
	TransBuffer->OnUndo().AddRaw(this, &FEdModeRoad::OnUndo);
	TransBuffer->OnRedo().AddRaw(this, &FEdModeRoad::OnRedo);
	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FRoadEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
		Toolkit->SetCurrentPalette(TEXT("Road"));
	}
	GLevelEditorModeTools().SetCoordSystem(COORD_Local);
	Scene = Cast<ARoadScene>(UGameplayStatics::GetActorOfClass(GetWorld(), ARoadScene::StaticClass()));
	if (!Scene)
		Scene = GetWorld()->SpawnActor<ARoadScene>();
}

void FEdModeRoad::Exit()
{
	Scene = nullptr;
	SelectedRoad = nullptr;
	SelectedJunction = nullptr;
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();
	UTransBuffer* TransBuffer = CastChecked<UTransBuffer>(GEditor->Trans);
	TransBuffer->OnUndo().RemoveAll(this);
	TransBuffer->OnRedo().RemoveAll(this);
	FEdMode::Exit();
}

void FEdModeRoad::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	const FScopedTransaction Transaction(LOCTEXT("NotifyPreChange", "NotifyPreChange"));
	static_cast<FRoadTool*>(CurrentTool)->NotifyPreChange(PropertyAboutToChange);
}

void FEdModeRoad::PostUndo()
{
	static_cast<FRoadTool*>(CurrentTool)->Reset();
}

bool FEdModeRoad::GetCursor(EMouseCursor::Type& OutCursor) const
{
	FEditorViewportClient* Client = GLevelEditorModeTools().GetFocusedViewportClient();
	HHitProxy* HitProxy = Client->Viewport->GetHitProxy(Client->GetCachedMouseX(), Client->GetCachedMouseY());
	if (HitProxy)
	{
		OutCursor = HitProxy->IsA(HActor::StaticGetType()) ? EMouseCursor::Default : HitProxy->GetMouseCursor();
		return true;
	}
	return false;
}

bool FEdModeRoad::ShouldDrawWidget() const
{
	if (((FRoadTool*)CurrentTool)->ShouldDrawWidget())
		return true;
	return FEdMode::ShouldDrawWidget();
}

FVector FEdModeRoad::GetWidgetLocation() const
{
	return ((FRoadTool*)CurrentTool)->GetWidgetLocation();
}

bool FEdModeRoad::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	return ((FRoadTool*)CurrentTool)->GetCustomDrawingCoordinateSystem(InMatrix, InData);
}

EAxisList::Type FEdModeRoad::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const
{
	if (InWidgetMode != UE::Widget::WM_Translate)
		return EAxisList::None;
	return ((FRoadTool*)CurrentTool)->GetWidgetAxisToDraw();
}

bool FEdModeRoad::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (((FRoadTool*)CurrentTool)->HandleClick(InViewportClient, HitProxy, Click))
		return true;
#if 0
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		TSharedPtr<SEditorViewport> ViewportWidget = InViewportClient->GetEditorViewportWidget();
		TSharedPtr<SWidget> MenuContents = ((FRoadTool*)CurrentTool)->GenerateContextMenu();
		FSlateApplication::Get().PushMenu(ViewportWidget.ToSharedRef(), FWidgetPath(), MenuContents.ToSharedRef(), Click.GetClickPos(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		return true;
	}
#endif
	return FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}
#undef LOCTEXT_NAMESPACE
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "OSMVisualizer.h"
#include "StructDialog.h"

IMPLEMENT_HIT_PROXY(HOSMVisProxy, HComponentVisProxy);

#define LOCTEXT_NAMESPACE "RoadBuilder"
void FOSMVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (!Component)
		return;
	if (AOSMActor* OSM = Cast<AOSMActor>(Component->GetOwner()))
	{
		for (TPair<uint64, FOSMWay>& KV : OSM->Ways)
		{
			if (KV.Value.IsDrivable())
			{
				FPolyline& Curve = KV.Value.Curve;
				PDI->SetHitProxy(new HOSMVisProxy(Component, KV.Key));
				for (int i = 0; i < Curve.Points.Num(); i++)
				{
					const FVector& Start = Curve.Points[i].Pos;
					PDI->DrawPoint(Start, SelectedWay == KV.Key ? FColor::Blue : FColor::Red, 4, SDPG_Foreground);
					if (i < Curve.Points.Num() - 1)
					{
						const FVector& End = Curve.Points[i + 1].Pos;
						PDI->DrawLine(Start, End, SelectedWay == KV.Key ? FColor::Blue : FColor::Red, SDPG_Foreground, 0);
					}
				}
			}
		}
		FBox2D Bounds = OSM->GetTileBounds();
		FVector Points[4] =
		{
			FVector(Bounds.Min.X, Bounds.Min.Y, 0),
			FVector(Bounds.Max.X, Bounds.Min.Y, 0),
			FVector(Bounds.Max.X, Bounds.Max.Y, 0),
			FVector(Bounds.Min.X, Bounds.Max.Y, 0),
		};
		PDI->SetHitProxy(nullptr);
		for (int i = 0; i < 4; i++)
			PDI->DrawLine(Points[i], Points[(i + 1) % 4], FColor::Yellow, SDPG_Foreground, 0);
	}
}

bool FOSMVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	SelectedComponent = (UOSMComponent*)Cast<UOSMComponent>(VisProxy->Component.Get());
	if (SelectedComponent)
	{
		if (VisProxy->IsA(HOSMVisProxy::StaticGetType()))
		{
			HOSMVisProxy* Proxy = (HOSMVisProxy*)VisProxy;
			SelectedWay = Proxy->WayId;
		}
		return true;
	}
	SelectedWay = 0;
	return false;
}

TSharedPtr<SWidget> FOSMVisualizer::GenerateContextMenu() const
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("ConvertRoad", "Convert to Road"),
		LOCTEXT("ConvertRoadtipText", ""),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateRaw(this, &FOSMVisualizer::ConvertToRoadClicked)
		)
	);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("Properties", "Properties"),
		LOCTEXT("PropertiestipText", ""),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateRaw(this, &FOSMVisualizer::PropertiesClicked)
		)
	);
	return MenuBuilder.MakeWidget();
}

void FOSMVisualizer::ConvertToRoadClicked() const
{
	if (AOSMActor* OSM = Cast<AOSMActor>(SelectedComponent->GetOwner()))
		OSM->CreateRoad(SelectedWay);
}

void FOSMVisualizer::PropertiesClicked() const
{
	if (AOSMActor* OSM = Cast<AOSMActor>(SelectedComponent->GetOwner()))
		IsDataDialogProceed(OSM->Ways[SelectedWay]);
}
#undef LOCTEXT_NAMESPACE
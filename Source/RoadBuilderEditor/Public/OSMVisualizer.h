// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ComponentVisualizer.h"
#include "OSMActor.h"

struct HOSMVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();
	HOSMVisProxy(const UActorComponent* InComponent, uint64 Id) : HComponentVisProxy(InComponent, HPP_Wireframe), WayId(Id) {}
	uint64 WayId;
};

class FOSMVisualizer : public FComponentVisualizer
{
private:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual TSharedPtr<SWidget> GenerateContextMenu() const;
	void ConvertToRoadClicked() const;
	void PropertiesClicked() const;
	UOSMComponent* SelectedComponent = nullptr;
	uint64 SelectedWay = 0;
};
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "EditorModeTools.h"
#include "EditorModeManager.h"
#include "EdMode.h"
#include "RoadActor.h"
#include "RoadScene.h"

class SRoadEdit;

struct HRoadProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	HRoadProxy(ARoadActor* R, int I = INDEX_NONE) : HHitProxy(HPP_Wireframe), Road(R), Index(I) {}
	virtual EMouseCursor::Type GetMouseCursor() { return EMouseCursor::Crosshairs; }
	ARoadActor* Road;
	int Index;	//INDEX_NONE indicates the whole road
};

struct HGroundProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	HGroundProxy(AGroundActor* G, int I = INDEX_NONE) : HHitProxy(HPP_Wireframe), Ground(G), Index(I) {}
	virtual EMouseCursor::Type GetMouseCursor() { return EMouseCursor::Crosshairs; }
	AGroundActor* Ground;
	int Index;
};

struct HJunctionProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	HJunctionProxy(AJunctionActor* J, int I = INDEX_NONE, int S = INDEX_NONE) : HHitProxy(HPP_Wireframe), Junction(J), Index(I), SubId(S) {}
	virtual EMouseCursor::Type GetMouseCursor() { return EMouseCursor::Crosshairs; }
	AJunctionActor* Junction;
	int Index;	//INDEX_NONE indicates the whole line
	int SubId;
};

struct HRoadCurveProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	HRoadCurveProxy(URoadCurve* C, int I = INDEX_NONE) : HHitProxy(HPP_Wireframe), Curve(C), Index(I) {}
	virtual EMouseCursor::Type GetMouseCursor() { return EMouseCursor::Crosshairs; }
	URoadCurve* Curve;
	int Index;	//INDEX_NONE indicates the whole line
};

struct HRoadMarkingProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	HRoadMarkingProxy(URoadMarking* M, int I = INDEX_NONE, int S = INDEX_NONE) : HHitProxy(HPP_Wireframe), Marking(M), Index(I), SubId(S) {}
	virtual EMouseCursor::Type GetMouseCursor() { return EMouseCursor::Crosshairs; }
	URoadMarking* Marking;
	int Index;	//INDEX_NONE indicates the whole object
	int SubId;
};

class FRoadTool : public FModeTool
{
public:
	static inline const float Size_Point = 16.f;
	static inline const float Thickness_Road = 2.f;
	static inline const float Thickness_Line = 1.f;
	static inline const float DepthBias_Select = 10.f;
	static inline const FColor Color_Road = FColor(128, 128, 255);
	static inline const FColor Color_Line = FColor(0, 128, 0);
	static inline const FColor Color_Grey = FColor(128, 128, 128);
	static inline const FColor Color_Select = FColor::Red;
	virtual bool ShouldDrawWidget() const { return false; }
	virtual FVector GetWidgetLocation() const { return FVector::ZeroVector; }
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData) { return false; }
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::None; }
//	virtual TSharedPtr<SWidget> GenerateContextMenu() { return TSharedPtr<SWidget>(); }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) { return false; }
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool EndModify();
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) {}
	virtual void Reset();

	ARoadScene* GetScene() const;
	ARoadActor*& GetSelectedRoad() const;
	AGroundActor*& GetSelectedGround() const;
	AJunctionActor*& GetSelectedJunction() const;
	SRoadEdit* GetEditWidget() const;
	FRay GetRay(FEditorViewportClient* ViewportClient) const;
	FVector LineTrace(const FRay& Ray, AActor* IgnoredActor = nullptr) const;
	FVector LineTrace(FEditorViewportClient* ViewportClient, AActor* IgnoredActor = nullptr) const;
	void SelectParent();
	bool HandleClickRoad(HHitProxy* HitProxy, const FViewportClick& Click, int* PointIndex = nullptr);
	bool HandleClickJunction(HHitProxy* HitProxy, const FViewportClick& Click, int* GateIndex = nullptr, int* LinkIndex = nullptr);
	void DrawCurve(FPrimitiveDrawInterface* PDI, const FPolyline& Curve, FColor Color, float Thickness, float DepthBias = 0);
	void DrawPoint(FPrimitiveDrawInterface* PDI, URoadCurve* Curve, double Dist, FColor Color);
	void DrawDivider(FPrimitiveDrawInterface* PDI, URoadLane* Lane, double Dist, FColor Color);
	void DrawRoads(FPrimitiveDrawInterface* PDI, bool DrawLinks);
	void DrawJunction(FPrimitiveDrawInterface* PDI, AJunctionActor* Junction, FColor Color);
	void DrawJunctions(FPrimitiveDrawInterface* PDI);
	bool LazyRebuild = false;
};

class FRoadTool_File : public FRoadTool
{
public:
};

class FRoadTool_RoadPlan : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return PointIndex != INDEX_NONE; }
	virtual FVector GetWidgetLocation() const;
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::XY; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		GetSelectedRoad()->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		GetSelectedRoad() = nullptr;
		PointIndex = INDEX_NONE;
	}
	int PointIndex = INDEX_NONE;
};

class FRoadTool_RoadHeight : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return PointIndex != INDEX_NONE; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::X; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		GetSelectedRoad()->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		PointIndex = INDEX_NONE;
	}
	int PointIndex = INDEX_NONE;
};

class FRoadTool_RoadChop : public FRoadTool
{
public:
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		GetSelectedRoad()->Modify();
	}
};

class FRoadTool_RoadSplit : public FRoadTool
{
public:
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		GetSelectedRoad()->Modify();
	}
};

class FRoadTool_JunctionLink : public FRoadTool
{
public:
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		GetSelectedJunction()->Gates[GateIndex].Links[LinkIndex].Road->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		GateIndex = INDEX_NONE;
		LinkIndex = INDEX_NONE;
	}
	int GateIndex = INDEX_NONE;
	int LinkIndex = INDEX_NONE;
};

class FRoadTool_LaneEdit : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return SegmentIndex != INDEX_NONE; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::X; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		CurrentLane->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		CurrentLane = nullptr;
		SegmentIndex = INDEX_NONE;
	}
	URoadLane* CurrentLane = nullptr;
	int SegmentIndex = INDEX_NONE;
};

class FRoadTool_LaneCarve : public FRoadTool
{
public:
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void Reset()
	{
		FRoadTool::Reset();
		StartUV = FVector2D(-1, 0);
	}
	FVector2D StartUV = FVector2D(-1, 0);
};

class FRoadTool_LaneWidth : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return OffsetIndex != INDEX_NONE; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::XY; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		CurrentBoundary->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		CurrentBoundary = nullptr;
		OffsetIndex = INDEX_NONE;
	}
	URoadBoundary* CurrentBoundary = nullptr;
	int OffsetIndex = INDEX_NONE;
};

class FRoadTool_MarkingLane : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return SegmentIndex != INDEX_NONE; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::X; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		CurrentBoundary->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		CurrentBoundary = nullptr;
		SegmentIndex = INDEX_NONE;
	}
	URoadBoundary* CurrentBoundary = nullptr;
	int SegmentIndex = INDEX_NONE;
};

class FRoadTool_MarkingPoint : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return CurrentMarking != nullptr; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::XY; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		CurrentMarking->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		CurrentMarking = nullptr;
	}
	UMarkingPoint* CurrentMarking = nullptr;
};

class FRoadTool_MarkingCurve : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return CurrentMarking != nullptr; }
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::XY; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange)
	{
		CurrentMarking->Modify();
	}
	virtual void Reset()
	{
		FRoadTool::Reset();
		CurrentMarking = nullptr;
		PointIndex = INDEX_NONE;
		SubIndex = 0;
	}
	UMarkingCurve* CurrentMarking = nullptr;
	int PointIndex = INDEX_NONE;
	int SubIndex = 0;
};

class FRoadTool_GroundEdit : public FRoadTool
{
public:
	virtual bool ShouldDrawWidget() const { return PointIndex != INDEX_NONE && GetSelectedGround()->Points[PointIndex].Road == nullptr; }
	virtual FVector GetWidgetLocation() const;
	virtual EAxisList::Type GetWidgetAxisToDraw() const { return EAxisList::XY; }
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI);
	virtual void Reset()
	{
		FRoadTool::Reset();
		GetSelectedGround() = nullptr;
		PointIndex = INDEX_NONE;
	}
	int PointIndex = INDEX_NONE;
};

class FRoadTool_Settings : public FRoadTool
{
public:
};

class FEdModeRoad : public FEdMode, public FNotifyHook
{
public:
	enum ToolType
	{
		File,
		RoadPlan,
		RoadHeight,
		RoadChop,
		RoadSplit,
		JunctionLink,
		LaneEdit,
		LaneCarve,
		LaneWidth,
		MarkingLane,
		MarkingPoint,
		MarkingCurve,
		GroundEdit,
		Settings,
	};
	static FEditorModeID GetModeID()
	{
		return FEditorModeID(TEXT("EM_Road"));
	}
	static FEdModeRoad* Get()
	{
		return (FEdModeRoad*)GLevelEditorModeTools().GetActiveMode(GetModeID());
	}

	FEdModeRoad();
	virtual ~FEdModeRoad();

	int GetToolIndex();
	void SetToolIndex(int Index);

	void OnUndo(const FTransactionContext& InTransactionContext, bool bSucceeded);
	void OnRedo(const FTransactionContext& InTransactionContext, bool bSucceeded);

	/** FEdMode: Called when the mode is entered */
	virtual void Enter() override;

	/** FEdMode: Called when the mode is exited */
	virtual void Exit() override;
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange);
	virtual void PostUndo();
	virtual bool GetCursor(EMouseCursor::Type& OutCursor) const;
	virtual bool AllowWidgetMove() { return true; }
	virtual bool ShouldDrawWidget() const;
	virtual FVector GetWidgetLocation() const;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData);
	virtual EAxisList::Type GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override { return false; }

	ARoadScene* Scene = nullptr;
	ARoadActor* SelectedRoad = nullptr;
	AGroundActor* SelectedGround = nullptr;
	AJunctionActor* SelectedJunction = nullptr;
};
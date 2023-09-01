// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "EditorExtensions.h"
#include "LevelEditor.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IContentBrowserSingleton.h"
#include "EngineUtils.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "Kismet/GameplayStatics.h"
#include "ConstrainedDelaunay2.h"
#include "RoadScene.h"
#include "RoadMesh.h"
#include "IContentBrowserDataModule.h"
#include "ContentBrowserDataSubsystem.h"
#include "Settings.h"
#include "StructDialog.h"

THIRD_PARTY_INCLUDES_START
#include <stdio.h>
#include <string.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION	
#include "ThirdParty/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "ThirdParty/nanosvg/src/nanosvgrast.h"
THIRD_PARTY_INCLUDES_END

#define LOCTEXT_NAMESPACE "RoadBuilder"
void FEditorExtensions::InstallHooks()
{
	FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelEditorMenuExtenderDelegate = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateSP(this, &FEditorExtensions::OnExtendLevelEditorMenu);
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(LevelEditorMenuExtenderDelegate);
	LevelEditorExtenderDelegateHandle = MenuExtenders.Last().GetHandle();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
//	FContentBrowserMenuExtender_SelectedAssets AssetDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateSP(this, &FEditorExtensions::OnSelectedAssetsContextMenu);
	FContentBrowserMenuExtender_SelectedPaths PathDelegate = FContentBrowserMenuExtender_SelectedPaths::CreateSP(this, &FEditorExtensions::OnSelectedPathsContextMenu);
//	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(AssetDelegate);
	ContentBrowserModule.GetAllPathViewContextMenuExtenders().Add(PathDelegate);
//	AssetDelegateHandle = AssetDelegate.GetHandle();
	PathDelegateHandle = PathDelegate.GetHandle();

	UToolMenu* ContextMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AddNewContextMenu");
	FToolMenuSection& ContextMenuSection = ContextMenu->FindOrAddSection("ContentBrowserGetContent");
	ContextMenuSection.AddDynamicEntry("ImportSVG", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
	{
		UContentBrowserDataMenuContext_AddNewMenu* AddNewMenuContext = InSection.FindContext<UContentBrowserDataMenuContext_AddNewMenu>();
		if (AddNewMenuContext && AddNewMenuContext->bCanBeModified && AddNewMenuContext->bContainsValidPackagePath)
		{
			FName ConvertedPath;
			IContentBrowserDataModule::Get().GetSubsystem()->TryConvertVirtualPath(AddNewMenuContext->SelectedPaths[0], ConvertedPath);
			InSection.AddMenuEntry(
				"ImportSVG",
				LOCTEXT("ImportSVG", "Import SVG"),
				LOCTEXT("ImportSVGTooltip", "Import SVG"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FEditorExtensions::OnImportSVGClicked, ConvertedPath.ToString()), FCanExecuteAction())
			);
		}
	}));
}

void FEditorExtensions::RemoveHooks()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	/*
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().RemoveAll([&](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
	{
		return Delegate.GetHandle() == AssetDelegateHandle;
	});*/
	ContentBrowserModule.GetAllPathViewContextMenuExtenders().RemoveAll([&](const FContentBrowserMenuExtender_SelectedPaths& Delegate)
	{
		return Delegate.GetHandle() == PathDelegateHandle;
	});

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll([&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate)
	{
		return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle;
	});
}

TSharedRef<FExtender> FEditorExtensions::OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors)
{
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension("ActorControl", EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateSP(this, &FEditorExtensions::CreateMenuEntries, SelectedActors));
	return Extender;
}

TSharedRef<FExtender> FEditorExtensions::OnSelectedPathsContextMenu(const TArray<FString>& Selection)
{
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension("PathContextBulkOperations", EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateSP(this, &FEditorExtensions::OnExtendSelectedPathsMenu, Selection));
	return Extender;
}

void FEditorExtensions::CreateMenuEntries(FMenuBuilder& MenuBuilder, TArray<AActor*> Actors)
{
	MenuBuilder.BeginSection("Road Builder", LOCTEXT("RoadBuilder", "Road Builder"));
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IsolateRoads", "Isolate Roads"),
		LOCTEXT("IsolateRoadstipText", ""),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateSP(this, &FEditorExtensions::OnIsolateRoadsClicked, Actors)
		)
	);
	MenuBuilder.EndSection();
}

void FEditorExtensions::OnExtendSelectedPathsMenu(FMenuBuilder& MenuBuilder, TArray<FString> Paths)
{
	MenuBuilder.BeginSection("Road Builder", LOCTEXT("RoadBuilder", "Road Builder"));
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("MarkDirty", "Mark Dirty"),
		LOCTEXT("MarkDirtyTooltipText", "Mark Dirty"),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateSP(this, &FEditorExtensions::OnMarkDirtyClicked, Paths)
		)
	);
	MenuBuilder.EndSection();
}

void FEditorExtensions::OnIsolateRoadsClicked(TArray<AActor*> Actors)
{
	if (ARoadScene* Scene = Cast<ARoadScene>(UGameplayStatics::GetActorOfClass(GWorld, ARoadScene::StaticClass())))
	{
		for (int i = 0; i < Scene->Roads.Num();)
		{
			if (Actors.Contains(Scene->Roads[i]))
				i++;
			else
				Scene->DestroyRoad(Scene->Roads[i]);
		}
		Scene->Rebuild();
	}
}

void FEditorExtensions::OnMarkDirtyClicked(TArray<FString> Paths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.bIncludeOnlyOnDiskAssets = true;
	for (FString& Path : Paths)
	{
		Filter.PackagePaths.Emplace(*Path);
	}
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);
	for (FAssetData& Asset : AssetList)
	{
		Asset.GetAsset()->MarkPackageDirty();
	}
}

using namespace UE::Geometry;
void FEditorExtensions::OnImportSVGClicked(FString Path)
{
	TArray<FString> Files;
	if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
	{
		if (DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			LOCTEXT("ImportPropsTitle", "Import Props").ToString(),
			TEXT(""),
			TEXT(""),
			TEXT("SVG File (*.svg)|*.svg"),
			EFileDialogFlags::None,
			Files))
		{
			USettings_SVGImport* Settings = GetMutableDefault<USettings_SVGImport>();
			if (IsDataDialogProceed(Settings))
			{
				FString Content;
				FString Name = FPaths::GetBaseFilename(Files[0]);
				if (FFileHelper::LoadFileToString(Content, *Files[0]))
				{
					NSVGimage* Image = nsvgParse(TCHAR_TO_ANSI(*Content), "px", 96.0f);
					if (Image)
					{
						FConstrainedDelaunay2d Triangulator;
						Triangulator.FillRule = FConstrainedDelaunay2d::EFillRule::NonZero;
						NSVGpath* path = Image->shapes->paths;
						while (path)
						{
							FPolygon2d Polygon;
							for (int i = 0; i < path->npts; i++)
								Polygon.AppendVertex(FVector2D(path->pts[i * 2 + 0], path->pts[i * 2 + 1]));
							FGeneralPolygon2d GeneralPolygon(Polygon);
							Triangulator.Add(GeneralPolygon);
							path = path->next;
						}
						bool bTriangulationSuccess = Triangulator.Triangulate();
						nsvgDelete(Image);
						FBox2D Box2D(Triangulator.Vertices);
						FTransform Trans(FRotator(0, Settings->Rotation, 0), FVector::ZeroVector, FVector(Settings->Scale.X, Settings->Scale.Y, 1));
						FBox Box = FBox(FVector(Box2D.Min, 0), FVector(Box2D.Max, 0)).TransformBy(Trans);
						if (Settings->ResetOrigin)
						{
							double X = FMath::Lerp(Box.Min.X, Box.Max.X, Settings->Anchor.X);
							double Y = FMath::Lerp(Box.Min.Y, Box.Max.Y, Settings->Anchor.Y);
							Trans.SetTranslation(FVector(-X, -Y, 0));
						}
						for (FVector2D& Vertex : Triangulator.Vertices)
							Vertex = FVector2D(Trans.TransformPosition(FVector(Vertex, 0)));
						if (Settings->Scale.X * Settings->Scale.Y < 0)
						{
							for (FIndex3i& Index : Triangulator.Triangles)
								Swap(Index.B, Index.C);
						}
						FStaticRoadMesh Builder;
						Builder.AddTriangles(Cast<UMaterialInterface>(Settings->Material.LoadSynchronous()), Triangulator.Triangles, Triangulator.Vertices, FVector::UpVector);
						UPackage* Package = CreatePackage(*(Path / Name));
						UStaticMesh* NewMesh = Builder.CreateMesh(Package, FName(Name), RF_Public | RF_Standalone);
						Package->SetDirtyFlag(true);

						FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
						ContentBrowserModule.Get().SetSelectedPaths({ ContentBrowserModule.Get().GetCurrentPath().GetInternalPathString() }, true);
					}
				}
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE
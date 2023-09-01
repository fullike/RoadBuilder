// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

class FEditorExtensions : public TSharedFromThis<FEditorExtensions>
{
public:
	void InstallHooks();
	void RemoveHooks();
	TSharedRef<FExtender> OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors);
	TSharedRef<FExtender> OnSelectedPathsContextMenu(const TArray<FString>& Selection);

	void CreateMenuEntries(FMenuBuilder& MenuBuilder, TArray<AActor*> Actors);
	void OnExtendSelectedPathsMenu(FMenuBuilder& MenuBuilder, TArray<FString> Paths);

	void OnIsolateRoadsClicked(TArray<AActor*> Actors);
	void OnMarkDirtyClicked(TArray<FString> Paths);
	void OnImportSVGClicked(FString Path);

	FDelegateHandle LevelEditorExtenderDelegateHandle;
	FDelegateHandle PathDelegateHandle;
};
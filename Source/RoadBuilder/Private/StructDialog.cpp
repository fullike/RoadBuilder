// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "StructDialog.h"
#if WITH_EDITOR
#include "SlateOptMacros.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#define LOCTEXT_NAMESPACE "RoadBuilder"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SObjectDialog::Construct(const FArguments& InArgs)
{
	Window = InArgs._WidgetWindow;
	bShouldProceed = false;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NotifyHook = GUnrealEd;
	ObjectDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.FillHeight(1.0f)
		[
			ObjectDetailsView.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.FillHeight(0.2f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString(TEXT("OK")))
				.OnClicked(this, &SObjectDialog::OnProceed)
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked(this, &SObjectDialog::OnCancel)
			]
		]
	];
	ObjectDetailsView->SetObject(InArgs._Object);
}

FReply SObjectDialog::OnProceed()
{
	bShouldProceed = true;
	if (Window.IsValid())
		Window.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SObjectDialog::OnCancel()
{
	bShouldProceed = false;
	if (Window.IsValid())
		Window.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

void SStructDialog::Construct(const FArguments& InArgs)
{
	Window = InArgs._WidgetWindow;
	bShouldProceed = false;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NotifyHook = GUnrealEd;
	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = true;
	StructureViewArgs.bShowInterfaces = true;
	StructDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.FillHeight(1.0f)
		[
			StructDetailsView->GetWidget().ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.FillHeight(0.2f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString(TEXT("OK")))
				.OnClicked(this, &SStructDialog::OnProceed)
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked(this, &SStructDialog::OnCancel)
			]
		]
	];
	StructDetailsView->SetStructureData(MakeShareable(new FStructOnScope(InArgs._Struct, InArgs._Data)));
}

FReply SStructDialog::OnProceed()
{
	bShouldProceed = true;
	if (Window.IsValid())
		Window.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SStructDialog::OnCancel()
{
	bShouldProceed = false;
	if (Window.IsValid())
		Window.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
#endif
// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#if WITH_EDITOR
#include "Widgets/SCompoundWidget.h"
#include "IStructureDetailsView.h"
#include "Interfaces/IMainFrameModule.h"

class ROADBUILDER_API SObjectDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SObjectDialog) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(UObject*, Object)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);
	bool ShouldProceed() const { return bShouldProceed; }
	FReply OnProceed();
	FReply OnCancel();
	bool bShouldProceed;
	TWeakPtr< SWindow > Window;
	TSharedPtr<IDetailsView> ObjectDetailsView;
};

class ROADBUILDER_API SStructDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStructDialog) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(UStruct*, Struct)
		SLATE_ARGUMENT(uint8*, Data)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);
	bool ShouldProceed() const { return bShouldProceed; }
	FReply OnProceed();
	FReply OnCancel();
	bool bShouldProceed;
	TWeakPtr< SWindow > Window;
	TSharedPtr<IStructureDetailsView> StructDetailsView;
};

template<class T>
bool IsDataDialogProceed(T* Obj)
{
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(T::StaticClass()->GetName()))
		.SizingRule(ESizingRule::Autosized);
	TSharedPtr<SObjectDialog> OptionsWindow;
	Window->SetContent
	(
		SAssignNew(OptionsWindow, SObjectDialog)
		.WidgetWindow(Window)
		.Object(Obj)
	);
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
	return OptionsWindow->ShouldProceed();
}

template<class T>
bool IsDataDialogProceed(T& Data)
{
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(T::StaticStruct()->GetName()))
		.SizingRule(ESizingRule::Autosized);
	TSharedPtr<SStructDialog> OptionsWindow;
	Window->SetContent
	(
		SAssignNew(OptionsWindow, SStructDialog)
		.WidgetWindow(Window)
		.Struct(T::StaticStruct())
		.Data((uint8*)&Data)
	);
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
	return OptionsWindow->ShouldProceed();
}
#endif
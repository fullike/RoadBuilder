// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "LaneShapeFactory.generated.h"
UCLASS()
class ULaneShapeFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	// UFactory interface
//	virtual bool FactoryCanImport(const FString& Filename) override;
//	virtual UObject* ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled) override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
//	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	// End of UFactory interface
//	FString GetDefaultNewAssetName() const override;
};

class FLaneShapeTypeActions : public FAssetTypeActions_Base
{
public:
	FLaneShapeTypeActions(EAssetTypeCategories::Type InAssetCategory) :MyAssetCategory(InAssetCategory)
	{
	}
	// IAssetTypeActions interface
	virtual FText GetName() const override;

	virtual FColor GetTypeColor() const override
	{
		return FColor::Cyan;
	}
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override
	{
		return MyAssetCategory;
	}
	// End of IAssetTypeActions interface

	EAssetTypeCategories::Type MyAssetCategory;
};
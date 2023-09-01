// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "RoadPropsFactory.generated.h"
UCLASS()
class URoadPropsFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

class FRoadPropsTypeActions : public FAssetTypeActions_Base
{
public:
	FRoadPropsTypeActions(EAssetTypeCategories::Type InAssetCategory) :MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override { return FColor::Cyan; }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	EAssetTypeCategories::Type MyAssetCategory;
};
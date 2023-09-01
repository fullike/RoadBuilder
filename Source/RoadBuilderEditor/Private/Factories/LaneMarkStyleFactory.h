// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "LaneMarkStyleFactory.generated.h"

UCLASS()
class ULaneMarkStyleFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UCrosswalkStyleFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UPolygonMarkStyleFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

class FLaneMarkStyleTypeActions : public FAssetTypeActions_Base
{
public:
	FLaneMarkStyleTypeActions(EAssetTypeCategories::Type InAssetCategory) :MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const { return FColor::Cyan; }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() { return MyAssetCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> SubMenus = { NSLOCTEXT("AssetTypeActions", "MarkStyles", "MarkStyles") };
		return SubMenus;
	}
	EAssetTypeCategories::Type MyAssetCategory;
};

class FCrosswalkStyleTypeActions : public FAssetTypeActions_Base
{
public:
	FCrosswalkStyleTypeActions(EAssetTypeCategories::Type InAssetCategory) :MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const { return FColor::Cyan; }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() { return MyAssetCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> SubMenus = { NSLOCTEXT("AssetTypeActions", "MarkStyles", "MarkStyles") };
		return SubMenus;
	}
	EAssetTypeCategories::Type MyAssetCategory;
};

class FPolygonMarkStyleTypeActions : public FAssetTypeActions_Base
{
public:
	FPolygonMarkStyleTypeActions(EAssetTypeCategories::Type InAssetCategory) :MyAssetCategory(InAssetCategory) {}
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const { return FColor::Cyan; }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() { return MyAssetCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> SubMenus = { NSLOCTEXT("AssetTypeActions", "MarkStyles", "MarkStyles") };
		return SubMenus;
	}
	EAssetTypeCategories::Type MyAssetCategory;
};
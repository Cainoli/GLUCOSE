// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "System/GameplayTagStack.h"
#include "GameplayTagContainer.h"
#include "LyraInventoryItemInstance.h"
#include "LyraInventoryFilter.generated.h"

class ULyraInventoryItemInstance;
struct FGameplayTag;

/**
 * Base class for inventory filters
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class LYRAGAME_API ULyraInventoryFilter : public UObject
{
    GENERATED_BODY()

public:
    ULyraInventoryFilter() {}

    // Main filter function that determines if an item passes the filter
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory")
    bool PassesFilter(const ULyraInventoryItemInstance* ItemInstance) const;
    virtual bool PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const;
};

/**
 * Filter that checks if an item has a specific gameplay tag
 */
UCLASS(Blueprintable)
class LYRAGAME_API ULyraInventoryFilter_HasTag : public ULyraInventoryFilter
{
    GENERATED_BODY()

public:
    // The gameplay tag to check for
    UPROPERTY(EditAnywhere, Category = "Filter")
    FGameplayTag RequiredTag;

    virtual bool PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const override;
};

/**
 * Filter that checks item definition class
 */
UCLASS(Blueprintable)
class LYRAGAME_API ULyraInventoryFilter_ItemDefinition : public ULyraInventoryFilter
{
    GENERATED_BODY()

public:
    // The item definition class to filter by
    UPROPERTY(EditAnywhere, Category = "Filter")
    TSubclassOf<class ULyraInventoryItemDefinition> ItemDefinitionClass;

    virtual bool PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const override;
};

/**
* Composite filter that combines multiple filters using logical operations
*/
UCLASS(Blueprintable)
class LYRAGAME_API ULyraInventoryFilter_Composite : public ULyraInventoryFilter
{
    GENERATED_BODY()

public:

    // List of filters to combine
    UPROPERTY(EditAnywhere, Instanced, Category = "Filter")
    TArray<TObjectPtr<ULyraInventoryFilter>> Filters;

    // If true, ALL filters must pass (AND operation)
    // If false, ANY filter must pass (OR operation)
    UPROPERTY(EditAnywhere, Category = "Filter")
    bool bRequireAllFilters = true;

    virtual bool PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const override;
};

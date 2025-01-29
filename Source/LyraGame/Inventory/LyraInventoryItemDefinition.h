// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "LyraInventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class ULyraInventoryItemInstance;
struct FFrame;

//////////////////////////////////////////////////////////////////////

// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class LYRAGAME_API ULyraInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const {}
};

//////////////////////////////////////////////////////////////////////

/**
 * ULyraInventoryItemDefinition
 */
UCLASS(Blueprintable, Const, Abstract)
class ULyraInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	ULyraInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine=true), Category=Display)
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<ULyraInventoryItemFragment>> Fragments;
	
	/**
	 Maximum number of items that can be stacked together
	 * 0 means unlimited stack size
	 * 1 means items cannot be stacked (each item will be in its own stack)
	 * >1 means items can be stacked up to this limit
	*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Display, meta=(ClampMin="0"))
    int32 MaxStackSize = 1;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Display, meta=(ClampMin="0"))
    float Weight = 0.0f;

public:
	const ULyraInventoryItemFragment* FindFragmentByClass(TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const;
};

/** @deprecated Use ULyraInventorySubsystem instead */
UCLASS(meta=(DeprecatedNode, DeprecationMessage="Use ULyraInventorySubsystem instead"))
class ULyraInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

    /** @deprecated Use ULyraInventorySubsystem::FindItemDefinitionFragment instead */
    UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use ULyraInventorySubsystem::FindItemDefinitionFragment instead", DeterminesOutputType=FragmentClass))
    static const ULyraInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, TSubclassOf<ULyraInventoryItemFragment> FragmentClass);
};

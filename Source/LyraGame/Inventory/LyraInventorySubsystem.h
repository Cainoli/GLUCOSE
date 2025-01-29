// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "LyraInventorySubsystem.generated.h"

class ULyraInventoryItemDefinition;
class ULyraInventoryItemFragment;
class ULyraInventoryItemInstance;

/**
* Subsystem responsible for managing inventory-related functionality across the game
*/
UCLASS()
class LYRAGAME_API ULyraInventorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Begin USubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // End USubsystem

    /** Find a fragment in an item definition by class */
    UFUNCTION(BlueprintCallable, Category="Lyra|Inventory", meta=(DeterminesOutputType=FragmentClass))
    const ULyraInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const;

    /** Register an inventory component with the subsystem */
    void RegisterInventoryComponent(class ULyraInventoryManagerComponent* Component);

    /** Unregister an inventory component from the subsystem */
    void UnregisterInventoryComponent(class ULyraInventoryManagerComponent* Component);

    /** Get all registered inventory components */
    UFUNCTION(BlueprintCallable, Category="Lyra|Inventory")
    const TArray<class ULyraInventoryManagerComponent*>& GetAllInventoryComponents() const { return RegisteredInventoryComponents; }

    /** Create a new inventory item instance */
    UFUNCTION(BlueprintCallable, Category="Lyra|Inventory")
    ULyraInventoryItemInstance* CreateInventoryItemInstance(UObject* Outer, TSubclassOf<ULyraInventoryItemDefinition> ItemDef);

    /** Initialize a newly created inventory item instance */
    void InitializeItemInstance(ULyraInventoryItemInstance* Instance, TSubclassOf<ULyraInventoryItemDefinition> ItemDef);
	
	/** Validate if an inventory item instance is still valid */
    UFUNCTION(BlueprintCallable, Category="Lyra|Inventory")
    bool IsValidItemInstance(const ULyraInventoryItemInstance* Instance) const;
	
    /** Get all valid items from an inventory entry array */
    UFUNCTION(BlueprintCallable, Category="Lyra|Inventory")
    TArray<ULyraInventoryItemInstance*> GetValidItemsFromList(const TArray<struct FLyraInventoryEntry>& Entries) const;

private:
    /** List of all active inventory components */
    UPROPERTY()
    TArray<TObjectPtr<ULyraInventoryManagerComponent>> RegisteredInventoryComponents;
	
    /** Cache of created inventory items for proper lifecycle management */
    UPROPERTY()
    TArray<TObjectPtr<ULyraInventoryItemInstance>> ManagedItems;
	
	/** Remove invalid items from management */
    void CleanupInvalidItems();
};

// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventorySubsystem.h"
#include "LyraInventoryItemDefinition.h"
#include "LyraInventoryItemInstance.h"
#include "LyraInventoryManagerComponent.h"

void ULyraInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void ULyraInventorySubsystem::Deinitialize()
{
    // Cleanup managed items
    ManagedItems.Empty();
    RegisteredInventoryComponents.Empty();
    Super::Deinitialize();
}

const ULyraInventoryItemFragment* ULyraInventorySubsystem::FindItemDefinitionFragment(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const
{
    if (const ULyraInventoryItemDefinition* Definition = ItemDef.GetDefaultObject())
    {
        return Definition->FindFragmentByClass(FragmentClass);
    }
    return nullptr;
}

void ULyraInventorySubsystem::RegisterInventoryComponent(ULyraInventoryManagerComponent* Component)
{
    if (Component)
    {
        RegisteredInventoryComponents.AddUnique(Component);
    }
}

void ULyraInventorySubsystem::UnregisterInventoryComponent(ULyraInventoryManagerComponent* Component)
{
    if (Component)
    {
        RegisteredInventoryComponents.Remove(Component);
    }
}

ULyraInventoryItemInstance* ULyraInventorySubsystem::CreateInventoryItemInstance(UObject* Outer, TSubclassOf<ULyraInventoryItemDefinition> ItemDef)
{
    if (!ItemDef || !Outer)
    {
        return nullptr;
    }

    // Create the instance with the provided outer
    ULyraInventoryItemInstance* NewInstance = NewObject<ULyraInventoryItemInstance>(Outer);
    if (NewInstance)
    {
        InitializeItemInstance(NewInstance, ItemDef);
        ManagedItems.Add(NewInstance);
    }

    return NewInstance;
}

void ULyraInventorySubsystem::InitializeItemInstance(ULyraInventoryItemInstance* Instance, TSubclassOf<ULyraInventoryItemDefinition> ItemDef)
{
    if (!Instance || !ItemDef)
    {
        return;
    }

    Instance->SetItemDef(ItemDef);

    // Initialize with fragments
    if (const ULyraInventoryItemDefinition* Definition = ItemDef.GetDefaultObject())
    {
        for (const ULyraInventoryItemFragment* Fragment : Definition->Fragments)
        {
            if (Fragment)
            {
                Fragment->OnInstanceCreated(Instance);
            }
        }
    }
}

bool ULyraInventorySubsystem::IsValidItemInstance(const ULyraInventoryItemInstance* Instance) const
{
    return IsValid(Instance) && Instance->GetItemDef() != nullptr && ManagedItems.Contains(Instance);
}

TArray<ULyraInventoryItemInstance*> ULyraInventorySubsystem::GetValidItemsFromList(const TArray<FLyraInventoryEntry>& Entries) const
{
    TArray<TObjectPtr<ULyraInventoryItemInstance>> Results;
    Results.Reserve(Entries.Num());
    for (const FLyraInventoryEntry& Entry : Entries)
    {
        if (IsValidItemInstance(Entry.Instance))
        {
            Results.Add(Entry.Instance);
        }
    }
    return Results;
}

void ULyraInventorySubsystem::CleanupInvalidItems()
{
    ManagedItems.RemoveAll([](const ULyraInventoryItemInstance* Item) {
        return !IsValid(Item);
    });
}

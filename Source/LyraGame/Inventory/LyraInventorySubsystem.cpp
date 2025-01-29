// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventorySubsystem.h"
#include "LyraInventoryItemDefinition.h"
#include "LyraInventoryManagerComponent.h"

void ULyraInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void ULyraInventorySubsystem::Deinitialize()
{
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

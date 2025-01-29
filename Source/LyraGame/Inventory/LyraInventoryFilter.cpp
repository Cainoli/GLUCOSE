// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventoryFilter.h"
#include "LyraInventoryItemInstance.h"
#include "LyraInventoryItemDefinition.h"

bool ULyraInventoryFilter::PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const
{
    // Base class implementation always returns true
    // Derived classes should override this to implement specific filtering logic
    return true;
}

bool ULyraInventoryFilter_HasTag::PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const
{
    if (!ItemInstance || !RequiredTag.IsValid())
    {
        return false;
    }
    // Check if the item instance has the required tag as a stat tag
    return ItemInstance->HasStatTag(RequiredTag);
}

bool ULyraInventoryFilter_ItemDefinition::PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const
{
    if (!ItemInstance || !ItemDefinitionClass)
    {
        return false;
    }
    return ItemInstance->GetItemDef() == ItemDefinitionClass;
}

bool ULyraInventoryFilter_Composite::PassesFilter_Implementation(const ULyraInventoryItemInstance* ItemInstance) const
{
    if (!ItemInstance || Filters.Num() == 0)
    {
        return false;
    }

    for (const TObjectPtr<ULyraInventoryFilter>& Filter : Filters)
    {
        if (!Filter)
        {
            continue;
        }

        const bool bPassesFilter = Filter->PassesFilter(ItemInstance);
        if (bRequireAllFilters)
        {
            // AND operation - if any filter fails, return false
            if (!bPassesFilter)
            {
                return false;
            }
        }
        else
        {
            // OR operation - if any filter passes, return true
            if (bPassesFilter)
            {
                return true;
            }
        }
    }

    // If we require all filters and haven't returned false yet, all filters passed
    // If we require any filter and haven't returned true yet, no filter passed
    return bRequireAllFilters;
}

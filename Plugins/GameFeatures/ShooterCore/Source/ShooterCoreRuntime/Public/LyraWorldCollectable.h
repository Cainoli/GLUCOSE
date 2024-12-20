// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Interaction/Actors/LyraWorldInteractable.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/IPickupable.h"

#include "LyraWorldCollectable.generated.h"

class UObject;
struct FInteractionQuery;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class ALyraWorldCollectable : public ALyraWorldInteractable, public IPickupable
{
	GENERATED_BODY()

public:

	ALyraWorldCollectable();

	virtual FInventoryPickup GetPickupInventory() const override;

protected:

	UPROPERTY(EditAnywhere)
	FInventoryPickup StaticInventory;
};

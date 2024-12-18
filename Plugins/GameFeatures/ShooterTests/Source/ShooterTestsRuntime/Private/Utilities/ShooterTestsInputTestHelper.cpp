// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterTestsInputTestHelper.h"

void FShooterTestsPawnTestActions::ToggleCrouch()
{
	PerformAction(FToggleCrouchTestAction{});
}

void FShooterTestsPawnTestActions::PerformMelee()
{
	PerformAction(FMeleeTestAction{});
}

void FShooterTestsPawnTestActions::PerformJump()
{
	PerformAction(FJumpTestAction{});
}

void FShooterTestsPawnTestActions::MoveForward()
{
	PerformAxisAction(FMoveForwardTestAction{});
}

void FShooterTestsPawnTestActions::MoveBackward()
{
	PerformAxisAction(FMoveBackwardTestAction{});
}

void FShooterTestsPawnTestActions::StrafeLeft()
{
	PerformAxisAction(FStrafeLeftTestAction{});
}

void FShooterTestsPawnTestActions::StrafeRight()
{
	PerformAxisAction(FStrafeRightTestAction{});
}

void FShooterTestsPawnTestActions::RotateLeft()
{
	PerformAxisAction(FRotateLeftTestAction{});
}

void FShooterTestsPawnTestActions::RotateRight()
{
	PerformAxisAction(FRotateRightTestAction{});
}

void FShooterTestsPawnTestActions::PerformAxisAction(TFunction<void(const APawn* Pawn)> Action)
{
	// Perform move actions over the duration of 5 seconds
	PerformAction(Action, [this]() -> bool {
		if (StartTime.GetTicks() == 0)
		{
			StartTime = FDateTime::UtcNow();
		}

		FTimespan Elapsed = FDateTime::UtcNow() - StartTime;
		return Elapsed >= FTimespan::FromSeconds(5.0);
	});
}
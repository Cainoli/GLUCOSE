// Copyright Epic Games, Inc.All Rights Reserved.

#pragma once

#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraCharacter.h"
#include "Components/MapTestSpawner.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "Helpers/CQTestAssetHelper.h"
#include "ShooterTestsAnimationTestHelper.h"
#include "ShooterTestsInputTestHelper.h"

/**
 * Implementation of our base class used to share functionality of sharing a Pawn and a level for tests.
 * Inherits from `TTest<Derived, AsserterType>` to provide us our testing functionality.
 *
 * Implements functionality to load a level, specified as parameters in the Constructor, and verifies that the Player is completely spawned within the game world during the `Setup`
 *
 * Makes use of the `TestCommandBuilder` to queue up latent commands to be executed on every Tick of the Engine/Editor
 * `Then` steps will execute within a single tick
 * 'StartWhen` and `Until` steps will keep executing each tick until the predicate has evaluated to true or the timeout period has elapsed. The latter will fail the test.
 */
template<typename Derived, typename AsserterType>
struct ShooterTestsActorBaseTest : public TTest<Derived, AsserterType>
{
	/** Let this object know about our templated parent's variables */
	using TTest<Derived, AsserterType>::TestRunner;
	using TTest<Derived, AsserterType>::Assert;
	using TTest<Derived, AsserterType>::TestCommandBuilder;

	/**
	 * Construct the Base Actor Test.
	 *
	 * @param MapName - Name of the map.
	 */
	ShooterTestsActorBaseTest(const FString& MapName)
	{
		// Don't load assets during initialization
		if (TestRunner->bInitializing)
		{
			return;
		}
		
		TOptional<FString> PackagePath = CQTestAssetHelper::FindAssetPackagePathByName(MapName);
		ASSERT_THAT(IsTrue(PackagePath.IsSet(), "Could not find the map package."));
		Spawner = MakeUnique<FMapTestSpawner>(PackagePath.GetValue(), MapName);
	}

	/**
	 * Check to make sure that the specified world has fully loaded.
	 *
	 * @return true if the world is fully loaded.
	 * 
	 * @note Method is expected to be used within the `Until` latent command to then wait until the world has loaded.
	 */
	bool HasWorldLoaded()
	{
		UWorld& World = Spawner->GetWorld();
		AGameStateBase* GameState = World.GetGameState();
		if (GameState == nullptr)
		{
			return false;
		}

		ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
		if (ExperienceComponent == nullptr)
		{
			return false;
		}

		return ExperienceComponent->IsExperienceLoaded();
	}

	/** Get our Lyra Player Pawn and all associated systems and functionality needed for our Player. */
	virtual void PreparePlayerPawn()
	{
		Player = Cast<ALyraCharacter>(Spawner->FindFirstPlayerPawn());
		ASSERT_THAT(IsNotNull(Player));

		AbilitySystemComponent = Player->GetLyraAbilitySystemComponent();
		ASSERT_THAT(IsNotNull(AbilitySystemComponent));

		GameplayCueCharacterSpawnTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.Spawn"), false);
		ASSERT_THAT(IsTrue(GameplayCueCharacterSpawnTag.IsValid()));
	}

	/**
	 * Functionality used to check if the Player is fully spawned into the game world.
	 * 
	 * @return true if the GameplayEffect 'GE_SpawnIn' is no longer active as this effect blocks player input.
	 */
	virtual bool IsPlayerPawnFullySpawned()
	{
		bool bIsCurrentlySpawning = AbilitySystemComponent->IsGameplayCueActive(GameplayCueCharacterSpawnTag);
		return !bIsCurrentlySpawning;
	}

	/** Setup the test by loading in the specified level and making sure that the Lyra player is fully spawned in before continuing. */
	virtual void Setup() override
	{
		ASSERT_THAT(IsNotNull(Spawner));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);

		const FTimespan LoadingScreenTimeout = FTimespan::FromSeconds(30);
		TestCommandBuilder
			.StartWhen([this]() { return HasWorldLoaded(); }, LoadingScreenTimeout)
			.Until([this]() { return nullptr != Spawner->FindFirstPlayerPawn(); })
			.Then([this]() { PreparePlayerPawn(); })
			.Until([this]() { return IsPlayerPawnFullySpawned(); });
	}

	/** Reference to our player in the level. */
	ALyraCharacter* Player{ nullptr };

	/** Object to handle loading of our desired level. */
	TUniquePtr<FMapTestSpawner> Spawner{ nullptr };

	/** Reference to the player's ability system component. */
	ULyraAbilitySystemComponent* AbilitySystemComponent{ nullptr };

	/** Reference to the player's spawning gameplay effect. */
	FGameplayTag GameplayCueCharacterSpawnTag;
};

/**
 * Implementation of our base class used to share functionality of sharing a Pawn and a level for tests.
 * Inherits from `ShooterTestsActorBaseTest<Derived, AsserterType>` to provide us our testing functionality and to handle our initial World setup.
 *
 * Apart from loading our level and Player from the base `ShooterTestsActorBaseTest` object, will fetch the Player's SkeletalMesh component and sets up the FShooterTestsPawnTestActions object for input handling.
 * The SkeletalMesh component is what handles animations for the Player and is used to query against for active animations
 * FShooterTestsPawnTestActions is a user-defined input handling object which derives from CQTest's `FInputTestActions` in order to specify Input Actions around what is available to our Player.
 *
 * Makes use of the `TestCommandBuilder` to queue up latent commands to be executed on every Tick of the Engine/Editor
 * `Do` steps will execute within a single tick
 * `Until` steps will keep executing each tick until the predicate has evaluated to true or the timeout period has elapsed. The latter will fail the test.
 */
template<typename Derived, typename AsserterType>
struct ShooterTestsActorAnimationTest : public ShooterTestsActorBaseTest<Derived, AsserterType>
{
	/** Let this object know about our templated parent's variables */
	using ShooterTestsActorBaseTest<Derived, AsserterType>::Assert;
	using ShooterTestsActorBaseTest<Derived, AsserterType>::TestCommandBuilder;
	using ShooterTestsActorBaseTest<Derived, AsserterType>::Player;

	/**
	 * Construct the Actor Animation Test.
	 *
	 * @param MapName - Name of the map.
	 */
	ShooterTestsActorAnimationTest(const FString& MapName) : ShooterTestsActorBaseTest<Derived, AsserterType>(MapName) { }

	/**
	 * Calls the parent method to get our Lyra Player Pawn and all associated systems and functionality needed for our Player before setting up functionality needed for input handling and animations.
	 * 
	 * @see ShooterTestsActorBaseTest<Derived, AsserterType>::PreparePlayerPawn()
	 */
	void PreparePlayerPawn() override
	{
		ShooterTestsActorBaseTest<Derived, AsserterType>::PreparePlayerPawn();
		ASSERT_THAT(IsTrue(IsValid(Player), TEXT("Player Pawn has not been set")));

		PawnActions = MakeUnique<FShooterTestsPawnTestActions>(Player);

		UActorComponent* ActorComponent = Player->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		ASSERT_THAT(IsTrue(IsValid(ActorComponent), TEXT("Cannot find SkeletalMeshComponent from Player")));

		PlayerMesh = Cast<USkeletalMeshComponent>(ActorComponent);
		ASSERT_THAT(IsTrue(IsValid(PlayerMesh), TEXT("Cannot cast component to SkeletalMeshComponent")));
	}

	/**
	 * Get our expected animation to test against.
	 * @note Will assert if the animation cannot be found within the SkeletalMesh
	 */
	void GetExpectedAnimation(const FString& AnimationName)
	{
		ExpectedAnimation = AnimationTestHelper.FindAnimationAsset(PlayerMesh, AnimationName);
		ASSERT_THAT(IsTrue(IsValid(ExpectedAnimation), FString::Format(TEXT("Cannot find animation '{0}'"), { AnimationName })));
	}

	/**
	 * Tests to see if the expected animation is playing after performing our InputAction.
	 *
	 * @param AnimationName - Name of the animation asset to test against.
	 * @param InputAction - Function of the Input actions to run which will trigger the animation.
	 */
	void TestInputActionAnimation(const FString& AnimationName, TFunction<void()> InputAction)
	{
		GetExpectedAnimation(AnimationName);
		TestCommandBuilder
			.Do(InputAction)
			.Until([this]() { return AnimationTestHelper.IsAnimationPlaying(PlayerMesh, ExpectedAnimation); });
	}

	/** Animation helper object. */
	FShooterTestsAnimationTestHelper AnimationTestHelper;

	/** Reference to the player's skeletal mesh component. */
	USkeletalMeshComponent* PlayerMesh{ nullptr };

	/** Reference to our animation asset. */
	UAnimationAsset* ExpectedAnimation{ nullptr };

	/** Object which performs input actions. */
	TUniquePtr<FShooterTestsPawnTestActions> PawnActions{ nullptr };
};

/** Macro to quickly create tests based on the above test object. */
#define ACTOR_ANIMATION_TEST(_ClassName, _TestDir) TEST_CLASS_WITH_BASE(_ClassName, _TestDir, ShooterTestsActorAnimationTest)

/** Macro to quickly create tests based on the above test object with custom flags. */
#define ACTOR_ANIMATION_TEST_WITH_FLAGS(_ClassName, _TestDir, _Flags) TEST_CLASS_WITH_BASE_AND_FLAGS(_ClassName, _TestDir, ShooterTestsActorAnimationTest, _Flags)

#endif // WITH_AUTOMATION_TESTS
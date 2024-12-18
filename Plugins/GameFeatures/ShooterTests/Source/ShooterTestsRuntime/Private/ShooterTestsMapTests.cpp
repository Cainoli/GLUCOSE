// Copyright Epic Games, Inc.All Rights Reserved.

#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Character/LyraCharacter.h"
#include "Character/LyraHealthComponent.h"
#include "Components/MapTestSpawner.h"
#include "Helpers/CQTestAssetHelper.h"
#include "LyraGameplayTags.h"
#include "ObjectBuilder.h"
#include "System/LyraAssetManager.h"
#include "System/LyraGameData.h"

/**
 * Creates a standalone test object using the name from the first parameter, in the case `AbilitySpawnerMapTest`, which inherits from `TTest<Derived, AsserterType>` to provide us our testing functionality.
 * The second parameter specifies the category and subcategories used for displaying within the UI
 * The third parameter specifies the flags as to what context the test will run in and the filter to be applied for the test to appear in the UI
 * 
 * Note that this is a test which will be run in the Editor context as this test spawns objects based on Blueprints found on the local filesystem. Assets require to be cooked along the game to run in the game/client
 * 
 * The test object will test gameplay ability behaviors applied to the Player. All variables are reset after each test iteration.
 * 
 * The test makes use of the `TestCommandBuilder` to queue up latent commands to be executed on every Tick of the Engine/Editor
 * `Do` and `Then` steps will execute within a single tick
 * `StartWhen` and `Until` steps will keep executing each tick until the predicate has evaluated to true or the timeout period has elapsed. The latter will fail the test.
 * 
 * Each TEST_METHOD will register with the `AbilitySpawnerMapTest` test object and has the variables and methods from `AbilitySpawnerMapTest` available for use.
*/
TEST_CLASS_WITH_FLAGS(AbilitySpawnerMapTest, "Project.Functional Tests.ShooterTests.GameplayAbility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
{
	TUniquePtr<FMapTestSpawner> Spawner;

	ALyraCharacter* Player{ nullptr };
	AActor* GameplayEffectPad{ nullptr };
	ULyraAbilitySystemComponent* AbilitySystemComponent{ nullptr };
	const ULyraHealthSet* HealthSet{ nullptr };

	// Fetches the GameplayEffect which will trigger and apply the damage specified to the player
	void DoDamageToPlayer(double Damage)
	{
		const TSubclassOf<UGameplayEffect> DamageEffect = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DamageGameplayEffect_SetByCaller);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageEffect, 1.0, AbilitySystemComponent->MakeEffectContext());
		ASSERT_THAT(IsTrue(SpecHandle.IsValid()));
		SpecHandle.Data->SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, Damage);
		FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		ASSERT_THAT(IsTrue(Handle.WasSuccessfullyApplied()));
	}

	// Attempts to spawn a gameplay pad with a specified effect
	void SpawnGameplayPad(const FString& EffectName)
	{
		UClass* DesiredEffect = CQTestAssetHelper::GetBlueprintClass(EffectName);
		ASSERT_THAT(IsNotNull(DesiredEffect));

		UClass* GameplayEffectPadBp = CQTestAssetHelper::GetBlueprintClass(TEXT("BP_GameplayEffectPad"));
		ASSERT_THAT(IsNotNull(GameplayEffectPadBp));

		GameplayEffectPad = &TObjectBuilder<AActor>(*Spawner, GameplayEffectPadBp)
			.SetParam("GameplayEffectToApply", DesiredEffect)
			.Spawn(Player->GetTransform());

		ASSERT_THAT(IsNotNull(GameplayEffectPad));
	}

	// Checks to see if the player has been damaged
	bool IsPlayerDamaged()
	{
		return HealthSet->GetHealth() < HealthSet->GetMaxHealth();
	}

	/**
	 * Run before each TEST_METHOD to load our level, initialize our Player, and Player components needed for the tests before to execute successfully.
	 * If an ASSERT_THAT fails at any point, the TEST_METHODS will also fail as this means that our test prerequisites were not setup
	 */
	BEFORE_EACH()
	{
		const FString LevelName = TEXT("L_ShooterTest_Basic");

		TOptional<FString> PackagePath = CQTestAssetHelper::FindAssetPackagePathByName(LevelName);
		ASSERT_THAT(IsTrue(PackagePath.IsSet(), "Could not find the level package."));
		Spawner = MakeUnique<FMapTestSpawner>(PackagePath.GetValue(), LevelName);
		Spawner->AddWaitUntilLoadedCommand(TestRunner);

		const FTimespan LoadingScreenTimeout = FTimespan::FromSeconds(30);
		TestCommandBuilder
			.StartWhen([this]() { return nullptr != Spawner->FindFirstPlayerPawn(); }, LoadingScreenTimeout)
			.Do([this]() {
				Player = CastChecked<ALyraCharacter>(Spawner->FindFirstPlayerPawn());
				AbilitySystemComponent = Player->GetLyraAbilitySystemComponent();
				ASSERT_THAT(IsNotNull(AbilitySystemComponent));

				HealthSet = AbilitySystemComponent->GetSetChecked<ULyraHealthSet>();
				ASSERT_THAT(IsTrue(HealthSet->GetHealth() > 0));
				ASSERT_THAT(IsTrue(HealthSet->GetHealth() == HealthSet->GetMaxHealth()));
			});
	}

	// Tests to verify that the Player is able to get damaged
	TEST_METHOD(PlayerOnDamageSpawner_Eventually_LosesHealth)
	{
		TestCommandBuilder
			.StartWhen([this]() { return !AbilitySystemComponent->HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity); })
			.Then([this]() { SpawnGameplayPad(TEXT("GE_GameplayEffectPad_Damage")); })
			.Until([this]() { return IsPlayerDamaged(); });
	}

	// Tests to verify that the Player is able to get recover after being damaged
	TEST_METHOD(PlayerMissingHealth_OnHealSpawner_RestoresHealth)
	{
		TestCommandBuilder
			.StartWhen([this]() { return !AbilitySystemComponent->HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity); })
			.Then([this]() { DoDamageToPlayer(10.0); })
			.Until([this]() { return IsPlayerDamaged(); })
			.Then([this]() { SpawnGameplayPad(TEXT("GE_GameplayEffectPad_Heal")); })
			.Until([this]() { return !IsPlayerDamaged(); });
	}
};

#endif // WITH_AUTOMATION_TESTS
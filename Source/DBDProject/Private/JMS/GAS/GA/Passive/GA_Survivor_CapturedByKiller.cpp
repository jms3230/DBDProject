// Fill out your copyright notice in the Description page of Project Settings.
// IN -> PickUp -> Loop 순으로 애니메이션을 실행합니다.
// Active하지 않으면 End로 갑니다(탈출하거나 취소되었을 경우)
// TODO: Loop중에 스킬체크를 실행
#include "JMS/GAS/GA/Passive/GA_Survivor_CapturedByKiller.h"

#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "KMJ/Character/KillerCharacter.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/DBDMotionWarpingComponent.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"


UGA_Survivor_CapturedByKiller::UGA_Survivor_CapturedByKiller()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Passive_CapturedByKiller);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	TriggerData.TriggerTag = DBDGameplayTags::Survivor_Status_Captured_Killer;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Survivor_CapturedByKiller::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AKillerCharacter* KillerCharacter = GetKillerCharacterFromObserver();
	if (KillerCharacter)
	{
		
		UDBDBlueprintFunctionLibrary::MoveCharacterWithRotationAdjust(KillerCharacter->GetMesh(),
		                                                                FName(TEXT("joint_Char")),
		                                                                GetSurvivorCharacterFromActorInfo(),
		                                                                FRotator(0.f, -90.f, 0.f));
		AttachToKiller(FName(TEXT("joint_Char")));
	}
	SetIgnoreOtherCharacterCollision();
	if (K2_HasAuthority())
	{
		GetSurvivorCharacterFromActorInfo()->PlaySyncMontageFromServer(
			GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->PickedupIn);
	}
}


void UGA_Survivor_CapturedByKiller::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayAbilityActivationInfo ActivationInfo,
                                              bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	GetSurvivorCharacterFromActorInfo()->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld,
	                                                        EDetachmentRule::KeepWorld);
}

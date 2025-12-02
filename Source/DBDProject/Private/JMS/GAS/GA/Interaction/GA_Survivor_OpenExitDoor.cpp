// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/GA/Interaction/GA_Survivor_OpenExitDoor.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "MMJ/Object/Interactable/Obj_ExitDoor.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"


UGA_Survivor_OpenExitDoor::UGA_Survivor_OpenExitDoor()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Interaction_OpenExitDoor);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
}

void UGA_Survivor_OpenExitDoor::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Cost, Cooldown 등을 검사 + 점프 여부 검사
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	InteractStart();
	GetSurvivorCharacterFromActorInfo()->LookAtTargetActorFromServer(GetCachedCurrentInteractable<AActor>(),-30);
	EndOnInteractableTaskFinished();
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(false);

	if (K2_HasAuthority())
	{
		if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
		{
			Survivor->PlaySyncMontageFromServer(Survivor->SurvivorMontageData->OpenExitDoor,1.f);
		}
	}
}

void UGA_Survivor_OpenExitDoor::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	if (IsActive())
	{
		K2_EndAbility();
	}
}


void UGA_Survivor_OpenExitDoor::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (K2_HasAuthority())
	{
		if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
		{
			Survivor->StopSyncMontageFromServer(Survivor->SurvivorMontageData->OpenExitDoor);
		}
	}
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(true);
}

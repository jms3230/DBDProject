// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/GA/Interaction/GA_Survivor_RepairGenerator.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "Kismet/KismetMathLibrary.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/GAS/DBDAbilitySystemComponent.h"

UGA_Survivor_RepairGenerator::UGA_Survivor_RepairGenerator()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Interaction_RepairGenerator);
	ActivationRequiredTags.AddTag(DBDGameplayTags::Interactable_Object_Generator);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}


void UGA_Survivor_RepairGenerator::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	EndOnInteractableTaskFinished();
	LookAt(GetCachedCurrentInteractable<AActor>());

	if (IsLocallyControlled())
	{
		SetRandomSkillCheckEnabledOnClient();
	}
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(false);
	if (K2_HasAuthority())
	{
		if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
		{
			FGameplayTag DirectionTag = UDBDBlueprintFunctionLibrary::ComputeInteractDirection(
				GetCachedCurrentInteractable<AActor>(), Survivor);

			CurrentDirection = UDBDBlueprintFunctionLibrary::GetTagLastName(DirectionTag);
			Survivor->PlaySyncMontageFromServer(Survivor->SurvivorMontageData->RepairGenerator, 1.f, CurrentDirection);
		}
	}
}

void UGA_Survivor_RepairGenerator::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo* ActorInfo,
                                                const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	if (IsActive())
	{
		K2_EndAbility();
	}
}

void UGA_Survivor_RepairGenerator::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
	{
		Survivor->MoveEnabled(true);
		if (K2_HasAuthority())
		{
			Survivor->StopSyncMontageFromServer(Survivor->SurvivorMontageData->RepairGenerator);
		}
	}
	if (GetAnimInstance()->OnMontageEnded.IsAlreadyBound(this,&UGA_Survivor_RepairGenerator::OnFailedMontageEnded))
	{
		GetAnimInstance()->OnMontageEnded.RemoveDynamic(this, &UGA_Survivor_RepairGenerator::OnFailedMontageEnded);
	}
}

void UGA_Survivor_RepairGenerator::Server_SendSkillCheckResult_Implementation(ESkillCheckResult Result)
{
	FGameplayEventData EventData;
	EventData.EventTag = DBDGameplayTags::Object_Event_Result_Failure;
	EventData.Instigator = GetAvatarActorFromActorInfo();

	switch (Result)
	{
	case ESkillCheckResult::Bad:
		EventData.InstigatorTags = FGameplayTagContainer(DBDGameplayTags::Object_Event_Result_Failure);
		break;
	case ESkillCheckResult::Great:
		EventData.InstigatorTags = FGameplayTagContainer(DBDGameplayTags::Object_Event_Result_Success);
		break;
	default:
		break;
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetCurrentInteractable<AObj_Generator>(),
	                                                         DBDGameplayTags::Object_Event_Check,
	                                                         EventData);
}

void UGA_Survivor_RepairGenerator::OnSkillCheckBad()
{
	Super::OnSkillCheckBad();
	if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
	{
		Survivor->PlaySyncMontageFromOwnerClient(Survivor->SurvivorMontageData->RepairFailed,
		                                                               1.f, CurrentDirection);
		GetAnimInstance()->OnMontageEnded.AddDynamic(this, &UGA_Survivor_RepairGenerator::OnFailedMontageEnded);
	}
}

void UGA_Survivor_RepairGenerator::OnFailedMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->RepairFailed)
	{
		K2_EndAbility();
	}
}

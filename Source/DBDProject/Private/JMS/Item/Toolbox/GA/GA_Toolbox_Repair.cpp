// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/Toolbox/GA/GA_Toolbox_Repair.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/Item/Toolbox/Item_Toolbox.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"


UGA_Toolbox_Repair::UGA_Toolbox_Repair()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_UseItem_ToolboxRepair);
	ActivationRequiredTags.AddTag(DBDGameplayTags::Interactable_Object_Generator);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	TriggerData.TriggerTag = DBDGameplayTags::Item_Status_Using;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Toolbox_Repair::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	USurvivorAbilitySystemComponent* SurvivorASC = GetSurvivorAbilitySystemComponentFromActorInfo();
	if (!SurvivorASC)
	{
		return;
	}
	if (K2_HasAuthority())
	{
		FGameplayEffectContextHandle ContextHandle = SurvivorASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = SurvivorASC->MakeOutgoingSpec(
			BuffRepairSpeedEffect, 0, ContextHandle);

		SpecHandle.Data->SetSetByCallerMagnitude(DBDGameplayTags::Survivor_SetbyCaller_Multiplier1,
		                                         Cast<AItem_Toolbox>(GetEquippedItemFromActorInfo())->
		                                         GetRepairSpeedMultiplier());
		SurvivorASC->BP_ApplyGameplayEffectSpecToSelf(SpecHandle);
	}
	// Cost, Cooldown 등을 검사 + 점프 여부 검사
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	InteractStart();
	EndOnInteractableTaskFinished();
	LookAt(GetCachedCurrentInteractable<AActor>());

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
	if (IsLocallyControlled())
	{
		SetRandomSkillCheckEnabledOnClient();
	}
}

void UGA_Toolbox_Repair::EndAbility(const FGameplayAbilitySpecHandle Handle,
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
			GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffectBySourceEffect(BuffRepairSpeedEffect,
				GetAbilitySystemComponentFromActorInfo());

			Survivor->StopSyncMontageFromServer(Survivor->SurvivorMontageData->RepairGenerator);
		}
	}
	if (GetAnimInstance()->OnMontageEnded.IsAlreadyBound(this,&UGA_Toolbox_Repair::OnFailedMontageEnded))
	{
		GetAnimInstance()->OnMontageEnded.RemoveDynamic(this, &UGA_Toolbox_Repair::OnFailedMontageEnded);
	}
}


void UGA_Toolbox_Repair::Server_SendSkillCheckResult_Implementation(ESkillCheckResult Result)
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

void UGA_Toolbox_Repair::OnSkillCheckBad()
{
	Super::OnSkillCheckBad();
	if (ASurvivorCharacter* Survivor = GetSurvivorCharacterFromActorInfo())
	{
		Survivor->PlaySyncMontageFromOwnerClient(Survivor->SurvivorMontageData->RepairFailed,
		                                         1.f, CurrentDirection);
		GetAnimInstance()->OnMontageEnded.AddDynamic(this, &UGA_Toolbox_Repair::OnFailedMontageEnded);
	}
}

void UGA_Toolbox_Repair::OnFailedMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->RepairFailed)
	{
		K2_EndAbility();
	}
}

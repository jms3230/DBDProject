// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/Toolbox/GA/GA_Toolbox_Sabotage.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/ItemAbilitySystemComponent.h"
#include "JMS/GAS/ItemAttributeSet.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractorComponent.h"


UGA_Toolbox_Sabotage::UGA_Toolbox_Sabotage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_UseItem_ToolboxSabotage);
	ActivationRequiredTags.AddTag(DBDGameplayTags::Interactable_Object_Hook);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	TriggerData.TriggerTag = DBDGameplayTags::Item_Status_Using;
	AbilityTriggers.Add(TriggerData);
}

bool UGA_Toolbox_Sabotage::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	USurvivorAbilitySystemComponent* SurvivorASC = GetSurvivorAbilitySystemComponentFromActorInfo();
	if (SurvivorASC)
	{
		float CurrentCharge = SurvivorASC->GetNumericAttribute(USurvivorAttributeSet::GetCurrentItemChargeAttribute());
		if (CurrentCharge < 10.0f)
		{
			return false;
		}
	}
	UInteractorComponent* InteractorComponent = GetInteractorComponentFromActorInfo();
	if (InteractorComponent)
	{
		AObj_Hook* Hook = Cast<AObj_Hook>(InteractorComponent->GetCurrentInteractableActor());
		if (Hook)
		{
			if (UAbilitySystemComponent* HookASC = Hook->GetAbilitySystemComponent())
			{
				FGameplayTagContainer RestrictTags;
				RestrictTags.AddTag(DBDGameplayTags::Object_Status_IsDestroy);
				RestrictTags.AddTag(DBDGameplayTags::Object_Status_IsActive);

				if (!HookASC->HasAnyMatchingGameplayTags(RestrictTags))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UGA_Toolbox_Sabotage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	UDBDBlueprintFunctionLibrary::MoveDBDCharacterToMeshSocket(
		GetCachedCurrentInteractable<AObj_Hook>()->GetSkeletalMeshComponent(),
		FName(TEXT("socket_SurvivorAttach")),
		GetSurvivorCharacterFromActorInfo());
	LookAt(GetCachedCurrentInteractable<AActor>());
	float SabotageSpeed = 1.0f;
	USurvivorAbilitySystemComponent* SurvivorASC = GetSurvivorAbilitySystemComponentFromActorInfo();
	if (SurvivorASC)
	{
		SabotageSpeed = SurvivorASC->GetNumericAttribute(USurvivorAttributeSet::GetHookSabotageSpeedAttribute());
		if (SabotageSpeed <= 0)
		{
			SabotageSpeed = 1.0f;
		}
	}
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(false);
	if (K2_HasAuthority())
	{
		float AnimDuration = GetSurvivorCharacterFromActorInfo()->PlaySyncMontageFromServer(
			GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->HookSabotage, SabotageSpeed);
		GetWorld()->GetTimerManager().SetTimer(SabotageTimerHandle, this, &UGA_Toolbox_Sabotage::OnSabotageSucceed,
		                                       AnimDuration / SabotageSpeed, false);
		Client_StartWidgetUpdate(AnimDuration, SabotageSpeed);
	}
}

void UGA_Toolbox_Sabotage::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (K2_HasAuthority())
	{
		GetSurvivorCharacterFromActorInfo()->StopSyncMontageFromServer(
			GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->HookSabotage);
	}
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(true);
}

void UGA_Toolbox_Sabotage::OnSabotageSucceed()
{
	if (IsActive())
	{
		InteractStart();
		USurvivorAbilitySystemComponent* SurvivorASC = GetSurvivorAbilitySystemComponentFromActorInfo();
		if (SurvivorASC)
		{
			if (SabotageChargeConsumptionEffect)
			{
				SurvivorASC->BP_ApplyGameplayEffectToSelf(SabotageChargeConsumptionEffect, 0,
				                                          SurvivorASC->MakeEffectContext());
			}
		}
		K2_EndAbility();
	}
}

void UGA_Toolbox_Sabotage::UpdateProgressPercentageByDeltaProgress(float DeltaProgress)
{
	ProgressPercentage += DeltaProgress;
}

void UGA_Toolbox_Sabotage::Client_StartWidgetUpdate_Implementation(float AnimDuration, float SabotageSpeed)
{
	if (IsActive())
	{
		float DeltaProgress = SabotageSpeed / AnimDuration * WidgetUpdateInterval;
		SabotageWidgetTimerDelegate = FTimerDelegate::CreateUObject(
			this, &UGA_Toolbox_Sabotage::UpdateProgressPercentageByDeltaProgress,
			DeltaProgress);
		GetWorld()->GetTimerManager().SetTimer(SabotageWidgetTimerHandle, SabotageWidgetTimerDelegate,
		                                       WidgetUpdateInterval, true);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/GA/Interaction/GA_Survivor_HealOther.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "JMS/GAS/GE/GE_HealSkillCheckBad.h"
#include "JMS/GAS/GE/GE_HealSkillCheckGreat.h"
#include "JMS/GAS/GE/GE_HealSurvivor.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"


UGA_Survivor_HealOther::UGA_Survivor_HealOther()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Interaction_HealOther);
	ActivationRequiredTags.AddTag(DBDGameplayTags::Interactable_Character_Survivor);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Ability_Interaction_HealOther);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGA_Survivor_HealOther::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	if (K2_HasAuthority())
	{
		InteractStart();
		if (ASurvivorCharacter* CachedSurvivor = GetCachedCurrentInteractable<ASurvivorCharacter>())
		{
			CachedSurvivor->GetInteractableComponent()->OnComplete.AddDynamic(
				this, &UGA_Survivor_HealOther::K2_EndAbility);
			UDBDBlueprintFunctionLibrary::AddUniqueTagToActor(CachedSurvivor,
			                                                  DBDGameplayTags::Survivor_Status_BeingHealed);
		}
		else
		{
			K2_EndAbility();
		}
	}
	LookAt(GetCachedCurrentInteractable<AActor>());
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		GetAbilitySystemComponentFromActorInfo()->PlayMontage(this, ActivationInfo,
		                                                      GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->
		                                                      HealOther, 1.f);
		GetSurvivorCharacterFromActorInfo()->MoveEnabled(false);
	}
	if (IsLocallyControlled())
	{
		Debug::Print(TEXT("JMS: UGA_SurvivorHealOther::ActivateAbility"), -1);

		SetRandomSkillCheckEnabledOnClient();
		CachedOtherSurvivor = GetCachedCurrentInteractable<ASurvivorCharacter>();
		if (CachedOtherSurvivor)
		{
			GetWorld()->GetTimerManager().SetTimer(HealWidgetTimerHandle, this,
			                                       &UGA_Survivor_HealOther::UpdateProgressPercentage,
			                                       WidgetUpdateInterval, true);
		}
	}
}

void UGA_Survivor_HealOther::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	if (IsActive())
	{
		K2_EndAbility();
	}
}

void UGA_Survivor_HealOther::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                        bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (GetAnimInstance())
	{
		GetAnimInstance()->StopAllMontages(0.1f);
	}
	if (K2_HasAuthority() && GetCachedCurrentInteractable<AActor>())
	{
		if (ASurvivorCharacter* CachedSurvivor = GetCachedCurrentInteractable<ASurvivorCharacter>())
		{
			CachedSurvivor->GetInteractableComponent()->OnComplete.RemoveDynamic(
				this, &UGA_Survivor_HealOther::K2_EndAbility);
			UDBDBlueprintFunctionLibrary::RemoveTagFromActor(CachedSurvivor,
			                                                 DBDGameplayTags::Survivor_Status_BeingHealed);
		}
	}
	if (IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().ClearTimer(HealWidgetTimerHandle);
	}
	GetSurvivorCharacterFromActorInfo()->MoveEnabled(true);
}

void UGA_Survivor_HealOther::Server_SendSkillCheckResult_Implementation(ESkillCheckResult Result)
{
	FGameplayEventData EventData;
	EventData.EventTag = DBDGameplayTags::Object_Event_Result_Failure;
	EventData.Instigator = GetAvatarActorFromActorInfo();

	ASurvivorCharacter* ServerCachedOtherSurvivor = GetCachedCurrentInteractable<ASurvivorCharacter>();
	if (ServerCachedOtherSurvivor)
	{
		FGameplayEffectSpecHandle ResultEffectSpecHandle;
		switch (Result)
		{
		case ESkillCheckResult::Bad:
			EventData.InstigatorTags = FGameplayTagContainer(DBDGameplayTags::Object_Event_Result_Failure);
			ResultEffectSpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
				UGE_HealSkillCheckBad::StaticClass(), 0, GetAbilitySystemComponentFromActorInfo()->MakeEffectContext());
			GetSurvivorAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
				*ResultEffectSpecHandle.Data.Get(), ServerCachedOtherSurvivor->GetAbilitySystemComponent());
			break;

		case ESkillCheckResult::Great:
			EventData.InstigatorTags = FGameplayTagContainer(DBDGameplayTags::Object_Event_Result_Success);
			ResultEffectSpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
				UGE_HealSkillCheckGreat::StaticClass(), 0,
				GetAbilitySystemComponentFromActorInfo()->MakeEffectContext());
			GetSurvivorAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
				*ResultEffectSpecHandle.Data.Get(), ServerCachedOtherSurvivor->GetAbilitySystemComponent());
		default:
			break;
		}
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetCachedCurrentInteractable<ASurvivorCharacter>(),
		                                                         DBDGameplayTags::Object_Event_Check,
		                                                         EventData);
	}
}

void UGA_Survivor_HealOther::UpdateProgressPercentage()
{
	float CurrentHealth = 0.f, MaxHealth = 100.f;
	if (CachedOtherSurvivor)
	{
		CurrentHealth = CachedOtherSurvivor->GetAbilitySystemComponent()->GetNumericAttribute(
			USurvivorAttributeSet::GetHealProgressAttribute());
		MaxHealth = CachedOtherSurvivor->GetAbilitySystemComponent()->GetNumericAttribute(
			USurvivorAttributeSet::GetMaxHealProgressAttribute());
	}
	else
	{
		CachedOtherSurvivor = GetCachedCurrentInteractable<ASurvivorCharacter>();
	}
	if (MaxHealth != 0.f)
	{
		ProgressPercentage = CurrentHealth / MaxHealth;
	}
	else
	{
		ProgressPercentage = 0.f;
	}
}

void UGA_Survivor_HealOther::OnSkillCheckBad()
{
	Super::OnSkillCheckBad();
	if (IsActive())
	{
		K2_EndAbility();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.
// 생존자가 갈고리에 걸릴 때 애니메이션을 재생하고 근처의 갈고리에 붙습니다.
// TODO:Attach가 필요하면 나중에 추가

#include "JMS/GAS/GA/Passive/GA_Survivor_HookedIn.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/CapsuleComponent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/DBDMotionWarpingComponent.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"


UGA_Survivor_HookedIn::UGA_Survivor_HookedIn()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Passive_HookedIn);
	ActivationOwnedTags.AddTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	TriggerData.TriggerTag = DBDGameplayTags::Survivor_Status_Captured_Hook;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Survivor_HookedIn::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
                                            const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (K2_HasAuthority())
	{
		UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, DBDGameplayTags::Survivor_Event_RescueFromHook);
		WaitGameplayEventTask->EventReceived.AddDynamic(this, &UGA_Survivor_HookedIn::Rescued);
		WaitGameplayEventTask->ReadyForActivation();
	}
	if (IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimer(HookWidgetTimerHandle, this,
											   &UGA_Survivor_HookedIn::UpdateProgressPercentage,
											   WidgetUpdateInterval, true);
	}
}

void UGA_Survivor_HookedIn::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	GetSurvivorCharacterFromActorInfo()->SetCollisionAndGravityEnabled(true);
	GetSurvivorCharacterFromActorInfo()->GetMesh()->ClearMoveIgnoreActors();
	if (IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().ClearTimer(HookWidgetTimerHandle);
	}
}

void UGA_Survivor_HookedIn::Rescued(FGameplayEventData Data)
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}

void UGA_Survivor_HookedIn::UpdateProgressPercentage()
{
	float CurrentHealth = GetSurvivorAbilitySystemComponentFromActorInfo()->GetNumericAttribute(
		USurvivorAttributeSet::GetHookHPAttribute());
	float MaxHealth = GetSurvivorAbilitySystemComponentFromActorInfo()->GetNumericAttribute(
		USurvivorAttributeSet::GetMaxHookHPAttribute());


	if (MaxHealth != 0.f)
	{
		ProgressPercentage = CurrentHealth / MaxHealth;
	}
	else
	{
		ProgressPercentage = 0.f;
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/GA/SurvivorInteractionAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/GAS/GA/Interaction/GA_SelfCare.h"
#include "Kismet/KismetMathLibrary.h"
#include "MMJ/Object/Interactable/DBDObject.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/Interface/Interactor.h"


USurvivorInteractionAbility::USurvivorInteractionAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Captured_Killer);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Dying);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Dead);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Escaped);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Captured_Hook);
}

void USurvivorInteractionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo,
                                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	CachedInteractorComponent = GetInteractorComponentFromActorInfo();
	if (CachedInteractorComponent)
	{
		CachedCurrentInteractableActor = CachedInteractorComponent->GetCurrentInteractableActor();
	}
	if (K2_HasAuthority())
	{
		UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, DBDGameplayTags::Survivor_Event_GetHit);
		WaitGameplayEventTask->EventReceived.AddDynamic(this, &USurvivorInteractionAbility::OnGetHit);
		WaitGameplayEventTask->ReadyForActivation();
	}
}

void USurvivorInteractionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (CachedSkillCheckComponent)
	{
		if (CachedSkillCheckComponent->IsSkillCheckStillPlaying())
		{
			CachedSkillCheckComponent->OnEndSkillCheck();
		}
		CachedSkillCheckComponent->SkillCheckEndDelegate.RemoveAll(this);
	}
	GetWorld()->GetTimerManager().ClearTimer(SkillCheckTimerHandle);
	UE_LOG(LogTemp, Warning, TEXT("[%s] ClearTimer called"), *GetName());
	if (K2_HasAuthority())
	{
		if (bInteractStarted && CachedInteractorComponent)
		{
			CachedInteractorComponent->EndInteraction();
		}
		bInteractStarted = false;
		if (IInteractable* CurrentInteractable = Cast<IInteractable>(CachedCurrentInteractableActor))
		{
			if (CurrentInteractable->GetInteractableComponent()->OnComplete.IsAlreadyBound(
				this, &USurvivorInteractionAbility::K2_EndAbility))
				CurrentInteractable->GetInteractableComponent()->OnComplete.RemoveDynamic(
					this, &USurvivorInteractionAbility::K2_EndAbility);
		}
	}
}

UInteractorComponent* USurvivorInteractionAbility::GetInteractorComponentFromActorInfo() const
{
	IInteractor* Interactor = Cast<IInteractor>(GetAvatarActorFromActorInfo());
	if (Interactor)
	{
		return Interactor->GetInteractorComponent();
	}
	return nullptr;
}

void USurvivorInteractionAbility::InteractStart()
{
	if (CachedInteractorComponent&&K2_HasAuthority())
	{
		CachedInteractorComponent->InteractWithCurrentInteractable();
		bInteractStarted = true;
	}
}

void USurvivorInteractionAbility::EndOnInteractableTaskFinished() const
{
	if (!K2_HasAuthority())
	{
		return;
	}
	if (IInteractable* CurrentInteractable = Cast<IInteractable>(CachedCurrentInteractableActor))
	{
		CurrentInteractable->GetInteractableComponent()->OnComplete.AddDynamic(
			this, &USurvivorInteractionAbility::K2_EndAbility);
	}
}

void USurvivorInteractionAbility::SetRandomSkillCheckEnabledOnClient(float Frequency)
{
	CachedSkillCheckComponent = GetSkillCheckComponentFromActorInfo();
	if (CachedSkillCheckComponent && !CachedSkillCheckComponent->SkillCheckEndDelegate.IsAlreadyBound(
		this, &USurvivorInteractionAbility::OnSkillCheckEnd))
	{
		CachedSkillCheckComponent->SkillCheckEndDelegate.AddDynamic(
			this, &USurvivorInteractionAbility::OnSkillCheckEnd);
	}
	SetSkillCheckTimer(Frequency);
}

void USurvivorInteractionAbility::SetSkillCheckTimer(float Frequency)
{
	if (Frequency <= 0)
	{
		return;
	}
	// 임시로 10~15초 사이에 스킬 체크 발생
	GetWorld()->GetTimerManager().SetTimer(SkillCheckTimerHandle, this, &USurvivorInteractionAbility::PlaySkillCheck,
	                                       FMath::RandRange(5.f / Frequency, 10.f / Frequency), false);
	UE_LOG(LogTemp, Warning, TEXT("[%s] SetTimer called"), *GetName());
}

void USurvivorInteractionAbility::PlaySkillCheck()
{
	if (CachedSkillCheckComponent)
	{
		CachedSkillCheckComponent->TriggerOneShotSkillCheck(3.f, FMath::RandRange(0.7f, 1.7f), .7f, .3f);
	}
}

void USurvivorInteractionAbility::OnSkillCheckEnd(ESkillCheckResult Result)
{
	Server_SendSkillCheckResult(Result);
	if (Result == ESkillCheckResult::Bad)
	{
		OnSkillCheckBad();
	}
	SetSkillCheckTimer(1.f);
}

void USurvivorInteractionAbility::OnSkillCheckBad()
{
}

void USurvivorInteractionAbility::OnGetHit(FGameplayEventData Payload)
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}


void USurvivorInteractionAbility::Server_SendSkillCheckResult_Implementation(ESkillCheckResult Result)
{
	// 자식에서 구현
}

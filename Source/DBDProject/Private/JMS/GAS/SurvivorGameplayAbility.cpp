// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/SurvivorGameplayAbility.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KMJ/Character/KillerCharacter.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/UI/DBDWidgetComponent.h"


void USurvivorGameplayAbility::UpdateWidgetData()
{
	GetDBDWidgetComponentFromActorInfo()->OnUpdateAbilityProgress.Broadcast(
		bShowProgressWidgetOnActivated, AbilityDescription, ProgressPercentage);
}

void USurvivorGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (IsLocallyControlled())
	{
		if (bShowProgressWidgetOnActivated)
		{
			ProgressPercentage = 0.f;
			GetDBDWidgetComponentFromActorInfo()->OnUpdateAbilityProgress.Broadcast(
				true, AbilityDescription, ProgressPercentage);
			GetWorld()->GetTimerManager().SetTimer(WidgetUpdateTimerHandle, this,
			                                       &USurvivorGameplayAbility::UpdateWidgetData,
			                                       WidgetUpdateInterval, true);
		}
		if (bShowNameOnlyWidgetOnActivated)
		{
			GetDBDWidgetComponentFromActorInfo()->OnUpdateAbilityNameOnlyWidget.Broadcast(true, AbilityDescription);
		}
	}
	if (bHideItemMesh)
	{
		GetSurvivorCharacterFromActorInfo()->HideItemMesh(true);
	}
}

void USurvivorGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (bIsIgnoreCharacterSet)
	{
		GetSurvivorCharacterFromActorInfo()->GetCapsuleComponent()->ClearMoveIgnoreActors();
		GetSurvivorCharacterFromActorInfo()->SetCollisionAndGravityEnabled(true);
		GetSurvivorCharacterFromActorInfo()->MoveEnabled(true);
	}
	if (IsLocallyControlled())
	{
		if (bShowProgressWidgetOnActivated)
		{
			GetDBDWidgetComponentFromActorInfo()->OnUpdateAbilityProgress.Broadcast(
				false, AbilityDescription, ProgressPercentage);
			GetWorld()->GetTimerManager().ClearTimer(WidgetUpdateTimerHandle);
		}
		if (bShowNameOnlyWidgetOnActivated)
		{
			GetDBDWidgetComponentFromActorInfo()->OnUpdateAbilityNameOnlyWidget.Broadcast(false, AbilityDescription);
		}
	}
	if (bHideItemMesh)
	{
		GetSurvivorCharacterFromActorInfo()->HideItemMesh(false);
	}
}

UInteractableComponent* USurvivorGameplayAbility::GetInteractableComponentFromActorInfo() const
{
	IInteractable* Interactable = Cast<IInteractable>(GetAvatarActorFromActorInfo());
	if (Interactable)
	{
		return Interactable->GetInteractableComponent();
	}
	return nullptr;
}

USkillCheckComponent* USurvivorGameplayAbility::GetSkillCheckComponentFromActorInfo() const
{
	if (ASurvivorCharacter* SurvivorCharacter = Cast<ASurvivorCharacter>(GetAvatarActorFromActorInfo()))
	{
		return SurvivorCharacter->GetSkillCheckComponent();
	}
	return nullptr;
}

USurvivorAbilitySystemComponent* USurvivorGameplayAbility::GetSurvivorAbilitySystemComponentFromActorInfo() const
{
	return Cast<USurvivorAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent);
}

UAnimInstance* USurvivorGameplayAbility::GetAnimInstance() const
{
	USkeletalMeshComponent* OwningSkeletalMeshComponent = GetOwningComponentFromActorInfo();
	if (OwningSkeletalMeshComponent)
	{
		return OwningSkeletalMeshComponent->GetAnimInstance();
	}
	return nullptr;
}

ASurvivorCharacter* USurvivorGameplayAbility::GetSurvivorCharacterFromActorInfo() const
{
	return Cast<ASurvivorCharacter>(GetAvatarActorFromActorInfo());
}

AKillerCharacter* USurvivorGameplayAbility::GetKillerCharacterFromObserver() const
{
	if (UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		return CharacterSubsystem->GetKiller();
	}
	return nullptr;
}

ASurvivorItem* USurvivorGameplayAbility::GetEquippedItemFromActorInfo() const
{
	return GetSurvivorCharacterFromActorInfo()->GetEquippedItem();
}

void USurvivorGameplayAbility::SetIgnoreOtherCharacterCollision()
{
	if (UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		GetSurvivorCharacterFromActorInfo()->GetCapsuleComponent()->IgnoreActorWhenMoving(
			CharacterSubsystem->GetKiller(), true);
		for (ASurvivorCharacter* SurvivorCharacter : CharacterSubsystem->GetSurvivors())
		{
			GetSurvivorCharacterFromActorInfo()->GetCapsuleComponent()->IgnoreActorWhenMoving(SurvivorCharacter, true);
		}
		GetSurvivorCharacterFromActorInfo()->SetCollisionAndGravityEnabled(false);
	}
	bIsIgnoreCharacterSet = true;
}

void USurvivorGameplayAbility::LookAt(AActor* TargetActor) const
{
	if (TargetActor && K2_HasAuthority())
	{
		GetSurvivorCharacterFromActorInfo()->LookAtTargetActorFromServer(TargetActor);
	}
}

void USurvivorGameplayAbility::LookAtKiller()
{
	if (AKillerCharacter* KillerCharacter = GetKillerCharacterFromObserver())
	{
		GetSurvivorCharacterFromActorInfo()->LookAtTargetActorFromServer(KillerCharacter);
	}
}

void USurvivorGameplayAbility::AttachToKiller(FName SocketName)
{
	if (AKillerCharacter* KillerCharacter = GetKillerCharacterFromObserver())
	{
		UDBDBlueprintFunctionLibrary::AttachDBDCharacterToMeshSocket(KillerCharacter->GetMesh(), SocketName,
		                                                             GetSurvivorCharacterFromActorInfo());
	}
}

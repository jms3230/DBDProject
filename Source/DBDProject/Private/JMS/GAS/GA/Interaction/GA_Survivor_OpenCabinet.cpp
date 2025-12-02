// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/GAS/GA/Interaction/GA_Survivor_OpenCabinet.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MMJ/Object/Interactable/Obj_Cabinet.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractorComponent.h"

UGA_Survivor_OpenCabinet::UGA_Survivor_OpenCabinet()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(DBDGameplayTags::Survivor_Ability_Interaction_OpenCabinet);
	ActivationRequiredTags.AddTag(DBDGameplayTags::Interactable_Object_Cabinet);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Dying);
	ActivationBlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Captured_Killer);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGA_Survivor_OpenCabinet::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	if (IsActive())
	{
		Server_GetOut();
	}
}

void UGA_Survivor_OpenCabinet::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	GetSurvivorCharacterFromActorInfo()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (K2_HasAuthority())
	{
		CachedCabinet = Cast<AObj_Cabinet>(GetInteractorComponentFromActorInfo()->GetCurrentInteractableActor());

		FTransform ResultTransform = UDBDBlueprintFunctionLibrary::MoveCharacterWithRotationAdjust(CachedCabinet->GetSkeletalMeshComponent(),
		                                                                FName(TEXT("AttachKillerSocket")),
		                                                                GetSurvivorCharacterFromActorInfo(),
		                                                                FRotator(0, 90, 0));
		LookAt(GetCachedCurrentInteractable<AActor>());
		// 오브젝트 상태에 따른 애니메이션 실행
		if (CachedCabinet->GetAbilitySystemComponent())
		{
			if (!CachedCabinet->GetAbilitySystemComponent()->HasMatchingGameplayTag(
				DBDGameplayTags::Object_Status_IsActive))
			{
				GetSurvivorCharacterFromActorInfo()->PlaySyncMontageFromServer(
					GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->OpenCabinet, 1.f, FName("NAME_None"));
				GetSurvivorCharacterFromActorInfo()->GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(
					this, &UGA_Survivor_OpenCabinet::OnMontageEnded);
				InteractStart();
				GetSurvivorAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(
					DBDGameplayTags::Survivor_Status_Captured_Cabinet);
				GetSurvivorAbilitySystemComponentFromActorInfo()->AddReplicatedLooseGameplayTag(
					DBDGameplayTags::Survivor_Status_Captured_Cabinet);
			}
			else
			{
				GetSurvivorCharacterFromActorInfo()->PlaySyncMontageFromServer(
					GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->OpenCabinet, 1.f, FName("Full"));
				GetSurvivorCharacterFromActorInfo()->GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(
					this, &UGA_Survivor_OpenCabinet::OnMontageEnded);
				InteractStart();
			}
		}
	}
}

void UGA_Survivor_OpenCabinet::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (K2_HasAuthority())
	{
		GetSurvivorCharacterFromActorInfo()->GetMesh()->GetAnimInstance()->OnMontageEnded.RemoveDynamic(
			this, &UGA_Survivor_OpenCabinet::OnMontageEnded);
	}
}

void UGA_Survivor_OpenCabinet::Server_GetOut_Implementation()
{
	GetSurvivorCharacterFromActorInfo()->PlaySyncMontageFromServer(
		GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->OpenCabinet, 1.f, FName("GetOut"));
	GetSurvivorAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(
		DBDGameplayTags::Survivor_Status_Captured_Cabinet);
	GetSurvivorAbilitySystemComponentFromActorInfo()->RemoveReplicatedLooseGameplayTag(
		DBDGameplayTags::Survivor_Status_Captured_Cabinet);
	InteractStart();
}

void UGA_Survivor_OpenCabinet::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// if (Montage == GetSurvivorCharacterFromActorInfo()->SurvivorMontageData->OpenCabinet)
	// {
	// 	GetSurvivorCharacterFromActorInfo()->SyncLocationAndRotation();
	// }
	if (!GetSurvivorAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(
		DBDGameplayTags::Survivor_Status_Captured_Cabinet))
	{
		K2_EndAbility();
	}
	else
	{
		if (CachedCabinet)
		{
		}
	}
}

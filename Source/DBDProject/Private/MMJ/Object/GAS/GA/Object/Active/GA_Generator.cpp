// Fill out your copyright notice in the Description page of Project Settings.


#include "MMJ/Object/GAS/GA/Object/Active/GA_Generator.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Component/ICObject.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/GameFramework/DBDGameStateBase.h"
#include "Shared/Interface/Interactor.h"
#include "Shared/Subsystem/DBDObjectAuraSubsystem.h"

UGA_Generator::UGA_Generator()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	TriggerData.TriggerTag = DBDGameplayTags::Object_Status_IsActive;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Generator::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		DBDGameplayTags::Object_Event_Check
		);
	//Task->EventReceived.AddDynamic(ObjInteractableComponent, &UICObject::SkillCheckResultReceivedEvent);
	Task->EventReceived.AddDynamic(this, &ThisClass::ExecuteGameplayEffectAndCue);
	Task->ReadyForActivation();

	// 태스크의 실행/중지 판단을 위해 델리게이트 바인딩
	ObjInteractableComponent->OnInteract.AddDynamic(this, &ThisClass::SetTaskTimer);
	ObjInteractableComponent->OnDisconnect.AddDynamic(this, &ThisClass::UnSetTaskTimer);

	ObjInteractableComponent->OnInteractWithActor.AddDynamic(this, &ThisClass::PlayAnimationByActorDirection);
	ObjInteractableComponent->OnDisconnectWithActor.AddDynamic(this, &ThisClass::StopAnimationByActorDirection);
	
	SetTaskTimer();
}

void UGA_Generator::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

	// 몽타주 스탑
	ObjectStopMontage();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	// 델리게이트 해제
	ObjInteractableComponent->OnInteract.RemoveDynamic(this, &ThisClass::SetTaskTimer);
	ObjInteractableComponent->OnDisconnect.RemoveDynamic(this, &ThisClass::UnSetTaskTimer);
	
	ObjInteractableComponent->OnInteractWithActor.RemoveDynamic(this, &ThisClass::PlayAnimationByActorDirection);
	ObjInteractableComponent->OnDisconnectWithActor.RemoveDynamic(this, &ThisClass::StopAnimationByActorDirection);
}

void UGA_Generator::TaskFunction()
{
	Super::TaskFunction();

	int32 InteractorCount = ObjInteractableComponent->GetInteractors().Num();
	
	float TaskSpeed = ObjInteractableComponent->GetTaskAttributeOfInteractors();

	FGameplayEffectContextHandle SpecContext = OwningObjectASC->MakeEffectContext();
	SpecContext.AddSourceObject(OwningObject);
	SpecContext.AddInstigator(OwningObject, OwningObject);

	for (const FObjGEStruct& Effect : OwningObjectASC->GetObjDataAsset()->ActiveAdditionalEffects)
	{
		FGameplayEffectSpecHandle SpecHandle = OwningObjectASC->MakeOutgoingSpec(
			Effect.GameplayEffect, 1, SpecContext);
		SpecHandle.Data->SetSetByCallerMagnitude(DBDGameplayTags::Object_Progress_Speed, TaskSpeed);
		SpecHandle.Data->SetSetByCallerMagnitude(DBDGameplayTags::Object_Progress_ActorCount, InteractorCount);
		OwningObjectASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
}

void UGA_Generator::ExecuteGameplayEffectAndCue(FGameplayEventData Data)
{
	Super::ExecuteGameplayEffectAndCue(Data);

	if (Data.InstigatorTags.HasTag(DBDGameplayTags::Object_Event_Result_Failure))
	{
		for (AActor* Interactor : ObjInteractableComponent->GetInteractors())
		{
			if (IInteractor* IC = Cast<IInteractor>(Interactor))
			{
				if (UInteractorComponent* UIC = IC->GetInteractorComponent())
				{
					UIC->EndInteraction();
				}
			}
		}

		if (UDBDObjectAuraSubsystem* AuraSystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>())
		{
			if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
			{
				if (DBDGameState->Killer)
				{
					AuraSystem->SetAuraState(OwningObject, DBDGameState->Killer->GetPlayerState(), 5, 3.f);
				}
			}
		}
		
	}
}

void UGA_Generator::PlayAnimationByActorDirection(AActor* Actor)
{
	FGameplayTag DirectionTag = UDBDBlueprintFunctionLibrary::ComputeInteractDirection(OwningObject, Actor);
	if (InteractionAnimationsByDirection.Contains(DirectionTag))
	{
		ObjectPlayMontage(InteractionAnimationsByDirection[DirectionTag]);
	}
}

void UGA_Generator::StopAnimationByActorDirection(AActor* Actor)
{
	FGameplayTag DirectionTag = UDBDBlueprintFunctionLibrary::ComputeInteractDirection(OwningObject, Actor);
	if (InteractionAnimationsByDirection.Contains(DirectionTag))
	{
		ObjectStopMontage(InteractionAnimationsByDirection[DirectionTag]);
	}
}

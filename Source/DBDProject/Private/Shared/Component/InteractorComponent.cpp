// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Component/InteractorComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Component/IC_Hook.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "Net/UnrealNetwork.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Interface/Interactable.h"
#include "Shared/Interface/Interactor.h"
#include "MotionWarping/Public/RootMotionModifier.h"
#include "Shared/Component/DBDMotionWarpingComponent.h"


// Sets default values for this component's properties
UInteractorComponent::UInteractorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SphereRadius = 150.f;
	SetIsReplicatedByDefault(true);
	bIsSearchingEnabled = true;
	SearchInterval = 0.1f;
	SearchRadius = 150.f;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	SetCollisionProfileName(FName("NoCollision"));
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(
			SearchTimerHandle,
			this,
			&UInteractorComponent::CheckNearbyInteractable,
			SearchInterval,
			true);
	}
}

void UInteractorComponent::CheckNearbyInteractable()
{
	if (!bIsSearchingEnabled)
	{
		return;
	}
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	TArray<FHitResult> HitResults;
	ETraceTypeQuery TraceTypeQuery = UCollisionProfile::Get()->ConvertToTraceType(
		ECollisionChannel::ECC_GameTraceChannel2);
	UKismetSystemLibrary::SphereTraceMulti(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation(),
	                                       SearchRadius,
	                                       TraceTypeQuery, false, ActorsToIgnore, EDrawDebugTrace::Type::None,
	                                       HitResults, true, FLinearColor::Red, FLinearColor::Green, 0.1);

	AActor* ClosestActor = nullptr;
	float ClosestDistance = MAX_FLT;
	if (HitResults.Num() == 0)
	{
		RegisterOverlappedInteractable(nullptr);
		return;
	}
	IInteractable* Candidate = nullptr;
	for (const FHitResult& HitResult : HitResults)
	{
		IInteractable* Interactable = Cast<IInteractable>(HitResult.GetActor());
		if (Interactable && Interactable->GetInteractableComponent()->CanInteraction(GetOwner()))
		{
			FVector HitActorLocation = HitResult.GetActor()->GetActorLocation();
			FVector OwnerLocation = GetOwner()->GetActorLocation();
			float DistanceToActor = (HitActorLocation - OwnerLocation).Size();
			if (DistanceToActor < ClosestDistance)
			{
				Candidate = Interactable;
				ClosestDistance = DistanceToActor;
			}
		}
	}
	RegisterOverlappedInteractable(Candidate);
}

void UInteractorComponent::RegisterOverlappedInteractable(IInteractable* Interactable)
{
	// JMS: RPC로 전달
	if (CurrentInteractable != Interactable)
	{
		Client_CurrentInteractableChanged(Cast<AActor>(Interactable));
	}

	CurrentInteractable = Interactable;
	// JMS: 서버에서만 실행되므로 단순화했습니다.
	AActor* NewInteractableActor = CurrentInteractable
		                               ? CurrentInteractable->GetInteractableComponent()->GetOwner()
		                               : nullptr;
	CurrentInteractableActor = NewInteractableActor;


	if (CurrentInteractable)
	{
		if (ADBDCharacter* OwnerCharacter = Cast<ADBDCharacter>(GetOwner()))
		{
			if (UObject* CurrentInteractableObject = Cast<UObject>(CurrentInteractable))
			{
				UDBDBlueprintFunctionLibrary::AddOrUpdateMotionWarpingTarget(
					IInteractable::Execute_GetMotionWarpingTargets(CurrentInteractableObject),
					OwnerCharacter->GetDBDMotionWarpingComponent());
			}
		}
	}

	UpdateTag();
}


void UInteractorComponent::UnRegisterOverlappedInteractable(IInteractable* Interactable)
{
	if (CurrentInteractable == Interactable)
	{
		CurrentInteractable = nullptr;
		Client_CurrentInteractableChanged(Cast<AActor>(Interactable));
		UpdateTag();
	}
}


void UInteractorComponent::InteractWithCurrentInteractable()
{
	if (CurrentInteractable)
	{
		if (CurrentInteractable->GetInteractableComponent()->CanInteraction(GetOwner()))
		{
			// Getter를 호출해서 컴포넌트를 가져옴
			// MMJ (BeginInteraction에서 StartInteraction으로 간소화)
			CurrentInteractable->GetInteractableComponent()->StartInteraction(GetOwner());
		}
	}
	SearchingEnabled(false);
}

void UInteractorComponent::EndInteraction()
{
	//현재 Interactable의 Interaction마무리 delegate broadcast
	if (CurrentInteractable)
	{
		//  MMJ (EndInteraction에서 FinishInteraction으로 간소화)
		CurrentInteractable->GetInteractableComponent()->FinishInteraction(GetOwner());
	}
	SearchingEnabled(true);
}

void UInteractorComponent::SearchingEnabled(bool bEnabled)
{
	bIsSearchingEnabled = bEnabled;
}

IInteractor* UInteractorComponent::GetOwnerAsInteractor()
{
	return Cast<IInteractor>(GetOwner());
}

UAbilitySystemComponent* UInteractorComponent::GetOwnerAbilitySystemComponent()
{
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (!ASI)
	{
		return nullptr;
	}
	return ASI->GetAbilitySystemComponent();
}

void UInteractorComponent::Client_CurrentInteractableChanged_Implementation(AActor* NewInteractable)
{
	CurrentInteractableActor = NewInteractable;
}

void UInteractorComponent::UpdateTag()
{
	UDBDBlueprintFunctionLibrary::RemoveAllSubTags(FName("Interactable"), GetOwnerAbilitySystemComponent());
	if (CurrentInteractable)
	{
		GetOwnerAbilitySystemComponent()->AddLooseGameplayTag(
			CurrentInteractable->GetInteractableComponent()->InteractableTag);
		GetOwnerAbilitySystemComponent()->AddReplicatedLooseGameplayTag(
			CurrentInteractable->GetInteractableComponent()->InteractableTag);
	}
}

//
// void UInteractorComponent::OnRep_CurrentInteractableActor()
// {
// }

void UInteractorComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME_CONDITION_NOTIFY(UInteractorComponent, CurrentInteractableActor, COND_None, REPNOTIFY_Always);
}


AActor* UInteractorComponent::GetCurrentInteractableActor()
{
	return CurrentInteractableActor;
}

#if WITH_EDITOR
void UInteractorComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		                     ? PropertyChangedEvent.Property->GetFName()
		                     : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInteractorComponent, SearchRadius))
	{
		SetSphereRadius(SearchRadius);
	}
}
#endif

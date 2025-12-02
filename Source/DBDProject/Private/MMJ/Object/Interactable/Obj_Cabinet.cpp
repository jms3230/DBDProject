// Fill out your copyright notice in the Description page of Project Settings.


#include "MMJ/Object/Interactable/Obj_Cabinet.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "KMJ/Axe/AxeComponent.h"
#include "KMJ/Axe/ProjectileAxe.h"
#include "KMJ/Character/KillerHuntress.h"
#include "MMJ/Object/Component/IC_Cabinet.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"

AObj_Cabinet::AObj_Cabinet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIC_Cabinet>(InteractableComponentName))
{
	
}

void AObj_Cabinet::CreateCombinedActor()
{
	if (UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		if (AKillerHuntress* Killer = Cast<AKillerHuntress>(CharacterSubsystem->GetKiller()))
		{
			if (IsValid(Killer->AxeComponent) && IsValid(Killer->AxeComponent->Projectile))
			{
				CombinedActorClass = Killer->AxeComponent->Projectile;
				
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Owner = this;
				
				if (AActor* ThrowObject = GetWorld()->SpawnActor<AActor>(CombinedActorClass, SpawnParameters))
				{
					ThrowObject->SetReplicates(true);
					ThrowObject->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform, "ProjectileAttachSocket");
					if (UProjectileMovementComponent* ProjectileMovementComponent = ThrowObject->FindComponentByClass<UProjectileMovementComponent>())
					{
						ProjectileMovementComponent->InitialSpeed = 0.f;
						ProjectileMovementComponent->Velocity = FVector::ZeroVector;
						ProjectileMovementComponent->SetActive(false);
						ThrowObject->SetActorTickEnabled(false);
					}
					CombinedActor = ThrowObject;
					if (AActor* ShelveActor = GetWorld()->SpawnActor<AActor>(Shelf, SpawnParameters))
					{
						ShelveActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform, "ShelfAttachSocket");
					}
				}
			}
		}
	}
}

AActor* AObj_Cabinet::GetCombinedActor() const
{
	return CombinedActor;
}

void AObj_Cabinet::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle Timer;
	GetWorld()->GetTimerManager().SetTimer(
		Timer,
		FTimerDelegate::CreateLambda([&]()
		{
			CreateCombinedActor();
		}),
		5.f,
		false);
}
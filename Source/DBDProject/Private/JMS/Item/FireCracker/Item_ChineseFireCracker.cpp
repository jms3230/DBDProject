// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/FireCracker/Item_ChineseFireCracker.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/GAS/ItemAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Character/DBDCharacter.h"
#include "Shared/GAS/GE/GE_GrantBlindTagForDuration.h"

AItem_ChineseFireCracker::AItem_ChineseFireCracker()
{
	ItemTag = DBDGameplayTags::Survivor_Item_FireCracker;
}

void AItem_ChineseFireCracker::OnStartUsingItem()
{
	Super::OnStartUsingItem();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(ExplodeTimerHandle, this, &AItem_ChineseFireCracker::Explode,
		                                       ExplodeDelay,
		                                       false);
		GetOwnerSurvivor()->DropItem(nullptr);
	}
}

void AItem_ChineseFireCracker::Explode()
{
	TArray<AActor*> ActorsToIgnore;
	TArray<FHitResult> HitResults;
	ETraceTypeQuery TraceTypeQuery = UCollisionProfile::Get()->ConvertToTraceType(ECollisionChannel::ECC_Pawn);
	UKismetSystemLibrary::SphereTraceMulti(GetWorld(), GetActorLocation(), GetActorLocation(),
	                                       ExplodeRadius,
	                                       TraceTypeQuery, false, ActorsToIgnore, EDrawDebugTrace::Type::ForDuration,
	                                       HitResults, true, FLinearColor::Red, FLinearColor::Green, 0.1);


	for (const FHitResult& HitResult : HitResults)
	{
		IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(HitResult.GetActor());
		if (!ASInterface)
		{
			continue;
		}

		UDBDAbilitySystemComponent* ASC = Cast<UDBDAbilitySystemComponent>(
			ASInterface->GetAbilitySystemComponent());
		if (ASC)
		{
			ASC->BP_ApplyGameplayEffectToSelf(UGE_GrantBlindTagForDuration::StaticClass(), 0, ASC->MakeEffectContext());
		}
	}
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

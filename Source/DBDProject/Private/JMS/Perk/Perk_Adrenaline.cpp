// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Perk/Perk_Adrenaline.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "Shared/GameFramework/DBDGameStateBase.h"
#include "Shared/Subsystem/DBDEndGameSubsystem.h"
#include "Shared/Subsystem/DBDObjectObserver.h"


UPerk_Adrenaline::UPerk_Adrenaline()
{
	PerkID = FName(TEXT("Adrenaline"));
}

void UPerk_Adrenaline::OnServerSideInitialized()
{
	Super::OnServerSideInitialized();
	ADBDGameStateBase* DBDGameStateBase = Cast<ADBDGameStateBase>(GetWorld()->GetGameState());
	if (DBDGameStateBase)
	{
		UDBDObjectObserver* ObjectObserver = GetWorld()->GetSubsystem<UDBDObjectObserver>();
		if (ObjectObserver)
		{
			ObjectObserver->OnExitDoorActivated.AddDynamic(this, &UPerk_Adrenaline::OnExitDoorEnabled);
		}
	}
}

void UPerk_Adrenaline::OnOwnerClientSideInitialized()
{
	Super::OnOwnerClientSideInitialized();
}

void UPerk_Adrenaline::OnExitDoorEnabled()
{
	if (AdrenalineEffect)
	{
		GetOuterAsDBDCharacter()->GetAbilitySystemComponent()->BP_ApplyGameplayEffectToSelf(
			AdrenalineEffect, 0, GetOuterAsDBDCharacter()->GetAbilitySystemComponent()->MakeEffectContext());
		if (ExhaustEffect)
		{
			GetOuterAsDBDCharacter()->GetAbilitySystemComponent()->BP_ApplyGameplayEffectToSelf(
				ExhaustEffect, 0, GetOuterAsDBDCharacter()->GetAbilitySystemComponent()->MakeEffectContext());
		}
	}
	if (ASurvivorCharacter* OwnerSurvivor = Cast<ASurvivorCharacter>(GetOuterAsDBDCharacter()))
	{
		if (OwnerSurvivor->GetAbilitySystemComponent()->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Injured))
		{
			OwnerSurvivor->SetSurvivorNormal();
		}
		else if (OwnerSurvivor->GetAbilitySystemComponent()->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Dying))
		{
			OwnerSurvivor->SetSurvivorInjured();
		}
	}
}

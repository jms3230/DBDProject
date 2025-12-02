// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Perk/Perk_Empathy.h"

#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "UObject/FastReferenceCollector.h"


UPerk_Empathy::UPerk_Empathy()
{
	PerkID = FName(TEXT("Empathy"));
}

void UPerk_Empathy::OnServerSideInitialized()
{
	Super::OnServerSideInitialized();
}

void UPerk_Empathy::OnOwnerClientSideInitialized()
{
	Super::OnOwnerClientSideInitialized();
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		Debug::Print(TEXT("JMS: CharacterSubsystem is null"), -1);
		return;
	}
	FGameplayTagContainer RequiredTags;
	RequiredTags.AddTag(DBDGameplayTags::Survivor_Status_Injured);
	RequiredTags.AddTag(DBDGameplayTags::Survivor_Status_Dying);
	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Captured_Killer);
	BlockedTags.AddTag(DBDGameplayTags::Survivor_Status_Captured_Hook);

	CharacterSubsystem->EnableSurvivorAuraWithDistanceAndTag(this, GetOuterAsDBDCharacter(), 2000, RequiredTags,
	                                                         BlockedTags);
}

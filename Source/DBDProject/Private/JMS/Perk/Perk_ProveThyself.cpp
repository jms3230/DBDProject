// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Perk/Perk_ProveThyself.h"

#include "AbilitySystemComponent.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Character/DBDCharacter.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"


UPerk_ProveThyself::UPerk_ProveThyself()
{
	PerkID = FName(TEXT("ProveThyself"));
}

void UPerk_ProveThyself::OnServerSideInitialized()
{
	Super::OnServerSideInitialized();
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		Debug::Print(TEXT("JMS: CharacterSubsystem is null"), -1);
		return;
	}
	CharacterSubsystem->ApplyGEWithSurvivorWithinDistance(GetOuterAsDBDCharacter(), 400,
	                                                     ProveThyselfEffect);
}

void UPerk_ProveThyself::OnOwnerClientSideInitialized()
{
	Super::OnOwnerClientSideInitialized();
}

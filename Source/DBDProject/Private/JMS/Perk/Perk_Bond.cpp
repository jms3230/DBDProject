// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Perk/Perk_Bond.h"

#include "Shared/DBDDebugHelper.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"


UPerk_Bond::UPerk_Bond()
{
	PerkID = FName(TEXT("Bond"));
}

void UPerk_Bond::OnServerSideInitialized()
{
	Super::OnServerSideInitialized();
}

void UPerk_Bond::OnOwnerClientSideInitialized()
{
	Super::OnOwnerClientSideInitialized();

	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11111:CharacterSubsystem is null"), 11111);
		return;
	}

	CharacterSubsystem->EnableSurvivorAuraWithDistanceAndTag(this, GetOuterAsDBDCharacter(), 1000);
}

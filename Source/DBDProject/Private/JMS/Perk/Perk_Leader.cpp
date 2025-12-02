// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Perk/Perk_Leader.h"

#include "Shared/DBDDebugHelper.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"


UPerk_Leader::UPerk_Leader()
{
	PerkID = FName(TEXT("Leader"));
}

void UPerk_Leader::OnServerSideInitialized()
{
	Super::OnServerSideInitialized();
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11111:CharacterSubsystem is null"), 11111);
		return;
	}

	CharacterSubsystem->ApplyGEToSurvivorsWithinDistance(GetOuterAsDBDCharacter(), 800,
														 LeaderEffect);
}

void UPerk_Leader::OnOwnerClientSideInitialized()
{
	Super::OnOwnerClientSideInitialized();
}

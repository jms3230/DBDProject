// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Perk/PerkComponent.h"
#include "Perk_SprintBurst.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API UPerk_SprintBurst : public UPerkComponent
{
	GENERATED_BODY()
public:
	UPerk_SprintBurst();
	virtual void OnServerSideInitialized() override;
	virtual void OnOwnerClientSideInitialized() override;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> SprintBurstEffect;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> ExhaustEffect;
	UFUNCTION()
	void OnSprintStarted();
	
	
	
};

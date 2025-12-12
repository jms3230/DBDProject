// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Perk/PerkComponent.h"
#include "Perk_QuickAndQuiet.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API UPerk_QuickAndQuiet : public UPerkComponent
{
	GENERATED_BODY()
public:
	UPerk_QuickAndQuiet();
	virtual void OnServerSideInitialized() override;
	virtual void OnOwnerClientSideInitialized() override;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> QuickAndQuietEffect;
	UFUNCTION()
	void OnQuickActionPlayed();
	void RestoreEffectAfterCooldown();
	
	
};

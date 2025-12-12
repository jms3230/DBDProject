// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Shared/Perk/PerkComponent.h"
#include "Perk_ProveThyself.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API UPerk_ProveThyself : public UPerkComponent
{
	GENERATED_BODY()
public:
	UPerk_ProveThyself();
	virtual void OnServerSideInitialized() override;
	virtual void OnOwnerClientSideInitialized() override;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> ProveThyselfEffect;
	UPROPERTY()
	FGameplayEffectSpecHandle ProveThyselfEffectSpecHandle;
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Perk/PerkComponent.h"
#include "Perk_Adrenaline.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API UPerk_Adrenaline : public UPerkComponent
{
	GENERATED_BODY()
public:
	UPerk_Adrenaline();
	virtual void OnServerSideInitialized() override;
	virtual void OnOwnerClientSideInitialized() override;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> AdrenalineEffect;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> ExhaustEffect;
	UFUNCTION()
	void OnExitDoorEnabled();
};

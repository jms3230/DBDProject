// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JMS/Item/SurvivorItem.h"
#include "Item_ChineseFireCracker.generated.h"

/**
 * 
 */
UCLASS()
class DBDPROJECT_API AItem_ChineseFireCracker : public ASurvivorItem
{
	GENERATED_BODY()

public:
	AItem_ChineseFireCracker();
public:
	virtual void OnStartUsingItem() override;
	UPROPERTY(EditDefaultsOnly, Category = "FireCracker")
	float ExplodeDelay = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "FireCracker")
	float ExplodeRadius = 200.f;
protected:
	UFUNCTION()
	void Explode();
	FTimerHandle ExplodeTimerHandle;
};

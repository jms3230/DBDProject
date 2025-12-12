// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JMS/Item/SurvivorItem.h"
#include "Item_Toolbox.generated.h"

class UGameplayAbility;
class UToolboxAttributeSet;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API AItem_Toolbox : public ASurvivorItem
{
	GENERATED_BODY()
public:
	AItem_Toolbox();
	FORCEINLINE float GetRepairSpeedMultiplier() const
	{
		return RepairSpeedMultiplier;
	}

	FORCEINLINE float GetSabotageSpeedMultiplier() const
	{
		return SabotageSpeedMultiplier;
	}

	FORCEINLINE void AddRepairSpeedMultiplier(float Amount)
	{
		RepairSpeedMultiplier += Amount;
	}

	FORCEINLINE void AddSabotageSpeedMultiplier(float Amount)
	{
		SabotageSpeedMultiplier += Amount;
	}
private:
	UPROPERTY(EditDefaultsOnly)
	float RepairSpeedMultiplier = 1.5f;
	UPROPERTY(EditDefaultsOnly)
	float SabotageSpeedMultiplier = 1.15f;
};

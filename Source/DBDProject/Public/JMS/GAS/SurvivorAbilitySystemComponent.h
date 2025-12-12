#pragma once

#include "CoreMinimal.h"
#include "Shared/GAS/DBDAbilitySystemComponent.h"
#include "SurvivorAbilitySystemComponent.generated.h"


class UDA_SurvivorCommonStats;
class UDA_SurvivorASCData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DBDPROJECT_API USurvivorAbilitySystemComponent : public UDBDAbilitySystemComponent
{
	GENERATED_BODY()

private:
	
	virtual void GrantInputAbilities() override;
	
	virtual void InitializeBaseAttributes() override;

	UPROPERTY(EditDefaultsOnly, Category = "DataControl")
	UDA_SurvivorASCData* SurvivorASCData;
};

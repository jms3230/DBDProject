// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Shared/ObjectPool/DBDPoolEntryObject.h"
#include "PoolEntry_ScratchMark.generated.h"

class ADecalActor;
class UBillboardComponent;
/**
 * 
 */
LLM_DECLARE_TAG(APoolEntry_ScratchMark);
UCLASS()
class DBDPROJECT_API APoolEntry_ScratchMark : public ADBDPoolEntryObject
{
	GENERATED_BODY()

public:
	APoolEntry_ScratchMark();
	virtual void SetActive(bool IsActive, APawn* InInstigator) override;
	virtual void Deactivate() override;

protected:
	UPROPERTY(EditAnywhere, Category = "Scratch Mark")
	UBillboardComponent* RootIndicator;
	UPROPERTY(EditAnywhere, Category = "Scratch Mark")
	UDecalComponent* ScratchMarkDecalComponentWall;
	UPROPERTY(EditAnywhere, Category = "Scratch Mark")
	UDecalComponent* ScratchMarkDecalComponentFloor;
};

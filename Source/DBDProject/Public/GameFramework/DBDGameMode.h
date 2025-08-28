// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DBDGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DBDPROJECT_API ADBDGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADBDGameMode();
	virtual void BeginPlay() override;

public:
	void TEST();
};

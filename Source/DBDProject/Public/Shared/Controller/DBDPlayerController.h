// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DBDPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class DBDPROJECT_API ADBDPlayerController : public APlayerController
{
	GENERATED_BODY()
	virtual void BeginPlay() override;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Controller/DBDPlayerController.h"

void ADBDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("DBDPlayerController::BeginPlay"));
}

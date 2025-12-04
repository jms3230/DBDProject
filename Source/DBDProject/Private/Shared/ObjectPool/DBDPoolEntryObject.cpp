// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/ObjectPool/DBDPoolEntryObject.h"


// Sets default values
ADBDPoolEntryObject::ADBDPoolEntryObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 0.f;
}

void ADBDPoolEntryObject::Deactivate()
{
	SetActive(false, nullptr);
	GetWorldTimerManager().ClearAllTimersForObject(this);
	OnPooledObjectDespawn.Broadcast(this);
	SetActorHiddenInGame(true);
}


void ADBDPoolEntryObject::SetActive(bool IsActive, APawn* InInstigator)
{
	if (IsActive)
	{
		SetInstigator(InInstigator);
		GetWorldTimerManager().SetTimer(LifeTimeTimerHandle, this, &ADBDPoolEntryObject::Deactivate, LifeTime, false);
	}
	bIsActive = IsActive;
	SetActorHiddenInGame(!IsActive);
}

void ADBDPoolEntryObject::SetLifeTime(float InLifeTime)
{
	LifeTime = InLifeTime;
}

void ADBDPoolEntryObject::SetPoolIndex(int InIndex)
{
	PoolIndex = InIndex;
}

bool ADBDPoolEntryObject::IsActive()
{
	return bIsActive;
}

int ADBDPoolEntryObject::GetPoolIndex()
{
	return PoolIndex;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/ScratchMark/PoolEntry_ScratchMark.h"

#include "Components/BillboardComponent.h"
#include "Components/DecalComponent.h"

LLM_DEFINE_TAG(APoolEntry_ScratchMark);
APoolEntry_ScratchMark::APoolEntry_ScratchMark()
{
	LLM_SCOPE_BYTAG(APoolEntry_ScratchMark);
	RootIndicator = CreateDefaultSubobject<UBillboardComponent>(TEXT("RootIndicator"));
	SetRootComponent(RootIndicator);
	ScratchMarkDecalComponentWall = CreateDefaultSubobject<UDecalComponent>(TEXT("ScratchMarkDecalComponentWall"));
	ScratchMarkDecalComponentWall->SetupAttachment(RootComponent);
	ScratchMarkDecalComponentWall->bDestroyOwnerAfterFade = false;
	ScratchMarkDecalComponentFloor = CreateDefaultSubobject<UDecalComponent>(TEXT("ScratchMarkDecalComponentFloor"));
	ScratchMarkDecalComponentFloor->SetupAttachment(RootComponent);
	ScratchMarkDecalComponentFloor->bDestroyOwnerAfterFade = false;
}

void APoolEntry_ScratchMark::SetActive(bool IsActive, APawn* InInstigator)
{
	Super::SetActive(IsActive, InInstigator);
	if (IsActive)
	{
		float Pitch =0.f, Yaw =0.f, Roll =0.f;
		// Pitch = FMath::FRandRange(-30.f, 30.f);
		// Roll = FMath::FRandRange(-30.f, 30.f);
		Yaw = FMath::FRandRange(-90.f, 90.f);
		SetActorRotation(FRotator(Pitch, Yaw, Roll));
		FTimerHandle DeactivateTimerHandle;
		GetWorldTimerManager().SetTimer(DeactivateTimerHandle, this, &APoolEntry_ScratchMark::Deactivate, 6.f, false);
	}
}


void APoolEntry_ScratchMark::Deactivate()
{
	Super::Deactivate();
}

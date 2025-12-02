// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Subsystem/DBDObjectAuraSubsystem.h"

#include "MMJ/Object/Interactable/DBDObject.h"
//#include "Shared/DataAsset/DBDMPC.h"
#include "GameFramework/PlayerState.h"
#include "Shared/GameFramework/DBDGameStateBase.h"

void UDBDObjectAuraSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (ADBDGameStateBase* GameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
	{
		
	}
	
	// static ConstructorHelpers::FObjectFinder<UDBDMPC> MPCAsset(
	// 	TEXT("/Game/_BP/Shared/Aura/DBDMPC.DBDMPC")
	// );
	// if (MPCAsset.Succeeded())
	// {
	// 	AuraMPC = MPCAsset.Object;
	// }
}

void UDBDObjectAuraSubsystem::SetAuraState(ADBDObject* Object, APlayerState* PlayerState, int32 StencilValue, float AuraDuration)
{
	if (!Object) return;
	if (!CastChecked<ADBDObject>(Object)) return;
	
	FStencilMap StencilMap;
	StencilMap.PlayerState = PlayerState;
	StencilMap.StencilValue = StencilValue;


	TWeakObjectPtr<ADBDObject> WeakObject = Object;
	if (AuraDuration > 0.f)
	{
		FVector PlayerLocation = PlayerState->GetPawn()->GetActorLocation();
		FVector ObjectLocation = Object->GetActorLocation();
		float Distance = FVector::Distance(PlayerLocation, ObjectLocation);
		if (Distance > 1000.f)
		{
			StencilMap.bUseBackground = true;
			FTimerHandle AuraTimerHandle;
			
			FTimerDelegate AuraTimerDelegate;
			AuraTimerDelegate.BindUObject(this, &ThisClass::UnSetAuraState, WeakObject.Get(), StencilMap);
			
			GetWorld()->GetTimerManager().SetTimer(
				AuraTimerHandle,
				AuraTimerDelegate,
				AuraDuration,
				false);
			
			Object->StencilMaps.Add(StencilMap);	
		}
		
	}
	else
	{
		Object->StencilMaps.Add(StencilMap);
	}

}

void UDBDObjectAuraSubsystem::SetAuraState(ADBDCharacter* Character, APlayerState* PlayerState, int32 StencilValue,
	float AuraDuration)
{
	// 추후 구현
}

void UDBDObjectAuraSubsystem::UnSetAuraState(ADBDObject* Object, const FStencilMap StencilMap)
{
	if (!Object) return;

	if (Object->StencilMaps.Find(StencilMap) != INDEX_NONE)
	{
		Object->StencilMaps.Remove(StencilMap);
	}

}

void UDBDObjectAuraSubsystem::UnSetAuraState(ADBDCharacter* Character, const FStencilMap StencilMap)
{
	// 추후 구현
}

void UDBDObjectAuraSubsystem::UnSetAuraState(ADBDObject* Object, APlayerState* PlayerState)
{
	if (!Object) return;

	FStencilMap StencilMap;
	StencilMap.PlayerState = PlayerState;
	
	if (Object->StencilMaps.Find(StencilMap) != INDEX_NONE)
	{
		Object->StencilMaps.Remove(StencilMap);
	}
}

void UDBDObjectAuraSubsystem::UnSetAuraState(ADBDObject* Object, APlayerState* PlayerState, int32 StencilValue)
{
	if (!Object) return;

	FStencilMap StencilMap;
	StencilMap.PlayerState = PlayerState;
	StencilMap.StencilValue = StencilValue;
	
	if (Object->StencilMaps.Find(StencilMap) != INDEX_NONE)
	{
		Object->StencilMaps.Remove(StencilMap);
	}
}

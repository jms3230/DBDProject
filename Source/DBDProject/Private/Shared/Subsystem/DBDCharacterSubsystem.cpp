// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Subsystem/DBDCharacterSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/ScratchMark/PoolEntry_ScratchMark.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Interactable/Obj_ExitDoor.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/GameFramework/DBDGameInstance.h"
#include "Shared/GameFramework/DBDGameMode.h"
#include "Shared/GameFramework/DBDGameStateBase.h"
#include "Shared/ObjectPool/GenericObjectPool.h"
#include "Shared/Subsystem/DBDObjectAuraSubsystem.h"


bool FDBDCharacterAuraInfo::operator==(const FDBDCharacterAuraInfo& right) const
{
	return DBDCharacter == right.DBDCharacter;
}

FDBDCharacterAuraInfo::FDBDCharacterAuraInfo(ADBDCharacter* InDBDCharacter)
{
	DBDCharacter = InDBDCharacter;
	StencilValue = 0;
	AuraInstigators.Empty();
}


uint32 GetTypeHash(const FDBDCharacterAuraInfo& AuraInfo)
{
	return GetTypeHash(AuraInfo.DBDCharacter);
}

void UDBDCharacterSubsystem::RegisterKiller(AKillerCharacter* KillerCharacter)
{
	Killer = KillerCharacter;
	CharacterAuraInfoContainer.AddUnique(FDBDCharacterAuraInfo(KillerCharacter));

	// TODO :: 로비�면 만들곸원�만채워졌을 �의 �수�바인�전 �요.
	if (KillerCharacter->HasAuthority())
	{
		if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
		{
			DBDGameState->Killer = KillerCharacter;
			BindingPlayerCharacter(KillerCharacter);

			if (UDBDObjectAuraSubsystem* AuraSystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>())
			{
				for (AObj_Generator* Generator : DBDGameState->Generators)
				{
					AuraSystem->SetAuraState(Generator, KillerCharacter->GetPlayerState(), 1);
				}
			}
		}
	}
}

void UDBDCharacterSubsystem::UnregisterKiller(AKillerCharacter* KillerCharacter)
{
	if (Killer == KillerCharacter)
	{
		Killer = nullptr;
		for (FDBDCharacterAuraInfo& AuraInfo : CharacterAuraInfoContainer)
		{
			if (AuraInfo.DBDCharacter == KillerCharacter)
			{
				CharacterAuraInfoContainer.Remove(AuraInfo);
			}
			break;
		}
	}

	// TODO :: 로비�면 만들곸원�만채워졌을 �의 �수�바인�전 �요.
	if (KillerCharacter->HasAuthority())
	{
		if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
		{
			if (DBDGameState->Killer == KillerCharacter)
			{
				DBDGameState->Killer = nullptr;
				UnBindingPlayerCharacter(KillerCharacter);
			}
		}
	}
}

void UDBDCharacterSubsystem::RegisterSurvivor(ASurvivorCharacter* SurvivorCharacter)
{
	Survivors.AddUnique(SurvivorCharacter);
	CharacterAuraInfoContainer.AddUnique(FDBDCharacterAuraInfo(SurvivorCharacter));
	if (SurvivorCharacter->HasAuthority())
	{
		if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
		{
			DBDGameState->Survivors.AddUnique(SurvivorCharacter);
			BindingPlayerCharacter(SurvivorCharacter);
		}
	}
}

void UDBDCharacterSubsystem::UnregisterSurvivor(ASurvivorCharacter* SurvivorCharacter)
{
	Survivors.Remove(SurvivorCharacter);
	for (FDBDCharacterAuraInfo& AuraInfo : CharacterAuraInfoContainer)
	{
		if (AuraInfo.DBDCharacter == SurvivorCharacter)
		{
			CharacterAuraInfoContainer.Remove(AuraInfo);
		}
		break;
	}

	// TODO :: 로비�면 만들곸원�만채워졌을 �의 �수�바인�전 �요.
	if (SurvivorCharacter->HasAuthority())
	{
		if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
		{
			DBDGameState->Survivors.Remove(SurvivorCharacter);
			UnBindingPlayerCharacter(SurvivorCharacter);
		}
	}
}

void UDBDCharacterSubsystem::PrintAllCharacter()
{
	if (ADBDGameStateBase* DBDGameState = Cast<ADBDGameStateBase>(GetWorld()->GetGameState()))
	{
		if (DBDGameState->Killer)
		{
			UE_LOG(LogTemp, Warning, TEXT("JMS : UDBDCharacterSubsystem : Killer : %s"),
			       *DBDGameState->Killer->GetName());
		}
		for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
		{
			UE_LOG(LogTemp, Warning, TEXT("JMS : UDBDCharacterSubsystem : Survivor : %s"), *Survivor->GetName());
		}
	}
}

AKillerCharacter* UDBDCharacterSubsystem::GetKiller() const
{
	return Killer;
}

ASurvivorCharacter* UDBDCharacterSubsystem::GetSurvivorByIndex(int32 Index) const
{
	return Survivors[Index];
}

TArray<ASurvivorCharacter*> UDBDCharacterSubsystem::GetSurvivors()
{
	return Survivors;
}

// JMS : 거리와 조건 태그들을 활용해 생존자의 오라를 활성화(EX: SelfCare, Bond)
void UDBDCharacterSubsystem::EnableSurvivorAuraWithDistanceAndTag(UObject* AuraInstigator, ADBDCharacter* EffectOwner,
                                                                  float Distance,
                                                                  FGameplayTagContainer RequiredTags,
                                                                  FGameplayTagContainer BlockedTags)
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UDBDCharacterSubsystem::ShowAuraAfterCheckSurvivorDistanceAndTag, AuraInstigator,
	                          EffectOwner,
	                          Distance,
	                          RequiredTags, BlockedTags);
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate,
	                                       AuraConditionCheckInterval, true);
	AuraConditionTimerHandles.Add(TimerHandle);

	RequestAuraRefresh();
}

void UDBDCharacterSubsystem::ApplyGEWithSurvivorWithinDistance(ADBDCharacter* Target, float Distance,
                                                               TSubclassOf<UGameplayEffect> GE)
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UDBDCharacterSubsystem::CheckSurvivorDistanceAndApplyEffect, Target,
	                          Distance, GE);
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate,
	                                       ApplyGEToSelfDistanceCheckInterval, true);
	DistanceCheckTimerHandles.Add(TimerHandle);
}

void UDBDCharacterSubsystem::ApplyGEToSurvivorsWithinDistance(ADBDCharacter* EffectOwner, float Distance,
                                                              TSubclassOf<UGameplayEffect> GE)
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UDBDCharacterSubsystem::CheckDistanceAndRefreshEffectToOthers, EffectOwner,
	                          Distance, GE);
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate,
	                                       ApplyGEToOthersDistanceCheckInterval, true);
	DistanceCheckTimerHandles.Add(TimerHandle);
}

void UDBDCharacterSubsystem::RequestAuraRefresh()
{
	if (bIsAuraRefreshing)
	{
		return;
	}
	bIsAuraRefreshing = true;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UDBDCharacterSubsystem::RefreshAura);
	GetWorld()->GetTimerManager().SetTimer(AuraRefreshTimerHandle, TimerDelegate,
	                                       AuraRefreshInterval, true);
}

void UDBDCharacterSubsystem::RefreshAura()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UDBDCharacterSubsystem::RefreshAura");
	for (FDBDCharacterAuraInfo& AuraInfo : CharacterAuraInfoContainer)
	{
		if (AuraInfo.AuraInstigators.Num() > 0)
		{
			AuraInfo.DBDCharacter->EnableAura(AuraInfo.StencilValue);
		}
		else
		{
			AuraInfo.DBDCharacter->DisableAura();
		}
	}
}

void UDBDCharacterSubsystem::ShowAuraAfterCheckSurvivorDistanceAndTag(UObject* AuraInstigator,
                                                                      ADBDCharacter* EffectOwner, float Distance,
                                                                      FGameplayTagContainer RequiredTags,
                                                                      FGameplayTagContainer BlockedTags)
{
	for (FDBDCharacterAuraInfo& AuraInfo : CharacterAuraInfoContainer)
	{
		if ((RequiredTags.Num() <= 0 || AuraInfo.DBDCharacter->GetAbilitySystemComponent()->
		                                         HasAnyMatchingGameplayTags(RequiredTags))
			&&
			!AuraInfo.DBDCharacter->GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(BlockedTags) &&
			AuraInfo.DBDCharacter->GetDistanceTo(EffectOwner) <= Distance && AuraInfo.DBDCharacter != EffectOwner)
		{
			AuraInfo.AuraInstigators.Add(AuraInstigator);
			AuraInfo.StencilValue = 1;
		}
		else
		{
			AuraInfo.AuraInstigators.Remove(AuraInstigator);
		}
	}
}

void UDBDCharacterSubsystem::CheckSurvivorDistanceAndApplyEffect(ADBDCharacter* Target, float Distance,
                                                                 TSubclassOf<UGameplayEffect> GE)
{
	int32 SurvivorCount = 0;
	Target->GetAbilitySystemComponent()->RemoveActiveGameplayEffectBySourceEffect(
		GE, Target->GetAbilitySystemComponent());
	TArray<ASurvivorCharacter*> NearbySurvivors;
	for (ASurvivorCharacter* Survivor : Survivors)
	{
		if (Survivor != Target && Survivor->GetDistanceTo(Target) <= Distance)
		{
			NearbySurvivors.Add(Survivor);
		}
	}
	if (NearbySurvivors.Num() > 0)
	{
		FGameplayEffectContextHandle ContextHandle = Target->GetAbilitySystemComponent()->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpecHandle = Target->GetAbilitySystemComponent()->MakeOutgoingSpec(
			GE, static_cast<float>(NearbySurvivors.Num()),
			ContextHandle);
		for (ASurvivorCharacter* Survivor : NearbySurvivors)
		{
			Target->GetAbilitySystemComponent()->BP_ApplyGameplayEffectSpecToTarget(
				EffectSpecHandle, Survivor->GetAbilitySystemComponent());
		}
		Target->GetAbilitySystemComponent()->BP_ApplyGameplayEffectSpecToSelf(
			EffectSpecHandle);
	}
}

void UDBDCharacterSubsystem::CheckDistanceAndRefreshEffectToOthers(ADBDCharacter* EffectOwner, float Distance,
                                                                   TSubclassOf<UGameplayEffect> GE)
{
	for (ASurvivorCharacter* Survivor : Survivors)
	{
		if (Survivor != EffectOwner && Survivor->GetDistanceTo(EffectOwner) <= Distance)
		{
			FGameplayEffectContextHandle ContextHandle = EffectOwner->GetAbilitySystemComponent()->MakeEffectContext();
			FGameplayEffectSpecHandle EffectSpecHandle = EffectOwner->GetAbilitySystemComponent()->MakeOutgoingSpec(
				GE, 0,
				ContextHandle);
			EffectOwner->GetAbilitySystemComponent()->BP_ApplyGameplayEffectSpecToTarget(
				EffectSpecHandle, Survivor->GetAbilitySystemComponent());
		}
	}
}

void UDBDCharacterSubsystem::EnableScratchMarkOnEverySurvivor()
{
	for (ASurvivorCharacter* Survivor : Survivors)
	{
		Survivor->SprintTagUpdateDelegate.AddDynamic(this, &UDBDCharacterSubsystem::LeaveScratchMarkOnSurvivorSprint);
	}
}

void UDBDCharacterSubsystem::EnableScratchMarkOnCurrentSurvivor(ASurvivorCharacter* Survivor)
{
	if (Survivor)
	{
		Survivor->SprintTagUpdateDelegate.AddDynamic(this, &UDBDCharacterSubsystem::LeaveScratchMarkOnSurvivorSprint);
	}
}


void UDBDCharacterSubsystem::BindingPlayerCharacter(ADBDCharacter* Player)
{
	if (AKillerCharacter* KillerCharacter = Cast<AKillerCharacter>(Player))
	{
		if (ADBDGameMode* DBDGameMode = Cast<ADBDGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (KillerCharacter->GetAbilitySystemComponent())
			{
				KillerCharacter->GetAbilitySystemComponent()->RegisterGameplayTagEvent(
					               DBDGameplayTags::Killer_Common_Status_Carrying,
					               EGameplayTagEventType::NewOrRemoved)
				               .AddUObject(DBDGameMode, &ADBDGameMode::OnKillerTagChange);
			}
		}
	}
	else
	{
		if (ADBDGameMode* DBDGameMode = Cast<ADBDGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Dying,
				                              EGameplayTagEventType::NewOrRemoved)
				   .AddUObject(DBDGameMode, &ADBDGameMode::OnSurvivorTagChange);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Captured_Killer,
				                              EGameplayTagEventType::NewOrRemoved)
				   .AddUObject(DBDGameMode, &ADBDGameMode::OnSurvivorTagChange);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Captured_Hook,
				                              EGameplayTagEventType::NewOrRemoved)
				   .AddUObject(DBDGameMode, &ADBDGameMode::OnSurvivorTagChange);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Dead,
				                              EGameplayTagEventType::NewOrRemoved)
				   .AddUObject(DBDGameMode, &ADBDGameMode::OnSurvivorTagChange);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Escaped,
				                              EGameplayTagEventType::NewOrRemoved)
				   .AddUObject(DBDGameMode, &ADBDGameMode::OnSurvivorTagChange);
			}
		}
	}
}

void UDBDCharacterSubsystem::UnBindingPlayerCharacter(ADBDCharacter* Player)
{
	if (AKillerCharacter* KillerCharacter = Cast<AKillerCharacter>(Player))
	{
		if (ADBDGameMode* DBDGameMode = Cast<ADBDGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (KillerCharacter->GetAbilitySystemComponent())
			{
				KillerCharacter->GetAbilitySystemComponent()->RegisterGameplayTagEvent(
					               DBDGameplayTags::Killer_Common_Status_Carrying, EGameplayTagEventType::NewOrRemoved)
				               .RemoveAll(DBDGameMode);
			}
		}
	}
	else
	{
		if (ADBDGameMode* DBDGameMode = Cast<ADBDGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Dying,
				                              EGameplayTagEventType::NewOrRemoved)
				   .RemoveAll(DBDGameMode);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Captured_Killer,
				                              EGameplayTagEventType::NewOrRemoved)
				   .RemoveAll(DBDGameMode);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Captured_Hook,
				                              EGameplayTagEventType::NewOrRemoved)
				   .RemoveAll(DBDGameMode);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Dead,
				                              EGameplayTagEventType::NewOrRemoved)
				   .RemoveAll(DBDGameMode);
				ASC->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Escaped,
				                              EGameplayTagEventType::NewOrRemoved)
				   .RemoveAll(DBDGameMode);
			}
		}
	}
}


void UDBDCharacterSubsystem::LeaveScratchMarkOnSurvivorSprint(ASurvivorCharacter* Survivor, int32 NewCount)
{
	if (NewCount > 0)
	{
		LeaveScratchMarkTimerDelegate.BindUObject(this, &UDBDCharacterSubsystem::SpawnScratchMarkOnSurvivorLocation,
		                                          Survivor);
		GetWorld()->GetTimerManager().SetTimer(LeaveScratchMarkTimerHandle, LeaveScratchMarkTimerDelegate,
		                                       LeaveScratchMarkInterval, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(LeaveScratchMarkTimerHandle);
		LeaveScratchMarkTimerHandle.Invalidate();
	}
}

void UDBDCharacterSubsystem::SpawnScratchMarkOnSurvivorLocation(ASurvivorCharacter* Survivor)
{
	APoolEntry_ScratchMark* ScratchMarkActor = Survivor->GetScratchMarkFromPool();
	ScratchMarkActor->SetActorLocation(Survivor->GetActorLocation());
	ScratchMarkActor->SetActive(true, Survivor);
	// FActorSpawnParameters SpawnParams;
	// SpawnParams.Owner = Survivor;
	// SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// APoolEntry_ScratchMark* SpawnedScratchMark = GetWorld()->SpawnActor<APoolEntry_ScratchMark>(SpawnParams);
	// SpawnedScratchMark->SetActorLocation(Survivor->GetActorLocation());
	// FTimerDelegate DestroyScratchMarkDelegate;
	// DestroyScratchMarkDelegate.BindUObject(this, &UDBDCharacterSubsystem::DestroySpawnedScratchMark,
	//                                        SpawnedScratchMark);
	// FTimerHandle DestroyScratchMarkTimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(DestroyScratchMarkTimerHandle, DestroyScratchMarkDelegate, 6.f, false);
}

void UDBDCharacterSubsystem::DestroySpawnedScratchMark(APoolEntry_ScratchMark* SpawnedScratchMark)
{
	SpawnedScratchMark->Destroy();
}

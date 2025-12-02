// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/GameFramework/DBDGameMode.h"

#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "MMJ/Object/Subsystem/EntitySpawnSubsystem.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/GameFramework/DBDGameStateBase.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDEnum.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/AssetManager/DBDAssetManager.h"
#include "Shared/Character/DBDCharacter.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Controller/DBDPlayerController.h"
#include "Shared/DataAsset/DBDDataBase.h"
#include "Shared/GameFramework/DBDGameInstance.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/GameFramework/Lobby/LobbyPlayerState.h"
#include "Shared/Subsystem/DBDObjectAuraSubsystem.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/Subsystem/DBDEndGameSubsystem.h"
#include "Shared/Subsystem/DBDObjectObserver.h"
#include "Shared/UI/DBDHUD.h"
#include "Shared/UI/GeneratorIcon.h"

ADBDGameMode::ADBDGameMode()
{
	//초기
	MaxPlayerCount = 5;
	KillerSpawnTransform = FTransform::Identity;
	SurvivorPlayerStarts.Empty();
	SurvivorSpawnCurrentIndex = 0;
	bUseSeamlessTravel = true;
}

void ADBDGameMode::StartPlay()
{

	StartInGameAssetLoading();
}

void ADBDGameMode::BeginPlay()
{
	Super::BeginPlay();

	DBDGameState = Cast<ADBDGameStateBase>(GameState);

	//YHG : PlayerStart 받아�기
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStartActors);
	if (PlayerStartActors.IsEmpty())
	{
		return;
	}
	TArray<APlayerStart*> PlayerStarts;
	for (AActor* PlayerStartActor : PlayerStartActors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(PlayerStartActor);
		PlayerStarts.Add(PlayerStart);
	}

	//YHG : �러 캐릭�폰 지�덤 �정
	TArray<APlayerStart*> KillerPlayerStarts;
	for (APlayerStart* PlayerStart : PlayerStarts)
	{
		if (PlayerStart->PlayerStartTag == FName("Killer"))
		{
			KillerPlayerStarts.Add(PlayerStart);
		}
	}
	if (KillerPlayerStarts.IsEmpty())
	{
		return;
	}
	int32 KillerSpawnRandomIndex = FMath::RandRange(0, KillerPlayerStarts.Num() - 1);
	APlayerStart* KillerPlayerStart = KillerPlayerStarts[KillerSpawnRandomIndex];
	KillerSpawnTransform = KillerPlayerStart->GetTransform();

	//YHG : PlayerStart Spawn Logic -> MMJ Encoding Error Change to English
	//The first PlayerStart  in a group of 4
	for (APlayerStart* PlayerStart : PlayerStarts)
	{
		if (PlayerStart->PlayerStartTag == FName("Survivor"))
		{
			SurvivorPlayerStarts.Add(PlayerStart);
		}
	}
	if (SurvivorPlayerStarts.IsEmpty())
	{
		return;
	}
	int32 SurvivorStartCount = SurvivorPlayerStarts.Num();
	int32 SurvivorSpawnRandomIndex = FMath::RandRange(0, SurvivorStartCount - 1);
	int32 MaxSurvivorCount = MaxPlayerCount - 1;
	int32 SurvivorSpawnStartIndex = (SurvivorSpawnRandomIndex / (MaxSurvivorCount)) * (MaxSurvivorCount);
	SurvivorSpawnCurrentIndex = SurvivorSpawnStartIndex;

	
	FTimerHandle InitTimer;
	GetWorld()->GetTimerManager().SetTimer(
		InitTimer,
		this,
		&ThisClass::CharacterInit,
		10.f,
		false
		);
}

void ADBDGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (ADBDPlayerState* PS = NewPlayer->GetPlayerState<ADBDPlayerState>())
	{
		int32 CurrentCount = GameState->PlayerArray.Num();
		if (CurrentCount == 1)
		{
			PS->SetPlayerRole(EPlayerRole::Killer);
		}
		else
		{
			PS->SetPlayerRole(EPlayerRole::Survivor);
		}
	}
	// �� 지�에 HandleStartingNewPlayer() �출(캐릭�폰)
	Super::PostLogin(NewPlayer);
}

void ADBDGameMode::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
	Super::SwapPlayerControllers(OldPC, NewPC);
}


void ADBDGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	ADBDPlayerState* PS = NewPlayer->GetPlayerState<ADBDPlayerState>();
	if (!PS)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	PS->KillerLoadout.Addons.Add(PS->KillerLoadout.Addon1);
	PS->KillerLoadout.Addons.Add(PS->KillerLoadout.Addon2);
	PS->SurvivorLoadout.ItemAddons.Add(PS->SurvivorLoadout.ItemInfo.Addon1);
	PS->SurvivorLoadout.ItemAddons.Add(PS->SurvivorLoadout.ItemInfo.Addon2);
	if (PS->GetPlayerRole() == EPlayerRole::Killer)
	{
		if (!KillerCharacterClass)
		{
			return;
		}
		APawn* KillerPawn = GetWorld()->SpawnActor<APawn>(KillerCharacterClass, KillerSpawnTransform, SpawnParams);
		NewPlayer->Possess(KillerPawn);
	}
	else
	{
		if (SurvivorPlayerStarts.IsEmpty())
		{
			return;
		}

		if (!SurvivorPlayerStarts.IsValidIndex(SurvivorSpawnCurrentIndex)) return;
		FTransform SurvivorSpawnTransform = SurvivorPlayerStarts[SurvivorSpawnCurrentIndex]->GetTransform();

		APawn* SurvivorPawn = SpawnSurvivorCharacter(PS, SurvivorSpawnTransform);
		SurvivorSpawnCurrentIndex++;

		NewPlayer->Possess(SurvivorPawn);
	}
}

void ADBDGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	if (APlayerController* NewPC = Cast<APlayerController>(C))
	{
		NewPC->ChangeState(NAME_Playing);
	}
}

void ADBDGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);
}

void ADBDGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
}

AActor* ADBDGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	//YHG : PlayerStart 받아�기
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStartActors);
	if (PlayerStartActors.IsEmpty())
	{
		return nullptr;
	}
	TArray<APlayerStart*> PlayerStarts;
	for (AActor* PlayerStartActor : PlayerStartActors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(PlayerStartActor);
		PlayerStarts.Add(PlayerStart);
	}

	if (ADBDPlayerState* PS = Cast<ADBDPlayerController>(Player)->GetPlayerState<ADBDPlayerState>())
	{
		if (PS->GetPlayerRole() == EPlayerRole::Killer)
		{
			// KillerStart
			TArray<APlayerStart*> KillerPlayerStarts;
			for (APlayerStart* PlayerStart : PlayerStarts)
			{
				if (PlayerStart->PlayerStartTag == FName("Killer"))
				{
					KillerPlayerStarts.Add(PlayerStart);
				}
			}
			if (KillerPlayerStarts.IsEmpty())
			{
				return nullptr;
			}

			int32 KillerSpawnRandomIndex = FMath::RandRange(0, KillerPlayerStarts.Num() - 1);
			APlayerStart* KillerPlayerStart = KillerPlayerStarts[KillerSpawnRandomIndex];
			KillerSpawnTransform = KillerPlayerStart->GetTransform();
			return KillerPlayerStart;
		}
		else
		{
			//YHG : PlayerStart Spawn Logic -> MMJ Encoding Error Change to English
			//The first PlayerStart  in a group of 4
			for (APlayerStart* PlayerStart : PlayerStarts)
			{
				if (PlayerStart->PlayerStartTag == FName("Survivor"))
				{
					SurvivorPlayerStarts.Add(PlayerStart);
				}
			}
			if (SurvivorPlayerStarts.IsEmpty())
			{
				return nullptr;
			}
			int32 SurvivorStartCount = SurvivorPlayerStarts.Num();
			int32 SurvivorSpawnRandomIndex = FMath::RandRange(0, SurvivorStartCount - 1);
			int32 MaxSurvivorCount = MaxPlayerCount - 1;
			int32 SurvivorSpawnStartIndex = (SurvivorSpawnRandomIndex / (MaxSurvivorCount)) * (MaxSurvivorCount);
			SurvivorSpawnCurrentIndex = SurvivorSpawnStartIndex;
			return SurvivorPlayerStarts[SurvivorSpawnStartIndex];
		}
	}
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ADBDGameMode::StartInGameAssetLoading()
{
	FString TargetPath = TEXT("/Game/Assets/YHG");

	FARFilter AssetFilter;
	AssetFilter.PackagePaths.Add(*TargetPath);
	AssetFilter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	AssetFilter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());
	AssetFilter.bRecursivePaths = true;

	TArray<FAssetData> AssetDatas;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(AssetFilter, AssetDatas);

	TArray<FSoftObjectPath> AssetsToLoad;
	for (const FAssetData& AssetData : AssetDatas)
	{
		// AssetData를 FSoftObjectPath로 변환하여 목록에 추가
		AssetsToLoad.Add(AssetData.GetSoftObjectPath()); 
	}
    
	// 비동기 로드 시작
	if (AssetsToLoad.Num() > 0)
	{
		StartAsyncLoading(AssetsToLoad);
	}
}


void ADBDGameMode::OnInGameAssetLoaded()
{
	InGameAssetLoadHandle.Reset();
	
	Super::StartPlay();
}

void ADBDGameMode::StartAsyncLoading(const TArray<FSoftObjectPath>& AssetsToLoad)
{
	UDBDAssetManager& AssetManager = UDBDAssetManager::Get();

	// 로딩 요청
	InGameAssetLoadHandle = AssetManager.GetStreamableManager().RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &ADBDGameMode::OnInGameAssetLoaded) // 완료 콜백 연결
	);
}



void ADBDGameMode::CharacterInit()
{
	if (!IsValid(DBDGameState)) return;

	

	int32 RequiredGeneratorRepairCount = DBDGameState->Survivors.Num() + 1;
	DBDGameState->RequiredGeneratorRepairCount = RequiredGeneratorRepairCount;
	
	if (UDBDCharacterSubsystem* Observer = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		DBDGameState->Killer = Observer->GetKiller();
		DBDGameState->Survivors = Observer->GetSurvivors();

		if (!DBDGameState->Survivors.IsEmpty())
		{
			for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
			{
				
			}
		}
	}
}


void ADBDGameMode::OnSurvivorTagChange(FGameplayTag Tag, int32 Count)
{
	/**
	 * 게임�로�에 변�� 줈는 �그�별
	 * Ex1) �출�개����UI�상 변-> Dying or Hooked
	 * Ex2) Hooked�태�존�� �는�존�� �외모든 �존�� �망 or �출�을-> Dead or Escaped
	 * Ex3) �인마� 발전�갌�갈출�공�ga�서 status�그���해강제롔드게임 가-> �른 �수�서 �요가 �음.
	 */
	FName LastTagName = UDBDBlueprintFunctionLibrary::GetTagLastName(Tag);
	if (LastTagName == "Dying" or LastTagName == "Captured" or LastTagName == "Hooked")
	{
		bool bFound = false;
		for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
		{			
			if (UAbilitySystemComponent* ASC = Survivor->GetAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Dying)
					or ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Captured_Killer)
					or ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Captured_Hook))
				{
					bFound = true;
				}
			}
		}
		DBDGameState->SetIsSlowEscape(bFound);
	}

	CheckGameCondition();
}

void ADBDGameMode::OnKillerTagChange(FGameplayTag Tag, int32 Count)
{
	FName LastTagName = UDBDBlueprintFunctionLibrary::GetTagLastName(Tag);
	if (LastTagName == "Carrying")
	{
		UDBDObjectAuraSubsystem* AuraSubsystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>();
		if (!AuraSubsystem)
		{
			return;
		}
		if (DBDGameState)
		{
			for (AObj_Hook* Hook : DBDGameState->Hooks)
			{
				if (Count > 0)
				{
					AuraSubsystem->SetAuraState(Hook, DBDGameState->Killer->GetPlayerState(), 1);
				}
				else
				{
					AuraSubsystem->UnSetAuraState(Hook, DBDGameState->Killer->GetPlayerState(), 1);
				}
			}
		}
	}
}

void ADBDGameMode::CheckGameCondition()
{
	if (!IsValid(DBDGameState)) return;

	TArray<ADBDCharacter*> EndPlayers;
	
	int32 Alive = 0, Else = 0;

	for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
	{
		if (!IsValid(Survivor)) continue;
		
		if (UAbilitySystemComponent* ASC = Survivor->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Dead)
				or ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Escaped)
				or ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Captured_Hook))
			{
				Else++;
				EndPlayers.AddUnique(Survivor);
			}
			else
			{
				Alive++;
			}
		}
	}
	// 게임 �에 �아�니�존�� �고 Dead or Escape or Hooked�체
	if (Alive == 0)
	{
		EndGameState();
	}
}

void ADBDGameMode::EndGameState()
{
	if (!IsValid(DBDGameState)) return;

	DBDGameState->SetGameEnd();
	for (APlayerState* ExistPS : DBDGameState->PlayerArray)
	{
		if (ADBDPlayerState* PS = Cast<ADBDPlayerState>(ExistPS))
		{
			if (PS->GetPlayerRole() == EPlayerRole::Killer)
			{
				PS->SetPlayerEndState(EPlayerEndState::Killer);
			}
			else
			{
				if (ASurvivorCharacter* Survivor = Cast<ASurvivorCharacter>(PS->GetPlayerController()->GetCharacter()))
				{
					if (UAbilitySystemComponent* ASC = Survivor->GetAbilitySystemComponent())
					{
						if (ASC->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Escaped))
						{
							PS->SetPlayerEndState(EPlayerEndState::Escape);
						}
						else
						{
							PS->SetPlayerEndState(EPlayerEndState::Sacrifice);
						}
					}
				}
			}
		}
	}
	MoveToEndLevel();
}

void ADBDGameMode::MoveToEndLevel()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();

		if (PlayerController && PlayerController->IsLocalController())
		{
			PlayerController->EndPlay(EEndPlayReason::Type::RemovedFromWorld);
		}
	}
	
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateLambda([&]()
		{
			GetWorld()->ServerTravel("/Game/_BP/Shared/Level/EndLevel?listen", true, false);
		}),
		10.f,
		false);
	
}

void ADBDGameMode::OnGeneratorComplete(AActor* Object, AActor* Interactor)
{
	if (DBDGameState)
	{
		int32 RepairCount = 0;

		for (ADBDObject* Generator : DBDGameState->Generators)
		{
			if (Generator->GetInteractableComponent()->IsComplete())
			{
				RepairCount++;
			}
		}

		if (RepairCount >= DBDGameState->Survivors.Num() + 1)
		{
			if (UDBDObjectObserver* ObjectObserver = GetWorld()->GetSubsystem<UDBDObjectObserver>())
			{
				ObjectObserver->ActivateExitDoors();
			}
			if (UDBDObjectAuraSubsystem* AuraSystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>())
			{
				for (AObj_ExitDoor* ExitDoor : DBDGameState->ExitDoors)
				{
					for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
					{
						AuraSystem->SetAuraState(ExitDoor, Survivor->GetPlayerState(), 7, 5.f);
					}
					if (IsValid(DBDGameState->Killer))
					{
						AuraSystem->SetAuraState(ExitDoor, DBDGameState->Killer->GetPlayerState(), 7, 5.f);
					}
				}
			}
			for (AObj_Generator* Generator : DBDGameState->Generators)
			{
				if (!Generator->GetInteractableComponent()->IsComplete())
				{
					Generator->GetInteractableComponent()->CompleteInteraction(this);
				}
			}
		}

		
		{
			// Set Aura & Set UI(Required GeneratorCount)
			int32 RequiredRepairCount = DBDGameState->Survivors.Num() + 1 - RepairCount;
			DBDGameState->RequiredGeneratorRepairCount = RequiredRepairCount;
			
			if (ADBDObject* Generator = Cast<ADBDObject>(Object))
			{
				if (UDBDObjectAuraSubsystem* AuraSystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>())
				{
					for (ASurvivorCharacter* Survivor : DBDGameState->Survivors)
					{
						AuraSystem->SetAuraState(Generator, Survivor->GetPlayerState(), 2, 5.f);
					}
					if (IsValid(DBDGameState->Killer))
					{
						AuraSystem->UnSetAuraState(Generator, DBDGameState->Killer->GetPlayerState(), 1);
						AuraSystem->SetAuraState(Generator, DBDGameState->Killer->GetPlayerState(), 2, 5.f);
					}
				}
			}
		}
	}
}

void ADBDGameMode::OnGeneratorInteract(AActor* Object, AActor* Interactor)
{
	if (DBDGameState)
	{
		if (DBDGameState->Killer)
		{
			if (UDBDObjectAuraSubsystem* AuraSystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>())
			{
				if (ADBDObject* Generator = Cast<ADBDObject>(Object))
				{
					AuraSystem->SetAuraState(Generator, DBDGameState->Killer->GetPlayerState(), 7, 3.f);
				}
			}
		}
	}
}

void ADBDGameMode::SetEscapeTimer()
{
	if (UDBDEndGameSubsystem* EndGameSubsystem = GetWorld()->GetSubsystem<UDBDEndGameSubsystem>())
	{
		EndGameSubsystem->StartEscapeTimer();
	}
}

ASurvivorCharacter* ADBDGameMode::SpawnSurvivorCharacter(ADBDPlayerState* DBDPS, FTransform SpawnTransform)
{
	UDBDGameInstance* DBDGI = Cast<UDBDGameInstance>(GetGameInstance());
	if (!DBDGI || !DBDPS)
	{
		return nullptr;
	}
	UDataTable* DT = Cast<UDataTable>(
		UAssetManager::Get().GetStreamableManager().LoadSynchronous(
			DBDGI->DBDDB->SurvivorCharacterDB.ToSoftObjectPath()));

	ASurvivorCharacter* SpawnedSurvivor = nullptr;

	if (DT)
	{
		
		FSurvivorCharacterData* SurvivorData = DT->FindRow<
			FSurvivorCharacterData>(DBDPS->SurvivorLoadout.Character, "");
		if (SurvivorData)
		{
			TSubclassOf<ASurvivorCharacter> SurvivorClassFromDB = SurvivorData->CharacterClass;
			if (SurvivorClassFromDB)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnedSurvivor = GetWorld()->SpawnActor<ASurvivorCharacter>(
					SurvivorClassFromDB, SpawnTransform, SpawnParams);
				return SpawnedSurvivor;
			}
		}
	}
	return SpawnedSurvivor;
}

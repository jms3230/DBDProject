// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/GameFramework/DBDGameStateBase.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Interactable/DBDObject.h"
#include "MMJ/Object/GAS/ObjAbilitySystemComponent.h"
#include "MMJ/Object/Interactable/Obj_Exit.h"
#include "MMJ/Object/Interactable/Obj_ExitDoor.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "Net/UnrealNetwork.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDEnum.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Character/DBDCharacter.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Controller/DBDPlayerController.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/Subsystem/DBDEndGameSubsystem.h"
#include "Shared/Subsystem/DBDObjectObserver.h"
#include "Shared/UI/DBDHUD.h"
#include "Shared/UI/DBDWidgetComponent.h"
#include "Shared/UI/EscapeTimerUI.h"

ADBDGameStateBase::ADBDGameStateBase()
{
	bReplicates = true;
}

void ADBDGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ADBDGameStateBase, RequiredGeneratorRepairCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDGameStateBase, RemainingEscapeTime, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDGameStateBase, bIsGameEnded, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDGameStateBase, bIsSlowEscape, COND_None, REPNOTIFY_Always);
}

void ADBDGameStateBase::SetRemainingEscapeTime(float NewTime)
{
	if (HasAuthority())
	{
		RemainingEscapeTime = NewTime;
		OnRep_RemainingEscapeTime(NewTime);
	}
}


void ADBDGameStateBase::OnRep_RemainingEscapeTime(float NewTime)
{
	if (ADBDPlayerController* PC = GetWorld()->GetFirstPlayerController<ADBDPlayerController>())
	{
		if (ADBDCharacter* Character = Cast<ADBDCharacter>(PC->GetCharacter()))
		{
			if (UDBDWidgetComponent* CharacterWidgetComponent = Character->GetWidgetComponent())
			{
				if (CharacterWidgetComponent->PlayerHUD->GetEndGameHUD())
				{
					if (!CharacterWidgetComponent->PlayerHUD->GetEndGameHUD()->IsVisible())
					{
						CharacterWidgetComponent->PlayerHUD->SetEndGameHUD();
					}
					CharacterWidgetComponent->PlayerHUD->UpdateEndGameHUD(NewTime);
				}
			}
		}
	}
}

void ADBDGameStateBase::SetIsSlowEscape(bool bIsSlow)
{
	if (HasAuthority())
	{
		if (bIsSlowEscape != bIsSlow)
		{
			bIsSlowEscape = bIsSlow;
			OnRep_IsSlowEscape();
			if (UDBDEndGameSubsystem* EndGameSubsystem = GetWorld()->GetSubsystem<UDBDEndGameSubsystem>())
			{
				EndGameSubsystem->SetTimerSlow(bIsSlowEscape);
			}
		}
	}
}

void ADBDGameStateBase::OnRep_IsSlowEscape()
{
	if (ADBDPlayerController* PC = GetWorld()->GetFirstPlayerController<ADBDPlayerController>())
	{
		if (ADBDCharacter* Character = Cast<ADBDCharacter>(PC->GetCharacter()))
		{
			if (UDBDWidgetComponent* CharacterWidgetComponent = Character->GetWidgetComponent())
			{
				if (CharacterWidgetComponent->PlayerHUD->GetEndGameHUD())
				{
					CharacterWidgetComponent->PlayerHUD->GetEndGameHUD()->ChangeProgressBar(bIsSlowEscape);
				}
			}
		}
	}
}


void ADBDGameStateBase::SetGameEnd()
{
	if (HasAuthority())
	{
		bIsGameEnded = true;

		OnRep_bIsGameEnded();
	}
}

void ADBDGameStateBase::OnRep_bIsGameEnded()
{
	//MoveToEndGameLevel();
	for (const APlayerState* PS : PlayerArray)
	{
		if (ADBDPlayerController* PC = Cast<ADBDPlayerController>(PS->GetPlayerController()))
		{
			if (PC->IsLocalPlayerController())
			{
				DisableInput(PC);
			}
		}
	}

	for (ASurvivorCharacter* Character : Survivors)
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			//ASC->AddLooseGameplayTag(DBDGameplayTags::Shared_GameState_End);
			ASC->AddReplicatedLooseGameplayTag(DBDGameplayTags::Shared_GameState_End);
		}
	}
}


void ADBDGameStateBase::MoveToEndGameLevel_Implementation()
{
	GetWorld()->ServerTravel("/Game/_BP/MMJ/GameFlow/Level/EndGame", false, true);
}

void ADBDGameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ADBDGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 옵저버 설정
		{
			if (UDBDCharacterSubsystem* Observer = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
			{
				CharacterSubsystem = Observer;
			}
			// 오브젝트
			if (UDBDObjectObserver* Observer = GetWorld()->GetSubsystem<UDBDObjectObserver>())
			{
				ObjectObserver = Observer;

				// 발전기
				//Generators = ObjectObserver->GetGenerators();
				// for (AObj_Generator* Generator : Generators)
				// {
				// 	if (Generator->GetInteractableComponent())
				// 	{
				// 		Generator->GetInteractableComponent()->OnComplete.AddDynamic(this, &ThisClass::ADBDGameStateBase::CheckGeneratorState);
				// 	}
				// }

				// 탈출구
				//ExitDoors = ObjectObserver->GetExitDoors();
				// for (AObj_ExitDoor* ExitDoor : ExitDoors)
				// {
				// 	if (ExitDoor->GetInteractableComponent())
				// 	{
				// 		ExitDoor->GetInteractableComponent()->OnComplete.AddDynamic(this, &ThisClass::SetEscapeTimer);
				// 		ExitDoor->GetInteractableComponent()->OnDestroy.AddDynamic(this, &ThisClass::SetGameEnd);
				// 	}
				// }
			}
		}
	}
}

void ADBDGameStateBase::OnRep_RequiredGeneratorRepairCount()
{
	if (ADBDPlayerController* PC = GetWorld()->GetFirstPlayerController<ADBDPlayerController>())
	{
		if (ADBDCharacter* Character = Cast<ADBDCharacter>(PC->GetCharacter()))
		{
			// JMS: UI수정: 위젯 컴포넌트를 통해 업데이트
			Character->GetWidgetComponent()->OnUpdateGeneratorCount.Broadcast(RequiredGeneratorRepairCount);
			// if (IsValid(Character->PlayerHUD))
			// {
			// 	Character->PlayerHUD->SetLeftGeneratorIcon(RequiredGeneratorRepairCount);
			// }
		}
	}
}

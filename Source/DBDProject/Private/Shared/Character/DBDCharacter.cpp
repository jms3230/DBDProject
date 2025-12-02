// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Character/DBDCharacter.h"
#include "AbilitySystemInterface.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KMJ/Character/KillerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDEnum.h"
#include "Shared/Animation/DBDAnimInstance.h"
#include "Shared/Component/DBDMotionWarpingComponent.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/Controller/DBDPlayerController.h"
#include "Shared/DataAsset/DBDDataBase.h"
#include "Shared/GameFramework/DBDGameInstance.h"
#include "Shared/GameFramework/DBDGameMode.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/Perk/PerkComponent.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/UI/DBDWidgetComponent.h"

ADBDCharacter::ADBDCharacter()
{
	NetUpdateFrequency = 100.0f;
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_Never);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	//GetCharacterMovement()->SetWalkableFloorAngle(55.f);
	GetCharacterMovement()->MaxStepHeight = 30.f;

	UIWidgetComponent = CreateDefaultSubobject<UDBDWidgetComponent>(TEXT("UIWidgetComponent"));
	DBDMotionWarpingComponent = CreateDefaultSubobject<UDBDMotionWarpingComponent>(TEXT("DBDMotionWarpingComponent"));
	InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("InteractorComponent"));
	InteractorComponent->SetupAttachment(GetRootComponent());

	GetCapsuleComponent()->BodyInstance.bLockRotation = true;
	GetCapsuleComponent()->BodyInstance.bLockTranslation = true;
}

ADBDCharacter::ADBDCharacter(const FObjectInitializer& ObjectInitializer)
{
}

UAbilitySystemComponent* ADBDCharacter::GetAbilitySystemComponent() const
{
	// IAbilitySystemInterface* PSASInterface = Cast<IAbilitySystemInterface>(GetPlayerState());
	// if (PSASInterface)
	// {
	// 	return PSASInterface->GetAbilitySystemComponent();
	// }
	// �식�서 구현
	return nullptr;
}

UDBDMotionWarpingComponent* ADBDCharacter::GetDBDMotionWarpingComponent() const
{
	return DBDMotionWarpingComponent;
}

UInteractableComponent* ADBDCharacter::GetInteractableComponent() const
{
	// �식�서 구현
	return nullptr;
}

const TArray<FMotionWarpingInfo> ADBDCharacter::GetMotionWarpingTargets_Implementation()
{
	// �식�서 구현
	return CharacterMotionWarpingTargetInfos;
}

UInteractorComponent* ADBDCharacter::GetInteractorComponent() const
{
	return InteractorComponent;
}

EPlayerRole ADBDCharacter::GetInteractorRole() const
{
	// �식�서 구현
	return EPlayerRole::Survivor;
}

UDBDWidgetComponent* ADBDCharacter::GetWidgetComponent() const
{
	return UIWidgetComponent;
}

void ADBDCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// �식�서 ASC초기�후AuthInitPerks() �출
	// MMJ �정 NewController가 Nullptr�러 발생�서 캐스�정
	if (APlayerController* const PC = Cast<APlayerController>(NewController))
	{
		if (PC->IsLocalController())
		{
			// FInputModeGameOnly InputMode;
			// InputMode.SetConsumeCaptureMouseDown(true);
			// PC->bShowMouseCursor = false;
			// PC->SetInputMode(InputMode);
		}
	}
}

void ADBDCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// FInputModeGameOnly InputMode;
		// InputMode.SetConsumeCaptureMouseDown(true);
		// PC->bShowMouseCursor = false;
		// PC->SetInputMode(InputMode);
	}
}

void ADBDCharacter::AuthInitPerks()
{
	if (!HasAuthority())
	{
		return;
	}
	ADBDPlayerState* PS = Cast<ADBDPlayerState>(GetPlayerState());
	if (!PS)
	{
		return;
	}
	UDBDGameInstance* GI = Cast<UDBDGameInstance>(GetGameInstance());
	if (!GI || !GI->DBDDB)
	{
		return;
	}
	if (PS->GetPlayerRole() == EPlayerRole::Survivor)
	{
		UDataTable* DT = Cast<UDataTable>(
			UAssetManager::Get().GetStreamableManager().LoadSynchronous(
				GI->DBDDB->SurvivorPerkDB.ToSoftObjectPath()));
		if (DT)
		{
			InitializePerks(*DT, PS->SurvivorLoadout.Perks[0], PS->SurvivorLoadout.Perks[1],
			                PS->SurvivorLoadout.Perks[2], PS->SurvivorLoadout.Perks[3]);
		};
	}
	else
	{
		UDataTable* DT = Cast<UDataTable>(
			UAssetManager::Get().GetStreamableManager().LoadSynchronous(
				GI->DBDDB->KillerPerkDB.ToSoftObjectPath()));
		if (DT)
		{
			InitializePerks(*DT, PS->KillerLoadout.Perks[0], PS->KillerLoadout.Perks[1],
			                PS->KillerLoadout.Perks[2], PS->KillerLoadout.Perks[3]);
		};
	}
}

void ADBDCharacter::Client_UpdatePerk_Implementation(UPerkComponent* NewPerk1, UPerkComponent* NewPerk2,
                                                     UPerkComponent* NewPerk3, UPerkComponent* NewPerk4)
{
	if (NewPerk1)
	{
		Perk1 = NewPerk1;
		Perk1->OnOwnerClientSideInitialized();
		Perk1->RegisterComponent();
	}
	if (NewPerk2)
	{
		Perk2 = NewPerk2;
		Perk2->OnOwnerClientSideInitialized();
		Perk2->RegisterComponent();
	}
	if (NewPerk3)
	{
		Perk3 = NewPerk3;
		Perk3->OnOwnerClientSideInitialized();
		Perk3->RegisterComponent();
	}
	if (NewPerk4)
	{
		Perk4 = NewPerk4;
		Perk4->OnOwnerClientSideInitialized();
		Perk4->RegisterComponent();
	}
	// JMS: UI수정: WidgetComponent가 Initialize, Delegate담당
	// OnPerkUIInitialize.Broadcast(this, GetInteractorRole());
}


void ADBDCharacter::InitializePerks(const UDataTable& DataTable, FName Perk1Name, FName Perk2Name, FName Perk3Name,
                                    FName Perk4Name)
{
	if (Perk1Name != NAME_None)
	{
		FPerkData* Perk1Data = DataTable.FindRow<FPerkData>(Perk1Name, "");
		if (Perk1Data)
		{
			TSubclassOf<UPerkComponent> Perk1Class = Perk1Data->PerkClass;
			if (Perk1Class)
			{
				Perk1 = NewObject<UPerkComponent>(this, Perk1Class);
				Perk1->RegisterComponent();
				Perk1->OnServerSideInitialized();
			}
		}
	}
	if (Perk2Name != NAME_None)
	{
		FPerkData* Perk2Data = DataTable.FindRow<FPerkData>(Perk2Name, "");
		if (Perk2Data)
		{
			TSubclassOf<UPerkComponent> Perk2Class = Perk2Data->PerkClass;
			if (Perk2Class)
			{
				Perk2 = NewObject<UPerkComponent>(this, Perk2Class);

				Perk2->RegisterComponent();
				Perk2->OnServerSideInitialized();
			}
		}
	}
	if (Perk3Name != NAME_None)
	{
		FPerkData* Perk3Data = DataTable.FindRow<FPerkData>(Perk3Name, "");
		if (Perk3Data)
		{
			TSubclassOf<UPerkComponent> Perk3Class = Perk3Data->PerkClass;
			if (Perk3Class)
			{
				Perk3 = NewObject<UPerkComponent>(this, Perk3Class);
				Perk3->RegisterComponent();
				Perk3->OnServerSideInitialized();
			}
		}
	}
	if (Perk4Name != NAME_None)
	{
		FPerkData* Perk4Data = DataTable.FindRow<FPerkData>(Perk4Name, "");
		if (Perk4Data)
		{
			TSubclassOf<UPerkComponent> Perk4Class = Perk4Data->PerkClass;
			if (Perk4Class)
			{
				Perk4 = NewObject<UPerkComponent>(this, Perk4Class);
				Perk4->RegisterComponent();
				Perk4->OnServerSideInitialized();
			}
		}
	}
}

void ADBDCharacter::ServerSideInit()
{
	// �식�서 구현
}

void ADBDCharacter::ClientSideInit()
{
	// �식�서 구현
}

void ADBDCharacter::EnableAura(int32 StencilValue)
{
}

void ADBDCharacter::DisableAura()
{
}

void ADBDCharacter::OnRep_AuraStencilMap()
{
	for (const FStencilMap& StencilMap : StencilMaps)
	{
		if (StencilMap.PlayerState)
		{
			if (StencilMap.PlayerState->GetPlayerController())
			{
				if (StencilMap.PlayerState->GetPlayerController()->IsLocalController())
				{
					EnableAura(1);
					SetCustomDepth(StencilMap.StencilValue);
				}
			}
		}
		else
		{
			DisableAura();
		}
	}
}

void ADBDCharacter::SetCustomDepth(int32 value)
{
}

void ADBDCharacter::EnableInputGame()
{
	ADBDPlayerController* PC = Cast<ADBDPlayerController>(GetController());
	if (PC)
	{
		EnableInput(PC);
		FInputModeGameOnly EnableGameMode;
		PC->SetInputMode(EnableGameMode);
		PC->bIsSetSpectatorCam = false;
		FViewTargetTransitionParams TransitionParams;
		TransitionParams.BlendTime = 0.5f;
		PC->SetViewTarget(this, TransitionParams);
	}
}

TArray<UPerkComponent*> ADBDCharacter::GetAllPerks()
{
	TArray<UPerkComponent*> Perks;
	Perks.Add(Perk1);
	Perks.Add(Perk2);
	Perks.Add(Perk3);
	Perks.Add(Perk4);
	return Perks;
}

float ADBDCharacter::PlaySyncMontageFromServer(UAnimMontage* Montage, float PlayRate, FName StartSectionName)
{
	if (HasAuthority())
	{
		Multicast_PlaySyncMontage(Montage, PlayRate, StartSectionName);
		Client_PlaySyncMontage(Montage, PlayRate, StartSectionName);
		return PlayAnimMontage(Montage, PlayRate, StartSectionName);
	}
	else
	{
		return 0.0f;
	}
}

float ADBDCharacter::SyncTransformAndPlayMontageFromServer(FTransform ServerCharacterTransform, UAnimMontage* Montage,
	float PlayRate, FName StartSectionName)
{
	if (HasAuthority())
	{
		Multicast_SyncTransformAndMontage(ServerCharacterTransform, Montage, PlayRate, StartSectionName);
		Client_SyncTransformAndMontage(ServerCharacterTransform, Montage, PlayRate, StartSectionName);
		return PlayAnimMontage(Montage, PlayRate, StartSectionName);
	}
	else
	{
		//Debug::Print(TEXT("JMS19: ADBDCharacter::PlaySyncMontageFromServer Called from Client"),19);
		return 0.0f;
	}
}

void ADBDCharacter::StopSyncMontageFromServer(UAnimMontage* Montage)
{
	if (HasAuthority())
	{
		if (Montage)
		{
			Multicast_StopSyncMontage(Montage);
			Client_StopSyncMontage(Montage);
			StopAnimMontage(Montage);
		}
		else
		{
			Debug::Print(TEXT("JMS19: ADBDCharacter::StopSyncMontageFromServer : Invalid Montage"), 19);
		}
	}
	else
	{
		Debug::Print(TEXT("JMS19: ADBDCharacter::StopSyncMontageFromServer Called from Client"), 19);
	}
}

void ADBDCharacter::LookAtTargetActorFromServer(AActor* TargetActor, float YawOffset)
{
	if (HasAuthority())
	{
		FRotator TargetRotation = FRotator::ZeroRotator;
		if (TargetActor)
		{
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				GetActorLocation(),
				TargetActor->GetActorLocation());
			TargetRotation = FRotator(0, LookAtRotation.Yaw + YawOffset, 0);
			
			K2_SetActorRotation(TargetRotation, false);
		}
		Client_SetSurvivorRotation(TargetRotation);
		Multicast_SetSurvivorRotation(TargetRotation);
	}
}

void ADBDCharacter::LookAtTargetActorLocal(AActor* TargetActor, float YawOffset)
{
	FRotator TargetRotation = FRotator::ZeroRotator;
	if (TargetActor)
	{
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
			GetActorLocation(),
			TargetActor->GetActorLocation());
		TargetRotation = FRotator(0, LookAtRotation.Yaw + YawOffset, 0);
			
		K2_SetActorRotation(TargetRotation, false);
	}
}

float ADBDCharacter::PlaySyncMontageFromOwnerClient(UAnimMontage* Montage, float PlayRate, FName StartSectionName)
{
	if (IsLocallyControlled())
	{
		Server_PlaySyncMontage(Montage, PlayRate, StartSectionName);
		return PlayAnimMontage(Montage, PlayRate, StartSectionName);
	}
	else
	{
		return 0.0f;
	}
}

void ADBDCharacter::Client_SyncTransformAndMontage_Implementation(FTransform ServerSideTransform,
                                                                  UAnimMontage* Montage, float PlayRate,
                                                                  FName StartSectionName)
{
	SetActorTransform(ServerSideTransform);
	PlayAnimMontage(Montage, PlayRate, StartSectionName);
}

void ADBDCharacter::Client_SyncTransformByServer_Implementation(FTransform ServerCharacterTransform)
{
	SetActorTransform(ServerCharacterTransform);
}

void ADBDCharacter::Server_PlaySyncMontage_Implementation(UAnimMontage* Montage, float PlayRate, FName StartSectionName)
{
	PlayAnimMontage(Montage, PlayRate, StartSectionName);
	Multicast_PlaySyncMontage(Montage, PlayRate, StartSectionName);
}

void ADBDCharacter::Multicast_SetSurvivorRotation_Implementation(FRotator TargetRotation)
{
	SetActorRotation(TargetRotation);
}

void ADBDCharacter::Client_SetSurvivorRotation_Implementation(FRotator TargetRotation)
{
	SetActorRotation(TargetRotation);
}

void ADBDCharacter::Client_StopSyncMontage_Implementation(UAnimMontage* Montage)
{
	if (Montage)
	{
		StopAnimMontage(Montage);
	}
}

void ADBDCharacter::Multicast_StopSyncMontage_Implementation(UAnimMontage* Montage)
{
	if (Montage)
	{
		StopAnimMontage(Montage);
	}
}


void ADBDCharacter::Client_PlaySyncMontage_Implementation(UAnimMontage* Montage, float PlayRate, FName StartSectionName)
{
	if (Montage)
	{
		PlayAnimMontage(Montage, PlayRate, StartSectionName);
	}
}

void ADBDCharacter::Multicast_PlaySyncMontage_Implementation(UAnimMontage* Montage, float PlayRate,
                                                             FName StartSectionName)
{
	if (Montage)
	{
		PlayAnimMontage(Montage, PlayRate, StartSectionName);
	}
}

void ADBDCharacter::Multicast_SyncTransformAndMontage_Implementation(FTransform ServerSideTransform,
                                                                     UAnimMontage* Montage, float PlayRate,
                                                                     FName StartSectionName)
{
	SetActorTransform(ServerSideTransform);
	PlayAnimMontage(Montage, PlayRate, StartSectionName);
}

void ADBDCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		FTimerHandle UIInitTimerHandle;
		GetWorldTimerManager().SetTimer(UIInitTimerHandle, this, &ADBDCharacter::InitPerkOnClient, 5.0f, false);
	}
	// if (IsLocallyControlled() && Cast<AKillerCharacter>(this))
	// {
	// 	FTimerHandle EnableScratchMarkTimerHandle;
	// 	GetWorldTimerManager().SetTimer(EnableScratchMarkTimerHandle, this, &ADBDCharacter::EnableScratchMarkToCurrentClientSurvivor, 5.0f, false);
	//
	// }
	ADBDPlayerController* PC = Cast<ADBDPlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
	}
}

void ADBDCharacter::InitPerkOnClient()
{
	if (HasAuthority())
	{
		Client_UpdatePerk(Perk1, Perk2, Perk3, Perk4);
	}
}

void ADBDCharacter::EnableScratchMarkToCurrentClientSurvivor()
{
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (CharacterSubsystem)
	{
		CharacterSubsystem->EnableScratchMarkOnEverySurvivor();
	}
}

void ADBDCharacter::UpdateWarpResultOnServer()
{
	Server_UpdateLocationAndRotation(GetActorLocation(), GetActorRotation());
}

void ADBDCharacter::SyncLocationAndRotation()
{
	if (HasAuthority())
	{
		Multicast_UpdateLocationAndRotation(GetActorLocation(), GetActorRotation());
	}
	else
	{
		Server_UpdateLocationAndRotation(GetActorLocation(), GetActorRotation());
	}
}

void ADBDCharacter::Server_UpdateLocationAndRotation_Implementation(FVector NewLocation, FRotator NewRotation)
{
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
	Multicast_UpdateLocationAndRotation(NewLocation, NewRotation);
}


void ADBDCharacter::Multicast_UpdateLocationAndRotation_Implementation(FVector NewLocation, FRotator NewRotation)
{
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
}

void ADBDCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDCharacter, Perk1, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDCharacter, Perk2, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDCharacter, Perk3, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDCharacter, Perk4, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ADBDCharacter, StencilMaps, COND_None, REPNOTIFY_Always);
}

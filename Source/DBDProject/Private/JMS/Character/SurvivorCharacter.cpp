// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Character/SurvivorCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "JMS/DataAsset/DA_SurvivorInput.h"
#include "EnhancedInputSubsystems.h"
#include "MotionWarpingComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/AssetManager.h"
#include "JMS/Component/SkillCheckComponent.h"
#include "JMS/Component/SurvivorInteractableComponent.h"
#include "JMS/DataAsset/DA_SurvivorMontage.h"
#include "JMS/GAS/ItemAttributeSet.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "JMS/GAS/GE/GE_QuickAndQuiet.h"
#include "JMS/GAS/GE/GE_SurvivorResetDyingHP.h"
#include "JMS/GAS/GE/GE_SurvivorResetHealProgress.h"
#include "JMS/Item/SurvivorItem.h"
#include "JMS/ItemAddon/ItemAddonComponent.h"
#include "JMS/ScratchMark/PoolEntry_ScratchMark.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KMJ/Character/KillerCharacter.h"
#include "MMJ/Object/Interactable/Obj_ExitDoor.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "MMJ/Object/Widget/Obj_Progress.h"
#include "Net/UnrealNetwork.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Animation/DBDAnimInstance.h"
#include "Shared/Component/DBDMotionWarpingComponent.h"
#include "Shared/Component/InteractableComponent.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/Controller/DBDPlayerController.h"
#include "Shared/DataAsset/DBDDataBase.h"
#include "Shared/GameFramework/DBDGameInstance.h"
#include "Shared/GameFramework/DBDGameMode.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/ObjectPool/DBDObjectPool.h"
#include "Shared/ObjectPool/DBDObjectPoolComponent.h"
#include "Shared/ObjectPool/GenericObjectPool.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/Subsystem/DBDObjectObserver.h"
#include "Shared/UI/DBDHUD.h"
#include "Shared/UI/DBDWidgetComponent.h"
#include "Sound/SoundCue.h"

ASurvivorCharacter::ASurvivorCharacter(const FObjectInitializer& ObjectInitializer)
{
	// GAS
	SurvivorAbilitySystemComponent = CreateDefaultSubobject<USurvivorAbilitySystemComponent>(
		TEXT("SurvivorAbilitySystemComponent"));
	SurvivorAttributeSet = CreateDefaultSubobject<USurvivorAttributeSet>(TEXT("SurvivorAttributeSet"));

	// Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;

	FollowCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCam"));
	FollowCam->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);


	bUseControllerRotationYaw = false;

	// SkillCheckComponent
	SkillCheckComponent_BPCorruption = CreateDefaultSubobject<USkillCheckComponent>(TEXT("SkillCheckComponent"));


	// InteractableComponent
	SurvivorInteractableComponent = CreateDefaultSubobject<USurvivorInteractableComponent>(
		TEXT("SurvivorInteractableComponent"));
	SurvivorInteractableComponent->SetupAttachment(GetRootComponent());

	// Skin
	Skin = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skin"));
	Skin->SetupAttachment(GetMesh());

	// Scratch Mark
	ScratchMarkPool = CreateDefaultSubobject<UDBDObjectPoolComponent>(TEXT("ScratchMarkPool"));


	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.0f, 0.f);

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));


	BindGASChangeDelegate();
}

UAbilitySystemComponent* ASurvivorCharacter::GetAbilitySystemComponent() const
{
	return SurvivorAbilitySystemComponent;
}

APoolEntry_ScratchMark* ASurvivorCharacter::GetScratchMarkFromPool()
{
	return Cast<APoolEntry_ScratchMark>(ScratchMarkPool->SpawnPooledObject(this));
}

void ASurvivorCharacter::OnRep_CurrentHook(AObj_Hook* OldHook)
{
	// if (OldHook != CurrentHook && CurrentHook)
	// {
	// 	K2_DetachFromActor(EDetachmentRule::KeepWorld,
	// 	                   EDetachmentRule::KeepWorld,
	// 	                   EDetachmentRule::KeepWorld);
	// 	UDBDBlueprintFunctionLibrary::AddOrUpdateMotionWarpingTarget(CurrentHook->GetMotionWarpingTargets(),
	// 	                                                             MotionWarpingComponent);
	// 	PlayAnimMontage(HookInMontage);
	// }
}

void ASurvivorCharacter::RegisterHook(AObj_Hook* Hook)
{
	if (Hook)
	{
		CurrentHook = Hook;
		UDBDBlueprintFunctionLibrary::AddOrUpdateMotionWarpingTarget(
			IInteractable::Execute_GetMotionWarpingTargets(Cast<UObject>(CurrentHook)),
			DBDMotionWarpingComponent);
	}
}

AObj_Hook* ASurvivorCharacter::GetCurrentHook() const
{
	return CurrentHook;
}


void ASurvivorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!IsLocallyControlled())
	{
		return;
	}
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(InputData->IA_Look, ETriggerEvent::Triggered, this,
		                                   &ASurvivorCharacter::LookAction);
		EnhancedInputComponent->BindAction(InputData->IA_Move, ETriggerEvent::Triggered, this,
		                                   &ASurvivorCharacter::MoveAction);
		EnhancedInputComponent->BindAction(InputData->IA_UseItem, ETriggerEvent::Started, this,
		                                   &ASurvivorCharacter::StartUsingItem);
		EnhancedInputComponent->BindAction(InputData->IA_UseItem, ETriggerEvent::Completed, this,
		                                   &ASurvivorCharacter::EndUsingItem);

		for (const TPair<ESurvivorAbilityInputID, UInputAction*>& InputActionPair : InputData->
		     GameplayAbilityInputActions)
		{
			EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Triggered, this,
			                                   &ASurvivorCharacter::AbilityInput, InputActionPair.Key);
		}
		if (SkillCheckComponent_BPCorruption)
		{
			SkillCheckComponent_BPCorruption->SetupInputActionBinding(EnhancedInputComponent);
		}
	}
}

void ASurvivorCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	APlayerController* PC = GetController<APlayerController>();
	if (PC)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PC->GetLocalPlayer()->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->AddMappingContext(InputData->InputMappingContext,
			                                  InputData->InputMappingContextPriority);
		}
	}
}

void ASurvivorCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || IsLocallyControlledByPlayer())
	{
		USkeletalMesh* MergedMesh = USkeletalMergingLibrary::MergeMeshes(SkeletalMeshMergeParamsForSkin);
		if (MergedMesh)
		{
			Skin->SetSkeletalMeshAsset(MergedMesh);
			Skin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Skin->SetCollisionResponseToAllChannels(ECR_Ignore);
			Skin->SetLeaderPoseComponent(GetMesh());
		}
	}


	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11: CharacterSubsystem is NULL!"), 11);
	}
	else
	{
		if (ADBDPlayerState* DBDPS = GetWorld()->GetFirstPlayerController()->GetPlayerState<ADBDPlayerState>())
		{
			if (DBDPS->GetPlayerRole() == EPlayerRole::Killer)
			{
				CharacterSubsystem->EnableScratchMarkOnCurrentSurvivor(this);
			}
		}
	}

	if (IsLocallyControlledByPlayer())
	{
		GetWorldTimerManager().SetTimer(HeartBeatTimerHandle, this,
		                                &ASurvivorCharacter::PlayHeartBeatIfKillerNearby,
		                                0.5f, true);
	}
	// FTimerHandle InitUITimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(InitUITimerHandle, this, &ASurvivorCharacter::InitItemHUD, 5.f, false);
	// FTimerHandle InitItemTimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(InitItemTimerHandle, this,
	//                                        &ASurvivorCharacter::InitializeStartItemAfterWaitForReplicated, 3.f, false);

	if (ADBDPlayerState* PS = GetDBDPlayerState())
	{
		if (PS->GetPlayerEndState() != EPlayerEndState::None)
		{
			PlayAnimMontage(SurvivorMontageData->RunningOnExit);
		}
		else
		{
			PlayAnimMontage(SurvivorMontageData->StartMontage);
		}
	}
	FTimerHandle ReadyTimerHandle;
	GetWorldTimerManager().SetTimer(ReadyTimerHandle, this, &ASurvivorCharacter::SetReady, 3.f, false);

	ADBDPlayerController* PC = Cast<ADBDPlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
		if (ADBDPlayerState* PS = PC->GetPlayerState<ADBDPlayerState>())
		{
			if (PS->GetPlayerEndState() != EPlayerEndState::None)
			{
				DisableInput(PC);
				UE_LOG(LogTemp, Warning, TEXT("MMJLog : EndLevel"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MMJLog : InGameLevel"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MMJLog : Not Valid PS"));
		}
	}
}

void ASurvivorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ASurvivorCharacter::HealProgressChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	bool bFound1, bFound2 = false;
	float CurrentHealProgress = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
		USurvivorAttributeSet::GetHealProgressAttribute(), bFound1);
	float MaxHealProgress = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
		USurvivorAttributeSet::GetMaxHealProgressAttribute(), bFound2);
	if (bFound1 && bFound2)
	{
		if (CurrentHealProgress >= MaxHealProgress)
		{
			if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Dying))
			{
				SetSurvivorInjured();
			}
			else if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Injured))
			{
				SetSurvivorNormal();
			}
			SurvivorInteractableComponent->OnComplete.Broadcast();
		}
	}
}

void ASurvivorCharacter::DyingHPChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	// bool bFound1, bFound2 = false;
	// float CurrentDyingHP = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
	// 	USurvivorAttributeSet::GetDyingHPAttribute(), bFound1);
	// float MaxDyingHP = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
	// 	USurvivorAttributeSet::GetMaxDyingHPAttribute(), bFound2);
	// if (bFound1 && bFound2)
	// {
	// 	if (CurrentDyingHP >= MaxDyingHP)
	// 	{
	// 		SetSurvivorInjured();
	// 		SurvivorInteractableComponent->OnComplete.Broadcast();
	// 	}
	// }
}

void ASurvivorCharacter::MovementSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Sprinting))
	{
		return;
	}
	bool bFound = false;
	float MovementSpeed = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
		USurvivorAttributeSet::GetMovementSpeedAttribute(), bFound);
	if (bFound)
	{
		GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	}
}

void ASurvivorCharacter::SprintSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Sprinting))
	{
		bool bFound = false;
		float SprintSpeed = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
			USurvivorAttributeSet::GetSprintSpeedAttribute(), bFound);
		if (bFound)
		{
			GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		}
	}
}

void ASurvivorCharacter::BindGASChangeDelegate()
{
	if (SurvivorAbilitySystemComponent)
	{
		SurvivorAbilitySystemComponent->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Dead).AddUObject(
			this, &ASurvivorCharacter::DeathTagUpdate);
		SurvivorAbilitySystemComponent->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Escaped).AddUObject(
			this, &ASurvivorCharacter::EscapeTagUpdate);
		SurvivorAbilitySystemComponent->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_Sprinting).AddUObject(
			this, &ASurvivorCharacter::SprintTagUpdate);
		SurvivorAbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName(TEXT("Survivor.Status.Captured")))).AddUObject(
			this, &ASurvivorCharacter::CapturedTagUpdate);
		SurvivorAbilitySystemComponent->RegisterGameplayTagEvent(DBDGameplayTags::Survivor_Status_HookedPhase2).
		                                AddUObject(
			                                this, &ASurvivorCharacter::HookPhase2TagUpdate);
	}
	SurvivorAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		USurvivorAttributeSet::GetHealProgressAttribute()).AddUObject(
		this, &ASurvivorCharacter::HealProgressChanged);
	SurvivorAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		USurvivorAttributeSet::GetDyingHPAttribute()).AddUObject(
		this, &ASurvivorCharacter::DyingHPChanged);
	SurvivorAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		USurvivorAttributeSet::GetMovementSpeedAttribute()).AddUObject(
		this, &ASurvivorCharacter::MovementSpeedChanged);
	SurvivorAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		USurvivorAttributeSet::GetSprintSpeedAttribute()).AddUObject(
		this, &ASurvivorCharacter::SprintSpeedChanged);
}

void ASurvivorCharacter::DeathTagUpdate(const FGameplayTag Tag, int32 NewCount)
{
	// ADBDPlayerController* PC = GetController<ADBDPlayerController>();
	// if (PC)
	// {
	// 	PC->EnterSpectatorCam();
	// }
	HideItemMesh(true);
	float Duration = PlaySyncMontageFromServer(SurvivorMontageData->HookDead);
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASurvivorCharacter::AfterDeathMontage, Duration, false);
}

void ASurvivorCharacter::EscapeTagUpdate(const FGameplayTag Tag, int32 NewCount)
{
	// ADBDPlayerController* PC = GetController<ADBDPlayerController>();
	// if (PC)
	// {
	// 	PC->EnterSpectatorCam();
	// }
	SetCollisionAndGravityEnabled(false);
	SetActorHiddenInGame(true);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MovementMode = MOVE_None;
		GetCharacterMovement()->StopActiveMovement();
	}

	if (EquippedItem)
	{
		EquippedItem->SetActorHiddenInGame(true);
	}
	if (HasAuthority())
	{
		ADBDGameMode* DBDGM = Cast<ADBDGameMode>(UGameplayStatics::GetGameMode(this));
		if (DBDGM)
		{
			DBDGM->CheckGameCondition();
		}
	}
}

void ASurvivorCharacter::SprintTagUpdate(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		bool bFound = false;
		float SprintSpeed = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
			USurvivorAttributeSet::GetSprintSpeedAttribute(), bFound);
		if (bFound)
		{
			GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		}
	}
	else
	{
		bool bFound = false;
		float MovementSpeed = SurvivorAbilitySystemComponent->GetGameplayAttributeValue(
			USurvivorAttributeSet::GetMovementSpeedAttribute(), bFound);
		if (bFound)
		{
			GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
		}
	}
	SprintTagUpdateDelegate.Broadcast(this, NewCount);
}

void ASurvivorCharacter::CapturedTagUpdate(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SetCollisionAndGravityEnabled(false);
		AddUniqueTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	}
	else
	{
		SetCollisionAndGravityEnabled(true);
		RemoveTag(DBDGameplayTags::Survivor_Status_MoveDisabled);
	}
}

void ASurvivorCharacter::HookPhase2TagUpdate(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0 && !SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Dead))
	{
		PlaySyncMontageFromServer(SurvivorMontageData->HookPhase2Start);
	}
	else
	{
		if (SurvivorMontageData->HookPhase2Start == GetMesh()->GetAnimInstance()->GetCurrentActiveMontage())
		{
			StopSyncMontageFromServer(SurvivorMontageData->HookPhase2Start);
		}
	}
}

void ASurvivorCharacter::AfterDeathMontage()
{
	SetCollisionAndGravityEnabled(false);
	SetActorHiddenInGame(true);
	if (EquippedItem)
	{
		EquippedItem->SetActorHiddenInGame(true);
	}
	if (HasAuthority())
	{
		ADBDGameMode* DBDGM = Cast<ADBDGameMode>(UGameplayStatics::GetGameMode(this));
		if (DBDGM)
		{
			DBDGM->CheckGameCondition();
		}
	}
	MoveEnabled(false);
	bIsReadyToStart = false;
}

ADBDPlayerState* ASurvivorCharacter::GetDBDPlayerState() const
{
	return Cast<ADBDPlayerState>(GetPlayerState());
}

void ASurvivorCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ASurvivorCharacter, CurrentHook, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASurvivorCharacter, bIsMoveEnabled, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASurvivorCharacter, EquippedItem, COND_InitialOnly, REPNOTIFY_Always);
}


void ASurvivorCharacter::AbilityInput(const FInputActionValue& InputActionValue, ESurvivorAbilityInputID InputID)
{
	bool bPressed = InputActionValue.Get<bool>();
	if (bPressed)
	{
		SurvivorAbilitySystemComponent->AbilityLocalInputPressed((int32)InputID);
	}
	else
	{
		SurvivorAbilitySystemComponent->AbilityLocalInputReleased((int32)InputID);
	}
}


void ASurvivorCharacter::LookAction(const FInputActionValue& InputActionValue)
{
	FVector2D InputValue = InputActionValue.Get<FVector2D>();

	AddControllerPitchInput(-InputValue.Y);
	AddControllerYawInput(InputValue.X);
}

void ASurvivorCharacter::MoveAction(const FInputActionValue& InputActionValue)
{
	if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_MoveDisabled))
	{
		return;
	}
	if (!bIsMoveEnabled)
	{
		return;
	}
	FVector2D InputValue = InputActionValue.Get<FVector2D>();

	AddMovementInput(GetMoveForwardDirection() * InputValue.Y + GetLookRightDirection() * InputValue.X);
}

FVector ASurvivorCharacter::GetLookRightDirection() const
{
	return FollowCam->GetRightVector();
}

FVector ASurvivorCharacter::GetLookForwardDirection() const
{
	return FollowCam->GetForwardVector();
}

FVector ASurvivorCharacter::GetMoveForwardDirection() const
{
	return FVector::CrossProduct(GetLookRightDirection(), FVector::UpVector);
}

void ASurvivorCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController)
	{
		ServerSideInit();
	}
}

void ASurvivorCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ClientSideInit();
}

void ASurvivorCharacter::EquipItem(ASurvivorItem* ItemToEquip)
{
	UItemAddonComponent *Addon1 = nullptr, *Addon2 = nullptr;
	if (ItemToEquip)
	{
		ItemToEquip->SetOwner(this);
		ItemToEquip->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		                               ItemSocketName);
		ItemToEquip->OnEquipItem();
		EquippedItem = ItemToEquip;
		Addon1 = ItemToEquip->GetAddon1();
		Addon2 = ItemToEquip->GetAddon2();
	}
	Client_UpdateCurrentItem(EquippedItem, Addon1, Addon2);
}

void ASurvivorCharacter::DropItem(ASurvivorItem* ItemToPickUp)
{
	if (EquippedItem)
	{
		// SurvivorAttributeSet->OnCurrentItemChargeChanged.Unbind();

		EquippedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		EquippedItem->OnDropItem();
		EquippedItem->SetOwner(nullptr);
		if (!ItemToPickUp)
		{
			FVector Start = GetActorLocation();
			FVector End = Start - FVector(0, 0, 200.0f);

			FHitResult HitResult;
			FCollisionShape Shape = FCollisionShape::MakeSphere(10.0f); // 반���0짜리 구체��
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			bool bHit = GetWorld()->SweepSingleByChannel(
				HitResult,
				Start,
				End,
				FQuat::Identity,
				ECC_Visibility,
				Shape,
				Params
			);

			if (bHit)
			{
				FVector DropLocation = HitResult.Location;
				EquippedItem->SetActorLocation(DropLocation);
			}
			else
			{
				EquippedItem->SetActorLocation(Start);
			}
		}
		else
		{
			EquippedItem->SetActorLocation(ItemToPickUp->GetActorLocation());
			EquippedItem->SetActorRotation(ItemToPickUp->GetActorRotation());
		}
		EquippedItem = nullptr;
	}
	Client_UpdateCurrentItem(nullptr, nullptr, nullptr);
}

void ASurvivorCharacter::PickUpItem(ASurvivorItem* ItemToPickUp)
{
	if (!ItemToPickUp)
	{
		return;
	}
	FVector ExchangeLocation = ItemToPickUp->GetActorLocation();
	if (EquippedItem)
	{
		DropItem(ItemToPickUp);
	}
	EquipItem(ItemToPickUp);
}


void ASurvivorCharacter::StartUsingItem()
{
	if (EquippedItem)
	{
		// EquippedItem->OnStartUsingItem();
		Server_ApplyUsingEffectToItem();
	}
}

void ASurvivorCharacter::Server_ApplyUsingEffectToItem_Implementation()
{
	SurvivorAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GEforUseItem, 0,
	                                                             SurvivorAbilitySystemComponent->MakeEffectContext());
}

void ASurvivorCharacter::EndUsingItem()
{
	if (EquippedItem)
	{
		// EquippedItem->OnEndUsingItem();
		Server_RemoveUsingEffectFromItem();
	}
}

void ASurvivorCharacter::Server_RemoveUsingEffectFromItem_Implementation()
{
	SurvivorAbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(
		GEforUseItem, SurvivorAbilitySystemComponent);
}

void ASurvivorCharacter::Client_UpdateCurrentItem_Implementation(ASurvivorItem* NewItem, UItemAddonComponent* Addon1,
                                                                 UItemAddonComponent* Addon2)
{
	if (EquippedItem)
	{
		EquippedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (NewItem)
	{
		NewItem->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		                           ItemSocketName);
	}
	EquippedItem = NewItem;
	if (EquippedItem)
	{
		if (Addon1)
		{
			EquippedItem->AttachAddon1(Addon1);
		}
		if (Addon2)
		{
			EquippedItem->AttachAddon2(Addon2);
		}
	}
	//EquippedItemChangedDelegate.Broadcast(EquippedItem);
	// OnItemUIInitialize.Broadcast(this, GetInteractorRole());
	GetWidgetComponent()->OnUpdateEquippedItem.Broadcast(EquippedItem);
	if (EquippedItem)
		UE_LOG(LogTemp, Warning, TEXT("EquippedItem : %s"), *EquippedItem->GetName());
}


// void ASurvivorCharacter::InitItemHUD()
// {
// 	if (EquippedItem)
// 	{
// 		if (HasAuthority())
// 		{
// 			Client_UpdateCurrentItem(EquippedItem, EquippedItem->GetAddon1(), EquippedItem->GetAddon2());
// 		}
// 		else
// 			OnItemUIInitialize.Broadcast(this, GetInteractorRole());
// 	}
// }

ASurvivorItem* ASurvivorCharacter::GetEquippedItem() const
{
	return EquippedItem;
}

UInteractableComponent* ASurvivorCharacter::GetInteractableComponent() const
{
	return SurvivorInteractableComponent;
}

EPlayerRole ASurvivorCharacter::GetInteractorRole() const
{
	return EPlayerRole::Survivor;
}

USkillCheckComponent* ASurvivorCharacter::GetSkillCheckComponent() const
{
	return SkillCheckComponent_BPCorruption;
}

// void ASurvivorCharacter::InitializeStartItemAfterWaitForReplicated()
// {
// 	if (EquippedItem)
// 	{
// 		Client_UpdateCurrentItem(EquippedItem, EquippedItem->GetAddon1(), EquippedItem->GetAddon2());
// 	}
// }

void ASurvivorCharacter::OnRep_EquippedItem(ASurvivorItem* OldItem)
{
	if (OldItem != EquippedItem)
	{
		GetWidgetComponent()->OnUpdateEquippedItem.Broadcast(EquippedItem);
	}
}

void ASurvivorCharacter::MoveEnabled(bool bEnable)
{
	bIsMoveEnabled = bEnable;
	GetCharacterMovement()->SetMovementMode(bEnable ? MOVE_Walking : MOVE_None);
}

// MovementMode�None�나 Custom�로 �면 모션�핑�동�� �습�다.
void ASurvivorCharacter::SetCollisionAndGravityEnabled(bool bEnable)
{
	// GetCharacterMovement()->SetMovementMode(bEnable ? MOVE_Walking : MOVE_Swimming);
	GetCapsuleComponent()->SetCollisionEnabled(bEnable
		                                           ? ECollisionEnabled::QueryAndPhysics
		                                           : ECollisionEnabled::Type::QueryOnly);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, bEnable ? ECR_Block : ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, bEnable ? ECR_Block : ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, bEnable ? ECR_Block : ECR_Ignore);
	GetCharacterMovement()->GravityScale = bEnable ? 1.0f : 0.0f;
	GetMesh()->SetEnableGravity(bEnable);
	bIsCollisionAndGravityEnabled = bEnable;
	// SetActorEnableCollision(bEnable);
}

bool ASurvivorCharacter::IsCollisionAndGravityEnabled() const
{
	return bIsCollisionAndGravityEnabled;
}

bool ASurvivorCharacter::GetIsReadyToStart() const
{
	return bIsReadyToStart;
}

void ASurvivorCharacter::SetReady()
{
	bIsReadyToStart = true;
}

void ASurvivorCharacter::AttackSurvivor()
{
	FGameplayTagContainer AttackDisabledTagContainer;
	AttackDisabledTagContainer.AddTag(DBDGameplayTags::Survivor_Status_Dying);
	AttackDisabledTagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Survivor.Status.Captured")));
	FGameplayEventData EventData;
	if (SurvivorAbilitySystemComponent->HasAnyMatchingGameplayTags(AttackDisabledTagContainer))
	{
	}
	else if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(DBDGameplayTags::Survivor_Status_Injured))
	{
		SendGameplayTagEventToSelf(DBDGameplayTags::Survivor_Event_GetHit, EventData);
		SetSurvivorDying();
	}
	else
	{
		SendGameplayTagEventToSelf(DBDGameplayTags::Survivor_Event_GetHit, EventData);
		SetSurvivorInjured();
	}
}

void ASurvivorCharacter::SetSurvivorInjured()
{
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Normal);
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Injured);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Dying);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_CaptureEnabled);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Killer);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Hook);
	SurvivorAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(UGE_SurvivorResetHealProgress::StaticClass(), 0,
	                                                             SurvivorAbilitySystemComponent->
	                                                             MakeEffectContext());
}

void ASurvivorCharacter::SetSurvivorDying()
{
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Normal);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Injured);
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Dying);
	AddUniqueTag(DBDGameplayTags::Survivor_Status_CaptureEnabled);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Killer);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Hook);
	SurvivorAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(UGE_SurvivorResetHealProgress::StaticClass(), 0,
	                                                             SurvivorAbilitySystemComponent->
	                                                             MakeEffectContext());
}

void ASurvivorCharacter::SetSurvivorNormal()
{
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Normal);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Injured);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Dying);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_CaptureEnabled);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Killer);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Hook);
}

void ASurvivorCharacter::CaptureSurvivor()
{
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Normal);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Injured);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Dying);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_CaptureEnabled);
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Captured_Killer);
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Hook);
}

void ASurvivorCharacter::AuthHookSurvivor(AObj_Hook* Hook)
{
	if (HasAuthority())
	{
		RegisterHook(Hook);
		// PlayAnimMontage(HookInMontage);
		AddTag(DBDGameplayTags::Survivor_Status_HookCount);
		RemoveTagAll(DBDGameplayTags::Survivor_Status_CaptureEnabled);
		RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Killer);
		MoveIgnoreActorAdd(Hook);
		SetMoveIgnoreSurvivors(true);
		FTransform ResultTransform = UDBDBlueprintFunctionLibrary::MoveCharacterWithRotationAdjust(
			CurrentHook->GetSkeletalMeshComponent(),
			FName(TEXT("socket_SurvivorAttach")),
			this, FRotator(0.f, 120.f, 0.f));
		LookAtTargetActorLocal(Hook, 180);
		SetCollisionAndGravityEnabled(false);
		float HookInMontageDuration = SyncTransformAndPlayMontageFromServer(
			GetActorTransform(), SurvivorMontageData->HookIn);
		AddUniqueTag(DBDGameplayTags::Survivor_Status_Captured_Hook);
		FTimerHandle TimerHandle;
	}
}


void ASurvivorCharacter::ReleaseSurvivor()
{
	SetSurvivorInjured();
	RemoveTagAll(DBDGameplayTags::Survivor_Status_Captured_Hook);
	CurrentHook = nullptr;
}

void ASurvivorCharacter::SetSurvivorEscaped()
{
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Escaped);
}

void ASurvivorCharacter::SetSurvivorDead()
{
	AddUniqueTag(DBDGameplayTags::Survivor_Status_Dead);
}

void ASurvivorCharacter::SetSurvivorMoving(bool IsMoving)
{
	if (IsMoving)
	{
		AddUniqueTag(DBDGameplayTags::Survivor_Status_Moving);
	}
	else
	{
		RemoveTagAll(DBDGameplayTags::Survivor_Status_Moving);
	}
}


void ASurvivorCharacter::InitializeEquippedItem(FSurvivorItemInstanceInfo InitialItemInfo)
{
	UDBDGameInstance* DBDGI = Cast<UDBDGameInstance>(GetGameInstance());
	if (!DBDGI)
	{
		return;
	}

	UDataTable* DT = Cast<UDataTable>(
		UAssetManager::Get().GetStreamableManager().LoadSynchronous(
			DBDGI->DBDDB->SurvivorItemDB.ToSoftObjectPath()));

	ASurvivorItem* SpawnedItem = nullptr;

	if (DT)
	{
		FItemData* ItemData = DT->FindRow<FItemData>(InitialItemInfo.Item, "");
		if (ItemData)
		{
			TSubclassOf<ASurvivorItem> ItemClassFromDB = ItemData->ItemClass;
			if (ItemClassFromDB)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnedItem = GetWorld()->SpawnActor<ASurvivorItem>(ItemClassFromDB, SpawnParams);
			}
		}
	}
	else
	{
		//Debug::Print(TEXT("JMS111 : SurvivorItemDB Load Failed"), 111);
	}

	if (!SpawnedItem)
	{
		return;
	}

	DT = Cast<UDataTable>(
		UAssetManager::Get().GetStreamableManager().LoadSynchronous(
			DBDGI->DBDDB->ItemAddonDB.ToSoftObjectPath()));
	if (DT)
	{
		if (InitialItemInfo.Addon1 != NAME_None)
		{
			FItemAddonData* ItemAddonData = DT->FindRow<FItemAddonData>(InitialItemInfo.Addon1, "");
			if (ItemAddonData)
			{
				TSubclassOf<UItemAddonComponent> ItemAddonClassFromDB = ItemAddonData->AddonClass;
				if (ItemAddonClassFromDB)
				{
					UItemAddonComponent* ItemAddon = NewObject<UItemAddonComponent>(SpawnedItem, ItemAddonClassFromDB);
					SpawnedItem->AttachAddon1(ItemAddon);
					ItemAddon->OnInitialized();
				}
			}
		}
		if (InitialItemInfo.Addon2 != NAME_None)
		{
			FItemAddonData* ItemAddonData = DT->FindRow<FItemAddonData>(InitialItemInfo.Addon2, "");
			if (ItemAddonData)
			{
				TSubclassOf<UItemAddonComponent> ItemAddonClassFromDB = ItemAddonData->AddonClass;
				if (ItemAddonClassFromDB)
				{
					UItemAddonComponent* ItemAddon = NewObject<UItemAddonComponent>(SpawnedItem, ItemAddonClassFromDB);
					SpawnedItem->AttachAddon2(ItemAddon);
					ItemAddon->OnInitialized();
				}
			}
		}
	}
	else
	{
		Debug::Print(TEXT("JMS112 : SurvivorItemAddonDB Load Failed"), 112);
	}

	EquipItem(SpawnedItem);
}

void ASurvivorCharacter::ServerSideInit()
{
	SurvivorAbilitySystemComponent->InitAbilityActorInfo(this, this);
	SurvivorAbilitySystemComponent->ServerSideInit();
	SetSurvivorNormal();

	ADBDPlayerState* PS = Cast<ADBDPlayerState>(GetPlayerState());
	if (PS)
	{
		AuthInitPerks();
	}
	if (!IsRunningDedicatedServer())
	{
		UDBDAnimInstance* DBDAnimInstance = Cast<UDBDAnimInstance>(GetMesh()->GetAnimInstance());
		if (DBDAnimInstance)
		{
			DBDAnimInstance->InitializeWithAbilitySystem(SurvivorAbilitySystemComponent);
		}
	}
	ADBDPlayerState* DBDPlayerState = GetDBDPlayerState();
	if (!DBDPlayerState)
	{
		return;
	}
	FSurvivorItemInstanceInfo InitialItemInfo = DBDPlayerState->SurvivorLoadout.ItemInfo;
	if (InitialItemInfo.Item != NAME_None)
	{
		InitializeEquippedItem(InitialItemInfo);
	}
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		Debug::Print(TEXT("JMS11: CharacterSubsystem is NULL!"), 11);
	}
	else
	{
		CharacterSubsystem->RegisterSurvivor(this);
	}
}

void ASurvivorCharacter::ClientSideInit()
{
	SurvivorAbilitySystemComponent->InitAbilityActorInfo(this, this);
	UDBDAnimInstance* DBDAnimInstance = Cast<UDBDAnimInstance>(GetMesh()->GetAnimInstance());
	if (DBDAnimInstance)
	{
		DBDAnimInstance->InitializeWithAbilitySystem(SurvivorAbilitySystemComponent);
	}

	// TempPrototypeWidget = CreateWidget<UDBDHUD>(GetLocalViewingPlayerController(), TempPrototypeWidgetClass);
	// if (TempPrototypeWidget)
	// {
	// 	TempPrototypeWidget->AddToViewport();
	// }
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11: CharacterSubsystem is NULL!"), 11);
	}
	else
	{
		CharacterSubsystem->RegisterSurvivor(this);
	}
}

bool ASurvivorCharacter::IsLocallyControlledByPlayer() const
{
	// 로컬�어 & �레�어 컨트롤러�� �단
	return GetController() && GetController()->IsLocalPlayerController();
}

USkeletalMeshComponent* ASurvivorCharacter::GetSkin()
{
	return Skin;
}

void ASurvivorCharacter::AddUniqueTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		Server_AddUniqueTag(Tag);
	}
	else
	{
		if (!SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
		{
			SurvivorAbilitySystemComponent->AddLooseGameplayTag(Tag);
			SurvivorAbilitySystemComponent->AddReplicatedLooseGameplayTag(Tag);
		}
	}
}

void ASurvivorCharacter::AddTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		Server_AddTag(Tag);
	}
	else
	{
		SurvivorAbilitySystemComponent->AddLooseGameplayTag(Tag);
		SurvivorAbilitySystemComponent->AddReplicatedLooseGameplayTag(Tag);
	}
}

void ASurvivorCharacter::Server_AddTag_Implementation(const FGameplayTag& Tag)
{
	SurvivorAbilitySystemComponent->AddLooseGameplayTag(Tag);
	SurvivorAbilitySystemComponent->AddReplicatedLooseGameplayTag(Tag);
}

void ASurvivorCharacter::RemoveTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		Server_RemoveTag(Tag);
	}
	else
	{
		SurvivorAbilitySystemComponent->RemoveLooseGameplayTag(Tag);
		SurvivorAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(Tag);
	}
}

void ASurvivorCharacter::RemoveTagAll(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		Server_RemoveTagAll(Tag);
	}
	else
	{
		if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
		{
			SurvivorAbilitySystemComponent->RemoveLooseGameplayTags(FGameplayTagContainer(Tag),
			                                                        SurvivorAbilitySystemComponent->GetTagCount(Tag));
			SurvivorAbilitySystemComponent->RemoveReplicatedLooseGameplayTags(FGameplayTagContainer(Tag));
		}
	}
}

void ASurvivorCharacter::Server_RemoveTagAll_Implementation(const FGameplayTag& Tag)
{
	if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
	{
		SurvivorAbilitySystemComponent->RemoveLooseGameplayTags(FGameplayTagContainer(Tag),
		                                                        SurvivorAbilitySystemComponent->GetTagCount(Tag));
		SurvivorAbilitySystemComponent->RemoveReplicatedLooseGameplayTags(FGameplayTagContainer(Tag));
	}
}

void ASurvivorCharacter::PrintHasTag(FGameplayTag Tag)
{
	if (!HasAuthority())
	{
		Server_PrintHasTag(Tag);
	}
	else
	{
		if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
		{
			UE_LOG(LogTemp, Warning, TEXT("JMS : Tag exist"))
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JMS : Tag does not exist"))
		}
	}
}

void ASurvivorCharacter::SendGameplayTagEventToSelf(FGameplayTag Tag, FGameplayEventData EventData)
{
	if (!HasAuthority())
	{
		Server_SendGameplayTagEventToSelf(Tag, EventData);
	}
	else
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Tag, EventData);
	}
}

void ASurvivorCharacter::EnableAura(int32 StencilValue)
{
	Super::EnableAura(1);
	GetSkin()->SetRenderCustomDepth(true);
	GetSkin()->SetCustomDepthStencilValue(StencilValue);
}

void ASurvivorCharacter::DisableAura()
{
	Super::DisableAura();
	GetSkin()->SetRenderCustomDepth(false);
}

void ASurvivorCharacter::OnQuickAction()
{
	OnQuickActionPlayed.Broadcast();
}

void ASurvivorCharacter::OnSprintStarted()
{
	OnSprintStartedDelegate.Broadcast();
}

void ASurvivorCharacter::MoveToKiller(FName KillerSocket)
{
	if (UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		if (AKillerCharacter* Killer = CharacterSubsystem->GetKiller())
		{
			UDBDBlueprintFunctionLibrary::MoveDBDCharacterToMeshSocket(Killer->GetMesh(), KillerSocket, this);
		}
	}
}

void ASurvivorCharacter::HideItemMesh(bool bHide)
{
	if (EquippedItem)
	{
		EquippedItem->SetActorHiddenInGame(bHide);
	}
}

void ASurvivorCharacter::Anim_DisableMoveStart()
{
	MoveEnabled(false);
	Client_DisableMoveStart();
}

void ASurvivorCharacter::Anim_DisableMoveEnd()
{
	MoveEnabled(true);
	Client_DisableMoveEnd();
}


void ASurvivorCharacter::SetMoveIgnoreKiller(bool bIgnore)
{
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (CharacterSubsystem)
	{
		if (bIgnore)
		{
			MoveIgnoreActorAdd(CharacterSubsystem->GetKiller());
		}
		else
		{
			MoveIgnoreActorRemove(CharacterSubsystem->GetKiller());
		}
	}
}

void ASurvivorCharacter::SetMoveIgnoreSurvivors(bool bIgnore)
{
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (CharacterSubsystem)
	{
		for (ASurvivorCharacter* Survivor : CharacterSubsystem->GetSurvivors())
		{
			if (Survivor != this)
			{
				if (bIgnore)
				{
					MoveIgnoreActorAdd(Survivor);
				}
				else
				{
					MoveIgnoreActorRemove(Survivor);
				}
			}
		}
	}
}

void ASurvivorCharacter::OnRep_IsMoveEnabled(bool OldIsMoveEnabled)
{
	if (!bIsMoveEnabled)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void ASurvivorCharacter::PlayHeartBeatIfKillerNearby()
{
	if (UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>())
	{
		if (CharacterSubsystem->GetKiller())
		{
			if (CharacterSubsystem->GetKiller()->GetDistanceTo(this) < 1000.f)
			{
				UGameplayStatics::PlaySound2D(GetWorld(), HeartBeatSound, HeartBeatVolume);
				if (HeartBeatVolume < 1.f)
				{
					HeartBeatVolume += 0.25f;
				}
				else if (HeartBeatVolume > 1.f)
				{
					HeartBeatVolume = 1.f;
				}
			}
			else
			{
				if (HeartBeatVolume > 0.f)
				{
					HeartBeatVolume -= 0.25f;
				}
				else if (HeartBeatVolume < 0.f)
				{
					HeartBeatVolume = 0.f;
				}
			}
		}
	}
}


void ASurvivorCharacter::Client_DisableMoveEnd_Implementation()
{
	MoveEnabled(true);
}

void ASurvivorCharacter::Client_DisableMoveStart_Implementation()
{
	MoveEnabled(false);
}

void ASurvivorCharacter::Server_SendGameplayTagEventToSelf_Implementation(
	const FGameplayTag& Tag, FGameplayEventData EventData)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Tag, EventData);
}

void ASurvivorCharacter::Server_PrintHasTag_Implementation(const FGameplayTag& Tag)
{
	if (SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
	{
		UE_LOG(LogTemp, Warning, TEXT("JMS : Tag exist"))
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JMS : Tag does not exist"))
	}
}

void ASurvivorCharacter::Server_RemoveTag_Implementation(const FGameplayTag& Tag)
{
	SurvivorAbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	SurvivorAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(Tag);
}

void ASurvivorCharacter::Server_AddUniqueTag_Implementation(const FGameplayTag& Tag)
{
	if (!SurvivorAbilitySystemComponent->HasMatchingGameplayTag(Tag))
	{
		SurvivorAbilitySystemComponent->AddLooseGameplayTag(Tag);
		SurvivorAbilitySystemComponent->AddReplicatedLooseGameplayTag(Tag);
	}
}

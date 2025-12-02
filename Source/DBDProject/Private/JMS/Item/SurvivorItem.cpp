// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/SurvivorItem.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/Component/ItemInteractableComponent.h"
#include "JMS/GAS/ItemAbilitySystemComponent.h"
#include "JMS/GAS/ItemAttributeSet.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "JMS/ItemAddon/ItemAddonComponent.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/Component/InteractableComponent.h"

ASurvivorItem::ASurvivorItem()
{
	NetUpdateFrequency = 10.f;
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName("NoCollision");

	ItemInteractableComponent = CreateDefaultSubobject<UItemInteractableComponent>(TEXT("ItemInteractableComponent"));
	ItemInteractableComponent->SetupAttachment(Mesh);
	ItemInteractableComponent->SetCollisionProfileName("InteractionOnly");
	ItemInteractableComponent->SetBoxExtent(FVector(150.f, 150.f, 150.f));

	ItemAbilitySystemComponent = CreateDefaultSubobject<
		UItemAbilitySystemComponent>(TEXT("ItemAbilitySystemComponent"));
	ItemAttributeSet = CreateDefaultSubobject<UItemAttributeSet>(TEXT("ItemAttributeSet"));
}

UInteractableComponent* ASurvivorItem::GetInteractableComponent() const
{
	return ItemInteractableComponent;
}

UAbilitySystemComponent* ASurvivorItem::GetAbilitySystemComponent() const
{
	return ItemAbilitySystemComponent;
}

void ASurvivorItem::AttachAddon1(UItemAddonComponent* InAddon1)
{
	if (InAddon1)
	{
		Addon1 = InAddon1;
		Addon1->RegisterComponent();
	}
}

void ASurvivorItem::AttachAddon2(UItemAddonComponent* InAddon2)
{
	if (InAddon2)
	{
		Addon2 = InAddon2;
		Addon2->RegisterComponent();
	}
}

void ASurvivorItem::OnInitialized_Implementation()
{
}

void ASurvivorItem::OnEquipItem()
{
	// ItemAbilitySystemComponent->InitAbilityActorInfo(this, this);
	// ItemAbilitySystemComponent->SetNumericAttributeBase(UItemAttributeSet::GetMaxChargeAttribute(), MaxCharge);
	// ItemAbilitySystemComponent->SetNumericAttributeBase(UItemAttributeSet::GetCurrentChargeAttribute(),
	//                                                     CurrentCharge);
	GetOwnerSurvivorAbilitySystemComponent()->SetNumericAttributeBase(
		USurvivorAttributeSet::GetMaxItemChargeAttribute(), MaxCharge);
	GetOwnerSurvivorAbilitySystemComponent()->SetNumericAttributeBase(
		USurvivorAttributeSet::GetCurrentItemChargeAttribute(), CurrentCharge);
	ItemInteractableComponent->SetInteractable(false);
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (ASI)
	{
		UDBDBlueprintFunctionLibrary::AddUniqueTagToActor(GetOwner(), ItemTag);
	}
	if (Addon1)
	{
		Addon1->OnEquip();
	}
	if (Addon2)
	{
		Addon2->OnEquip();
	}
	for (TSubclassOf<UGameplayAbility> UseItemAbilityClass : UseItemAbilities)
	{
		GrantedSpecHandles.Add(
			GetOwnerSurvivorAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(UseItemAbilityClass, 0)));
	}
	OnInitialized();
}

void ASurvivorItem::OnStartUsingItem()
{
}

void ASurvivorItem::OnEndUsingItem()
{
	// for (TSubclassOf<UGameplayAbility> UseItemAbilityClass : UseItemAbilities)
	// {
	// 	ItemAbilitySystemComponent->CancelAbility(UseItemAbilityClass.GetDefaultObject());
	// }
}

void ASurvivorItem::OnDropItem()
{
	ItemInteractableComponent->SetInteractable(true);
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (ASI)
	{
		UDBDBlueprintFunctionLibrary::RemoveTagFromActor(GetOwner(), ItemTag);
	}
	if (Addon1)
	{
		Addon1->OnUnEquip();
	}
	if (Addon2)
	{
		Addon2->OnUnEquip();
	}

	MaxCharge = GetOwnerSurvivorAbilitySystemComponent()->GetNumericAttributeBase(
		USurvivorAttributeSet::GetMaxItemChargeAttribute());
	CurrentCharge = GetOwnerSurvivorAbilitySystemComponent()->GetNumericAttributeBase(
		USurvivorAttributeSet::GetCurrentItemChargeAttribute());

	for (FGameplayAbilitySpecHandle GrantedSpecHandle : GrantedSpecHandles)
	{
		GetOwnerSurvivorAbilitySystemComponent()->ClearAbility(GrantedSpecHandle);
	}
}

USkeletalMeshComponent* ASurvivorItem::GetMesh() const
{
	return Mesh;
}

FName ASurvivorItem::GetItemID() const
{
	return ItemID;
}

UItemAddonComponent* ASurvivorItem::GetAddon1() const
{
	return Addon1;
}

UItemAddonComponent* ASurvivorItem::GetAddon2() const
{
	return Addon2;
}

void ASurvivorItem::AddMaxCharge(float Amount)
{
	MaxCharge += Amount;
}

void ASurvivorItem::AddCurrentCharge(float Amount)
{
	CurrentCharge += Amount;
}

// 오류시 더 앞쪽으로 이동
void ASurvivorItem::BeginPlay()
{
	Super::BeginPlay();
	SetReplicates(true);
	SetReplicateMovement(true);
	// if (HasAuthority())
	// {
	// 	for (TSubclassOf<UGameplayAbility> UseItemAbilityClass : UseItemAbilities)
	// 	{
	// 		GrantedSpecHandles.Add(
	// 			ItemAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UseItemAbilityClass, 0)));
	// 	}
	// }
}

void ASurvivorItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// FString AbilityListString = "";
	// for (auto ActivatableAbility : GetAbilitySystemComponent()->GetActivatableAbilities())
	// {
	// 	AbilityListString += ActivatableAbility.GetDebugString();
	// 	AbilityListString += " ";
	// }
	// FString DebugString = FString::Printf(TEXT("Activatable Abilities: %s"), *AbilityListString);

	// for (FGameplayAbilitySpecHandle Handle : GrantedSpecHandles)
	// {
	// 	DebugString += Handle.ToString();
	// }
	// if (HasAuthority())
	// {
	// 	Debug::DebugStringWithNetMode(this, DebugString, GetActorLocation() + FVector(0, 0, 50), DeltaTime);
	// }
	// else
	// {
	// 	Debug::DebugStringWithNetMode(this, DebugString, GetActorLocation() + FVector(0, 0, 70), DeltaTime);
	// }
}

USurvivorAbilitySystemComponent* ASurvivorItem::GetOwnerSurvivorAbilitySystemComponent()
{
	ASurvivorCharacter* OwnerSurvivor = GetOwnerSurvivor();
	if (OwnerSurvivor)
	{
		return Cast<USurvivorAbilitySystemComponent>(OwnerSurvivor->GetAbilitySystemComponent());
	}
	return nullptr;
}

ASurvivorCharacter* ASurvivorItem::GetOwnerSurvivor()
{
	return Cast<ASurvivorCharacter>(GetOwner());
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/MedKit/Item_MedKit.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/GAS/ItemAbilitySystemComponent.h"
#include "Shared/DBDGameplayTags.h"
#include "JMS/GAS/AttributeSet/MedkitAttributeSet.h"


AItem_MedKit::AItem_MedKit()
{
	ItemTag = DBDGameplayTags::Survivor_Item_MedKit;
}

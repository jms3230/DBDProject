// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Item/Toolbox/Item_Toolbox.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/GAS/ItemAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAbilitySystemComponent.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "JMS/GAS/AttributeSet/ToolboxAttributeSet.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDGameplayTags.h"


AItem_Toolbox::AItem_Toolbox()
{
	ItemTag = DBDGameplayTags::Survivor_Item_Toolbox;
}

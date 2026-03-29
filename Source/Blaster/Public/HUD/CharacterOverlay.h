// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//BindWidget 是一个 meta 标记，用来把 C++ 变量和 UMG 蓝图中的同名控件自动绑定在一起
	UPROPERTY(meta = (BindWidget)) 
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock*HealthText;
};

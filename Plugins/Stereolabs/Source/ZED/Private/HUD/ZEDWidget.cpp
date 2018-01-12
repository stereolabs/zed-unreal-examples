//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "ZEDPrivatePCH.h"
#include "ZED/Public/HUD/ZEDWidget.h"
#include "Stereolabs/Public/Utilities/StereolabsFunctionLibrary.h"

UZEDWidget::UZEDWidget()
{
	// Unique name
	static int32 Count = 0;
	FString Name("WidgetComponent_");
	Name.AppendInt(++Count);

	WidgetComponent = CreateDefaultSubobject<UZEDWidgetComponent>(*Name);
	WidgetComponent->SetupAttachment(this);
}

void UZEDWidget::BeginDestroy()
{
	if (WidgetComponent->IsValidLowLevel())
	{
		delete WidgetComponent;
		WidgetComponent = nullptr;
	}

	Super::BeginDestroy();
}

void UZEDWidget::SetVisibility(bool bNewVisibility, bool bPropagateToChildren)
{
	Super::SetVisibility(bNewVisibility, bPropagateToChildren);

	WidgetComponent->SetVisibility(bNewVisibility, true);
}

EWidgetSpace UZEDWidget::GetWidgetSpace()
{
	return WidgetComponent->GetWidgetSpace();
}

void UZEDWidget::SetWidgetSpace(EWidgetSpace NewWidgetSpace)
{
	WidgetComponent->SetWidgetSpace(NewWidgetSpace);
}

void UZEDWidget::SetText(const FText& NewText)
{
	WidgetComponent->SetText(NewText);
}

void UZEDWidget::SetFontSize(int32 NewSize)
{
	WidgetComponent->SetFontSize(NewSize);
}

void UZEDWidget::SetTextColorAndOpacity(const FLinearColor& NewColor)
{
	WidgetComponent->SetTextColorAndOpacity(NewColor);
}

void UZEDWidget::FadeIn()
{
	WidgetComponent->FadeIn();
}

void UZEDWidget::FadeOut()
{
	WidgetComponent->FadeOut();
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreWidget.h"
#include "Components/TextBlock.h"

// This function updates the UI text
void UScoreWidget::UpdateScore(int NewScore)
{
    if (ScoreText) // Always check pointer, Unreal loves null crashes
    {
        FString ScoreString = FString::Printf(TEXT("Score: %d"), NewScore);
        ScoreText->SetText(FText::FromString(ScoreString));
    }
}

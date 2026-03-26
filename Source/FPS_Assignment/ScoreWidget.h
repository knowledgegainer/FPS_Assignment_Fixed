// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreWidget.generated.h"

// Forward declaration (clean code, avoids unnecessary includes)
class UTextBlock;

UCLASS()
class FPS_ASSIGNMENT_API UScoreWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // This will be linked to a TextBlock in Blueprint
    // Name MUST match in Blueprint
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreText;

    // Function to update score from C++
    void UpdateScore(int NewScore);
};

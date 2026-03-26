// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoxEnemy.generated.h"

UCLASS()
class FPS_ASSIGNMENT_API ABoxEnemy : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoxEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Material instance to control color
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Health = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Score = 10;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;


	// Initialize values from JSON
	void InitializeBox(int InHealth, int InScore, FLinearColor InColor);

	void TakeDamage();

};

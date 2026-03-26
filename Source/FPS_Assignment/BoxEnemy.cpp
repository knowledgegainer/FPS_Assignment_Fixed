// Fill out your copyright notice in the Description page of Project Settings.


#include "BoxEnemy.h"
#include "Engine/Engine.h"
#include "ShooterCharacter.h"

#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
ABoxEnemy::ABoxEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

}

// Called when the game starts or when spawned
void ABoxEnemy::BeginPlay()
{
	Super::BeginPlay();

	
	
	
}

// Called every frame
void ABoxEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABoxEnemy::TakeDamage()
{
    Health--;

    if (Health <= 0)
    {
        // === GET PLAYER ===
        APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();

        if (PlayerPawn)
        {
            AShooterCharacter* Player = Cast<AShooterCharacter>(PlayerPawn);

            if (Player)
            {
                Player->AddScore(Score);
            }
        }

        Destroy();
    }
}

void ABoxEnemy::InitializeBox(int InHealth, int InScore, FLinearColor InColor)
{
    Health = InHealth;
    Score = InScore;

    // Apply color using dynamic material
    if (Mesh)
    {
        UMaterialInstanceDynamic* DynMat = Mesh->CreateAndSetMaterialInstanceDynamic(0);

        if (DynMat)
        {
            DynMat->SetVectorParameterValue("Color", InColor);
        }
    }
}


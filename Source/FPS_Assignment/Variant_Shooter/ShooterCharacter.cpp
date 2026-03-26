// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"

#include "ScoreWidget.h"
#include "Blueprint/UserWidget.h"
#include "BoxEnemy.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	
	// CREATE AND SHOW UI
	if (ScoreWidgetClass)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Widget Class Found"));

		ScoreWidgetInstance = CreateWidget<UScoreWidget>(GetWorld(), ScoreWidgetClass);

		if (ScoreWidgetInstance)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Widget Created"));

			ScoreWidgetInstance->AddToViewport();
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Widget Class NULL"));
	}

	FetchJSONData();

	// Spawns test box
	//SpawnTestBoxes();
	
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1)
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}


//SCORE FUNCTION
void AShooterCharacter::AddScore(int Amount)
{
	PlayerScore += Amount;

	// Debug message to verify it works
	FString Msg = FString::Printf(TEXT("Score: %d"), PlayerScore);
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);

	// Update UI
	if (ScoreWidgetInstance)
	{
		ScoreWidgetInstance->UpdateScore(PlayerScore);
	}
}


void AShooterCharacter::SpawnTestBoxes()
{
	if (!BoxEnemyClass) return;

	FVector StartLocation = GetActorLocation() + FVector(500, 0, 100);

	for (int i = 0; i < 3; i++)
	{
		FVector Offset = FVector(i * 200, 0, 0);

		ABoxEnemy* Box = GetWorld()->SpawnActor<ABoxEnemy>(
			BoxEnemyClass,
			StartLocation + Offset,
			FRotator::ZeroRotator
		);

		
		if (Box)
		{
			// Temporary values 
			FLinearColor Colors[3] = {
				FLinearColor::Red,
				FLinearColor::Green,
				FLinearColor::Blue
			};

			Box->InitializeBox(3, 10, Colors[i]);
		}
	}
}

void AShooterCharacter::FetchJSONData()
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindUObject(this, &AShooterCharacter::OnJSONResponseReceived);

	Request->SetURL("https://raw.githubusercontent.com/CyrusCHAU/Varadise-Technical-Test/refs/heads/main/data.json");
	Request->SetVerb("GET");

	Request->ProcessRequest();
}


void AShooterCharacter::OnJSONResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{

	if (!bWasSuccessful || !Response.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("HTTP Failed"));
		return;
	}

	FString JSONString = Response->GetContentAsString();

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("JSON Received"));

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("JSON Parse Failed"));
		return;
	}

	// READ TYPES 
	TMap<FString, FLinearColor> TypeColors;
	TMap<FString, int> TypeHealth;
	TMap<FString, int> TypeScore;

	const TArray<TSharedPtr<FJsonValue>>* TypesArray;


	if (JsonObject->TryGetArrayField("types", TypesArray))
	{
		for (auto& TypeValue : *TypesArray)
		{
			TSharedPtr<FJsonObject> TypeObj = TypeValue->AsObject();

			FString Name = TypeObj->GetStringField("name");
			int Health = TypeObj->GetIntegerField("health");
			int Score = TypeObj->GetIntegerField("score");

			const TArray<TSharedPtr<FJsonValue>>* ColorArray;
			if (TypeObj->TryGetArrayField("color", ColorArray))
			{
				float R = (*ColorArray)[0]->AsNumber() / 255.0f;
				float G = (*ColorArray)[1]->AsNumber() / 255.0f;
				float B = (*ColorArray)[2]->AsNumber() / 255.0f;

				TypeColors.Add(Name, FLinearColor(R, G, B));
				TypeHealth.Add(Name, Health);
				TypeScore.Add(Name, Score);
			}
		}
	}

	// READ OBJECTS 
	const TArray<TSharedPtr<FJsonValue>>* ObjectsArray;


	if (JsonObject->TryGetArrayField("objects", ObjectsArray))
	{

		for (auto& ObjValue : *ObjectsArray)
		{   
		
			TSharedPtr<FJsonObject> Obj = ObjValue->AsObject();

			FString TypeName = Obj->GetStringField("type");

			TSharedPtr<FJsonObject> TransformObj = Obj->GetObjectField("transform");

			//rotation
			const TArray<TSharedPtr<FJsonValue>>* RotationArray;

			FRotator Rotation = FRotator::ZeroRotator;

			if (TransformObj->TryGetArrayField("rotation", RotationArray))
			{
				Rotation = FRotator(
					(*RotationArray)[0]->AsNumber(),
					(*RotationArray)[1]->AsNumber(),
					(*RotationArray)[2]->AsNumber()
				);

			}
			
			// scale
			const TArray<TSharedPtr<FJsonValue>>* ScaleArray;

			FVector Scale = FVector(1, 1, 1);

			if (TransformObj->TryGetArrayField("scale", ScaleArray))
			{
				Scale = FVector(
					(*ScaleArray)[0]->AsNumber(),
					(*ScaleArray)[1]->AsNumber(),
					(*ScaleArray)[2]->AsNumber()
				);

				
			}

			//location
			const TArray<TSharedPtr<FJsonValue>>* LocationArray;


			if (TransformObj->TryGetArrayField("location", LocationArray))
			{
				
				FVector Location(
					(*LocationArray)[0]->AsNumber(),
					(*LocationArray)[1]->AsNumber(),
					(*LocationArray)[2]->AsNumber()
				);
				
				
				ABoxEnemy* Box = GetWorld()->SpawnActor<ABoxEnemy>(
					BoxEnemyClass,
					Location,
					Rotation
				);

				
				if (Box)
				{
					Box->SetActorScale3D(Scale);

					if (TypeHealth.Contains(TypeName) &&
						TypeScore.Contains(TypeName) &&
						TypeColors.Contains(TypeName))
					{
						Box->InitializeBox(
							TypeHealth[TypeName],
							TypeScore[TypeName],
							TypeColors[TypeName]
						);
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("TYPE NOT FOUND"));
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SPAWN FAILED"));
				}
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("objects not found"));
	}
}




void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JargonCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UJargonHeroDefinition;

/**
 *  A controllable top-down perspective character
 */
UCLASS(Blueprintable)
class AJargonCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

public:

	/** Constructor */
	AJargonCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Hero")
	virtual void InitializeFromHeroDefinition(UJargonHeroDefinition* HeroDefinition);

	UFUNCTION(BlueprintPure, Category = "Hero")
	UJargonHeroDefinition* GetAppliedHeroDefinition() const
	{
		return AppliedHeroDefinition;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Hero")
	void BP_OnHeroDefinitionApplied(UJargonHeroDefinition* HeroDefinition);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Hero")
	TObjectPtr<UJargonHeroDefinition> AppliedHeroDefinition = nullptr;

};


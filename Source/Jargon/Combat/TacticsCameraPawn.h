// TacticsCameraPawn.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TacticsCameraPawn.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class JARGON_API ATacticsCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ATacticsCameraPawn();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;


	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;
	
	UPROPERTY(EditDefaultsOnly, Category = "Presentation")
	float SpringArmLength = 1500;

	void SetSpringArmLength(float Length) const;

	virtual void BeginPlay() override;
};
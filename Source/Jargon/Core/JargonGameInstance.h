// JargonGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Encounters/EncounterTypes.h"
#include "Engine/GameInstance.h"
#include "JargonGameInstance.generated.h"

UCLASS()
class JARGON_API UJargonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UJargonGameInstance();

	/** Legacy milestone-1 path. Keeps current encounter trigger flow working while we refactor forward. */
	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void StartEncounter(const FName& InEncounterId, const FName& InSourceMapName, const FTransform& InSourceTransform);

	/** Milestone-2 path. Stores full runtime encounter data before loading the combat map. */
	void StartEncounterWithRuntimeData(
		const FPendingEncounterRuntimeData& InPendingEncounter,
		const FName& InSourceMapName,
		const FTransform& InSourceTransform
	);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void MarkEncounterCleared(const FName& EncounterId);

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool IsEncounterCleared(const FName& EncounterId) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void PrepareReturnToExploration();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void CompleteReturnToExploration();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void ClearPendingEncounter();

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool IsReturningFromCombat() const
	{
		return bReturningFromCombat;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FName& GetReturnMapName() const
	{
		return ReturnMapName;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FTransform& GetReturnTransform() const
	{
		return ReturnTransform;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FName& GetPendingEncounterId() const
	{
		return PendingEncounterData.EncounterId;
	}

	const FPendingEncounterRuntimeData& GetPendingEncounterData() const
	{
		return PendingEncounterData;
	}

	bool HasPendingEncounterData() const
	{
		return PendingEncounterData.HasConfiguredCombatEncounter();
	}

protected:
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FName ReturnMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FTransform ReturnTransform = FTransform::Identity;

	/** Runtime encounter data passed from the exploration world into the combat world. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FPendingEncounterRuntimeData PendingEncounterData;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	bool bReturningFromCombat = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	TSet<FName> ClearedEncounterIds;
};
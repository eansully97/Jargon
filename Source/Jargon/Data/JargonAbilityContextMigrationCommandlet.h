#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "JargonAbilityContextMigrationCommandlet.generated.h"

UCLASS()
class JARGON_API UJargonAbilityContextMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UJargonAbilityContextMigrationCommandlet();

	/** Editor commandlet entry point for ability hook-context migration/audit work. Intended for explicit offline runs. */
	virtual int32 Main(const FString& Params) override;
};

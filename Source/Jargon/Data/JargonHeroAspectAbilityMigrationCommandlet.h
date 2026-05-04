#pragma once

#include "Commandlets/Commandlet.h"
#include "JargonHeroAspectAbilityMigrationCommandlet.generated.h"

/**
 * Editor-only content migration commandlet that converts Mage hero aspect raw effect hooks
 * into reusable UJargonAbilityDefinition assets.
 */
UCLASS()
class JARGON_API UJargonHeroAspectAbilityMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UJargonHeroAspectAbilityMigrationCommandlet();

	virtual int32 Main(const FString& Params) override;
};

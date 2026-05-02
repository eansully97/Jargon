#pragma once

#include "Commandlets/Commandlet.h"
#include "JargonRuntimeShellAssignmentCommandlet.generated.h"

/**
 * Editor-only cleanup commandlet for assigning CardScript runtime shell Blueprint classes
 * after Data Asset migration passes.
 */
UCLASS()
class JARGON_API UJargonRuntimeShellAssignmentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UJargonRuntimeShellAssignmentCommandlet();

	virtual int32 Main(const FString& Params) override;
};

# Combat Post-Match Return Flow

Combat completion is gated by the combat-side post-match report:

`Victory/Defeat -> PostMatchReportWidget in combat -> Continue -> saved exploration map and transform`

## Runtime Flow

- Exploration encounter entry stores the source map and player transform on `UJargonGameInstance`.
- Combat victory/defeat stores the post-combat report and marks the run as returning to exploration.
- `AJargonCombatPlayerController` shows the assigned `PostMatchReportWidgetClass` when combat phase becomes `Victory` or `Defeat`.
- `UPostMatchReportWidget::RequestContinue()` broadcasts to the combat controller.
- Continue clears the pending report and asks `AJargonCombatGameMode` to return to the saved exploration map.
- `AJargonExplorationGameMode` restores the player pawn to the saved transform after the exploration map loads.

## Blueprint Setup

- Assign a Blueprint child of `UPostMatchReportWidget` to `PostMatchReportWidgetClass` on the combat controller.
- Name the continue button `ContinueButton` to let C++ bind it automatically, or call `RequestContinue()` manually from the widget Blueprint.
- Town no longer auto-opens pending post-combat reports. The report belongs to the combat screen.

## Notes

- Victory and defeat both use this same combat-side report gate.
- Direct combat maps need a valid return map/transform in the game instance to travel after Continue; otherwise C++ logs a clear warning.
- Widget Blueprint owns presentation only. C++ owns result timing, report data, and travel.

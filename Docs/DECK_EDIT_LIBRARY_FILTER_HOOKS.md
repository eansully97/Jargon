# Deck Edit Library Filter Hooks

## Summary

`UDeckEditWidget` now exposes a source-only filter layer for the deck library. Blueprint UI can filter the library by card element, card type, ownership, and addability without changing the existing library entry widgets.

## Blueprint Setup

- Use `GetLibraryEntriesForCurrentPage()` exactly as before when spawning or refreshing visible library cards.
- Use `GetFilteredLibraryCardCount()` and `GetTotalLibraryCardCount()` for result counts.
- Use `GetLibraryPageText()` for filtered paging text.
- Use `GetLibraryFilterSummaryText()` for a compact active-filter label.
- Implement `BP_OnLibraryFilterChanged` to refresh filter labels or selected button state.
- Existing `BP_OnLibraryPageChanged` still fires when the visible page should be redrawn.

Suggested controls:

- Element dropdown/buttons call `SetLibraryElementFilter(true, Element)` for one selected element.
- Element checkboxes call `SetLibraryElementFilterEnabled(Element, bChecked)`. Multiple checked elements are treated as an OR filter, so Fire plus Radiance shows Fire or Radiance cards.
- A checkbox group can also build an array and call `SetLibraryElementFilters(Elements)`.
- An "All Elements" button calls `SetLibraryElementFilter(false, EJargonElementType::None)`.
- Type dropdown/buttons call `SetLibraryCategoryFilter(true, Category)` for one selected card type.
- Type checkboxes call `SetLibraryCategoryFilterEnabled(Category, bChecked)`. Multiple checked types are treated as an OR filter.
- A checkbox group can also build an array and call `SetLibraryCategoryFilters(Categories)`.
- An "All Types" button calls `SetLibraryCategoryFilter(false, ECardCategory::Spell)`.
- Owned toggle calls `SetShowOnlyOwned`.
- Addable toggle calls `SetShowOnlyAddable`.
- Clear button calls `ClearLibraryFilter`.

## Behavior

- `LibraryEntries` remains the full unfiltered catalog.
- `FilteredLibraryEntries` is the active filtered catalog.
- `CurrentLibraryPageEntries` pages over `FilteredLibraryEntries`.
- Filters reset the current page to page 0.
- Element filter disabled means all elements.
- Element filter enabled with `EJargonElementType::None` means Neutral.
- Multiple selected elements match any selected element.
- Category filter disabled means all card types.
- Multiple selected categories match any selected card type.
- Owned-only requires `OwnedCount > 0`.
- Addable-only requires `bCanAddToDeck`.

## Notes

- This pass does not add text search.
- Blueprint owns layout, dropdown contents, selected-state styling, and button behavior.
- Existing hover hooks continue to work because visible entries are still `FDeckEditLibraryCardEntry` values.

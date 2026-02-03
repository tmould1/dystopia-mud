# Tech Debt

Known technical debt items to address in future development.

## Dirgesinger Class

- **Self class announcements**: The Dirgesinger class needs self class announcements like other base classes. When a player selects Dirgesinger via `selfclass` or an immortal uses `class` to set it, the announcement messaging should be consistent with other base classes. See `wizutil.c:do_classself()` and `clan.c:do_class()` for the existing patterns.

# Tech Debt

Known technical debt items to address in future development.

## Dirgesinger Class

- ~~**Self class announcements**: The Dirgesinger class needs self class announcements like other base classes.~~ **RESOLVED**: Added `dirgetalk` class communication channel (CHANNEL_DIRGETALK) allowing Dirgesingers and Sirens to communicate with each other, matching the pattern of other base classes (vamptalk, magetalk, etc.). See commit 70afb54.

6) Update implementing-a-new-class.md with instructions to extend do_mudstats MVC for new classes.
7) Review new class guide for areas of improvement for iteration.  Can some things be simplified or refactored to support a more data-driven approach with less code modifications?
8) UX pass on boards and notes.  At least MXP tags to help navigate from "do_board" to switch and note list on that channel; then mxp "note list" results so clicking on the note line runs "note read x" for them.  Currently, its cumbersome and new users bounce right off of it.
9) Quest system to drive players towards game's goals that are not signposted well. Over-arching path: "Obtain max upgrade level"; with graph-based sub-quests (trees) that drive the player from brand-new to seasoned player"
10) "Config +Automap" show map results for 3-4 radius when moving to a new room.  Standardize output for client plugins to easily detect with regex. research gmcp for standard supported map channels too.
11) Class gen-names and open/close "brackets" should be in game db somewhere. Viewable in the python gui editor, with ansi previews.
12) Profiling and display system for server performance.
13) Run through todo section design documents

---

## Completed / Design Docs Created

- [x] Database backup to S3 - see [db-backup-s3.md](db-backup-s3.md)
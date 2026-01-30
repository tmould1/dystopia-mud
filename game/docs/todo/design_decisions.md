# Design Decisions

Open questions and potential changes to discuss.

## Kingdoms: Voluntary Leave

**System:** [Kingdom System](../design/kingdoms/overview.md)

Players currently have no way to voluntarily leave a kingdom. The only paths out are:

- Being outcasted by a leader or general (`koutcast`)
- Being reassigned by an immortal (`kingset`)

This means a member whose leadership has gone inactive is permanently stuck.

**Options:**

1. Add a `kleave` command that lets any member remove themselves (set `kingdom` to 0)
2. Add `kleave` with a cooldown or QPS penalty to discourage kingdom-hopping
3. Keep current behavior — leadership controls membership entirely

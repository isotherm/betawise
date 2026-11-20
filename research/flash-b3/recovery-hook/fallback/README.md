# Incomplete NEO13 manual-updater reachability probes

These fixtures are exploratory control-flow observations, **not successful USB
repair tests**. See [the report](../../usb-repair-and-width.md) for conclusions.
Raw logs include failed routes; FALLBACK_COMPLETE means the observation interval
ended, not that recovery or USB succeeded. No screen claims or flash commands.

The recorded Shift+Tab attempt selected the normal handoff. The service-chord
attempt reached the vendor password routine with an invalid normal handoff,
but neither password attempt established arrival at updater/USB setup.

The current runner takes a new output path and a disposable stock NEO13 fixture
prepared by the parent hook runner. Optional trailing case names select cases.
The service chord and password are sent as native keyboard inputs; the runner
does not force a CPU entry point, change password checks or fabricate USB state.

```sh
python3 research/flash-b3/recovery-hook/fallback/run.py /tmp/fallback-new /tmp/hooks/neo13small-stock
```

Captured earlier iterations used different input timing/chords; the labels in
results identify them. Current code uses the service chord and longer intervals.

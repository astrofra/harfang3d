# Tau Patch Log

Date: 2026-08-31

This file records Harfang-local changes applied to the imported Tau source
snapshot.

Current status:

- no functional Tau source patches applied yet;
- build integration scaffold added around the imported source tree;
- Harfang transform adapter added under `compat/` to avoid importing the legacy
  scenegraph around Tau;
- future patches should be logged here with:
  - date,
  - affected files,
  - reason,
  - and whether the change is safety-critical or adapter-only.

Planned first patch set:

- collision buffer bounds checks;
- symmetric collision mask evaluation;
- cuboid-only compile subset selection;
- compatibility include redirection;
- removal or disabling of serialization and debug-only legacy paths for the
  first compile spike.

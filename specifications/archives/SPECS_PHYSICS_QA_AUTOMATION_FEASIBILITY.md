# Bullet-Referenced Physics QA Automation Feasibility For Harfang

Date: 2026-08-31

Method: static local source review plus local smoke validation of the
vendorized `harfang3d/physics-qa` Lua suite. I did not implement the
automation layer, and I did not run cross-backend replay comparisons.

## Executive Summary

Automating physics QA in Harfang, so a future backend can be validated against
the current Bullet behavior, is feasible and worth doing.

The repository already contains most of the raw ingredients:

- a vendorized Lua physics QA suite under `harfang3d/physics-qa`,
- a small existing automated Bullet physics test subset in
  `harfang/tests/engine/scene.cpp`,
- runtime capture APIs that can support optional image regression,
- and an existing regression-runner pattern in
  `languages/launcher_tests/run_launcher_regression.py`.

The missing piece is not content. It is a deterministic automation layer.

Bottom line:

- Bullet can be used as a compatibility baseline, not as absolute physical
  truth.
- Full automation is feasible with medium integration cost.
- The right design is a layered QA system:
  - backend-neutral invariant tests,
  - Bullet baseline trace tests,
  - optional image regression,
  - performance benchmarks.
- It is not realistic to automate the current Lua suite as-is without adding a
  small runner contract and structured outputs.

## Feasibility Verdict

Feasible with medium implementation cost and good long-term value.

Recommended as a staged effort.

Not recommended as:

- a pure screenshot-comparison project,
- a bit-exact replay system across backends,
- or a direct attempt to batch-run every current interactive Lua scenario
  unchanged.

## Why This Is Worth Doing

If Harfang ever tries to replace Bullet, the main risk is not only hard
crashes. The bigger risk is silent semantic drift:

- collision events still happen, but at different frames;
- raycasts still hit, but return different nodes or hit positions;
- kinematic bodies mostly work, but diverge around moving contacts;
- mesh colliders still load, but behave differently at edges, corners, or thin
  surfaces;
- stacked bodies remain "stable enough" visually while gameplay logic regresses.

Manual visual QA does not scale well enough for that problem.

An automated physics QA layer would give Harfang:

- a repeatable compatibility baseline,
- a faster go/no-go loop for candidate backends,
- benchmark-ready scenarios,
- and a durable regression suite for future engine work even if Bullet remains
  the shipping backend.

## Bullet As Ground Truth: Acceptable, But Only In A Narrow Sense

Using Bullet as "ground truth" is acceptable only if the term is interpreted
carefully.

Bullet is not physical truth.

Bullet is:

- Harfang's current physics behavior,
- the compatibility target for existing content,
- and the most practical baseline for regression detection.

That distinction matters because a replacement backend should not be forced to
inherit every accidental Bullet quirk forever.

So the QA strategy should split tests into three categories:

### 1. Backend-Neutral Invariants

These are statements that should remain true regardless of engine choice.

Examples:

- a dynamic body falls under gravity;
- a kinematic body does not freefall;
- a raycast that should miss returns no hit;
- a static collider does not drift;
- scene sync does not teleport bodies unexpectedly.

### 2. Bullet Compatibility Baselines

These are checks where Harfang explicitly wants to preserve current Bullet
behavior closely enough for content compatibility.

Examples:

- collision-event presence and approximate timing,
- hit node identity for selected raycasts,
- mesh-collider raycast results on known scenes,
- rough bounce/friction envelopes on reference setups.

### 3. Performance And Stability Envelopes

These are not "truth" tests. They answer:

- is the candidate backend faster,
- is it stable enough,
- and does it degrade under stress scenes.

That keeps the suite honest. It avoids confusing "same as Bullet" with
"correct".

## Current Starting Point In The Repository

The current repo is already in a good position to support this work.

### Existing QA Content

`harfang3d/physics-qa` is now vendorized specifically as a future physics QA
and benchmarking suite.

That content is valuable because it already covers real Harfang physics usage:

- rigid bodies,
- collision events,
- raycasts,
- kinematic cases,
- and mesh-collider scenarios.

### Existing Automated Engine Tests

`harfang/tests/engine/scene.cpp` already contains a minimal automated Bullet
subset:

- dynamic freefall,
- kinematic no freefall,
- dynamic-vs-static collision callback presence,
- `NodeCollideWorld`,
- `RaycastFirstHit`,
- `RaycastAllHits`,
- plus no-hit variants for raycasts.

This is important because it proves Harfang already has one workable pattern
for deterministic physics assertions.

### Existing Runtime Capture Support

The runtime exposes `CaptureTexture`, `SavePNG`, `TF_ReadBack`, and
`TF_BlitDestination`, and `tutorials/scene_capture_texture.lua` shows how to
use them.

That means optional offscreen visual regression is technically available
without inventing a new capture stack first.

### Existing Regression Runner Pattern

`languages/launcher_tests/run_launcher_regression.py` already demonstrates a
simple fixture preparation and pass/fail runner pattern based on process exit
code and expected output.

A physics QA runner can reuse the same general shape.

## What Prevents Direct Automation Today

The current Lua QA suite is not yet a pass/fail harness.

Most scenarios are still shaped for human inspection:

- they open a window,
- they loop until `Escape`,
- they use live keyboard or mouse input,
- they print descriptions instead of structured results,
- and they target `SceneBullet3Physics` directly.

Two examples show the issue clearly:

- `!rb_dynamic_collision_events.lua` already exposes a measurable signal
  (`nodes_in_contact`), but it still runs as an interactive infinite loop.
- `rb_mesh_collider_raycast.lua` is useful for mesh-raycast validation, but its
  current output is mostly visual debug lines and manual camera inspection.

There is another nuance: some current behaviors are already fragile or
order-sensitive.

For example:

- collision-pair queries should not be compared as raw ordered lists if the
  underlying behavior is effectively unordered;
- `RaycastAllHits` should not be validated by order because the existing C++
  test explicitly documents that hit order is not distance-sorted.

This means the future automation layer must normalize outputs before diffing
them.

## Recommended QA Architecture

The right design is a layered system, not one giant test type.

### Layer 1: Deterministic Invariant Tests

This layer should live close to the current C++ engine tests.

Its role is to validate the minimum shared contract across backends:

- body creation,
- stepping,
- scene sync,
- gravity,
- kinematic/static behavior,
- collision presence,
- first-hit raycasts,
- all-hit raycasts,
- overlap or collide-world queries.

This layer should stay mostly primitive-based and low-noise.

It is the first gate a candidate backend must pass.

### Layer 2: Bullet Baseline Trace Tests

This is the most important new layer.

For selected scenarios, run the same scripted setup twice:

1. once with Bullet to record a baseline trace,
2. once with the candidate backend to compare against it.

The trace should capture structured observables such as:

- frame index,
- transform per tracked body,
- linear and angular velocity,
- awake/sleep state,
- collision-event counts,
- normalized contact pairs,
- raycast hit node, hit point, hit normal, and hit distance,
- scenario-specific counters.

This should produce machine-readable output such as JSON.

### Layer 3: Optional Image Regression

This layer is useful for a narrow subset only:

- mesh-collider debug scenes,
- raycast-grid visualization,
- contact/debug rendering sanity checks.

It should use offscreen runtime capture rather than desktop screenshots.

This layer should be tolerant:

- image diff with thresholds,
- stable camera only,
- stable render path only,
- and only for scenarios where a visual signal adds real value.

It should never be the primary gate for core rigid-body correctness.

### Layer 4: Benchmark Mode

This layer measures the value proposition of a replacement backend.

At minimum, it should capture:

- average physics-step time,
- worst-frame physics-step time,
- body count,
- contact count,
- raycast throughput,
- and stability failures such as exploding stacks or tunneling.

Without this layer, "lighter than Bullet" remains a claim rather than a
measured result.

## Required Engineering Changes

The current content should be adapted through a small automation contract.

Recommended changes:

- introduce a shared QA helper layer for Lua scenarios, for example
  `physics-qa/lib/qa_runner.lua`;
- add a test mode that runs a fixed number of frames and exits automatically;
- remove live input dependence in QA mode by using scripted camera paths and
  scripted actions;
- force fixed `dt` and fixed physics step in QA mode;
- add per-scenario manifests describing:
  - tracked bodies,
  - expected observables,
  - tolerances,
  - and whether image capture is required;
- emit structured results to JSON or another strict machine-readable format;
- add a small native or Python diff tool that compares:
  - exact fields,
  - epsilon-based numeric fields,
  - unordered sets such as hit collections or contact pairs;
- add backend selection through a thin indirection layer so scenarios are not
  hard-wired forever to `SceneBullet3Physics`.

The last point matters. Even if Bullet remains the default path, the automation
harness should not require editing every scenario again when a second backend is
introduced.

## Recommended Result Model

The automation output should not try to record every internal solver detail.

It should record the gameplay-relevant surface.

Recommended categories:

- scenario metadata:
  - name,
  - backend,
  - fixed step,
  - total frame count;
- body state:
  - position,
  - rotation,
  - linear velocity,
  - angular velocity,
  - sleeping flag;
- collision/query state:
  - normalized node pairs,
  - contact count per pair,
  - first contact frame,
  - raycast results,
  - overlap results;
- scenario metrics:
  - peak bounce height,
  - time-to-rest,
  - number of hits,
  - number of frames in contact.

For unstable cases, milestone sampling is better than full raw-frame equality.

Example:

- compare body height at frames 10, 30, 60, 120,
- not every intermediate float exactly.

## Determinism And Tolerance Strategy

This is the main design constraint.

Two different physics engines will diverge if the suite asks for bit-exact
identity over long contact-heavy runs.

So the comparison model should be:

- strict on discrete outcomes,
- tolerant on continuous values,
- and more tolerant as scenario complexity increases.

Recommended rules:

- exact compare:
  - hit or no-hit,
  - hit node identity,
  - number of broad scenario milestones reached,
  - pass/fail event presence;
- epsilon compare:
  - hit position,
  - transform,
  - velocity,
  - bounce apex,
  - settle time;
- unordered compare:
  - `RaycastAllHits`,
  - collision pair collections;
- envelope compare:
  - friction outcomes,
  - stack stability,
  - dense-contact settling.

This is also why Bullet baselines should be recorded at the Harfang API level,
not by introspecting backend-private solver internals.

## Visual Regression: Benefit And Limits

Image regression has real value, but only as a secondary tool.

Benefits:

- catches obviously broken mesh-collider setup,
- validates raycast pattern scenes quickly,
- helps detect missing debug-render output,
- useful for release notes and QA triage.

Limits:

- GPU output is noisier than physics state,
- renderer changes can create false positives,
- camera/input instability ruins usefulness,
- it says little about exact contact semantics by itself.

Recommendation:

- use image regression only on a curated subset,
- keep the camera deterministic,
- and gate it separately from core numeric physics compatibility.

## Suggested MVP

Do not begin by automating the entire Lua suite.

Start with a narrow, high-signal subset.

Recommended MVP scenario set:

- dynamic freefall,
- kinematic no freefall,
- dynamic body versus static ground collision event,
- `NodeCollideWorld`,
- `RaycastFirstHit` hit and miss,
- `RaycastAllHits` hit and miss,
- one mesh-collider raycast scene,
- one restitution or friction envelope scene.

Recommended MVP phases:

### Phase 1: Runner Contract

- fixed-frame execution,
- structured JSON output,
- deterministic camera and scripted actions,
- clean process exit code.

### Phase 2: Bullet Baseline Recording

- generate baseline traces for the MVP scenarios,
- store them as reference artifacts in-repo or in test data.

### Phase 3: Diff And CI Integration

- compare candidate backend results against baseline and invariants,
- report exact failing frame and field,
- integrate with local and CI runs.

### Phase 4: Optional Visual And Benchmark Extensions

- add offscreen image capture for selected scenes,
- add timing and throughput metrics.

## Effort Estimate

These numbers assume one engineer already familiar with Harfang internals.

Prototype runner plus 5 to 8 deterministic scenarios:

- 1 to 2 engineer-weeks

Usable MVP with baseline recording, diffing, and CI integration:

- 3 to 5 engineer-weeks

Broader suite coverage with curated image regression and benchmark mode:

- add 2 to 4 engineer-weeks

Trying to automate the entire current Lua suite without first introducing a
runner contract is likely to waste time.

## Recommendation

Proceed.

The main recommendation is to treat this as a compatibility and benchmarking
project, not as an attempt to prove physical truth.

The most effective path is:

1. keep Bullet as the baseline reference backend;
2. formalize a deterministic scenario runner;
3. automate a narrow high-signal subset first;
4. compare at the Harfang API level with tolerances;
5. add image regression only where it brings extra signal;
6. add benchmark mode before making any backend replacement decision.

If this is done well, Harfang gets more than a migration aid.

It gets a reusable physics QA platform that remains valuable even if Bullet is
never replaced.

## Sources Checked

- `harfang3d/physics-qa/README.md`
- `harfang3d/physics-qa/!rb_dynamic_collision_events.lua`
- `harfang3d/physics-qa/rb_mesh_collider_raycast.lua`
- `harfang3d/harfang/tests/engine/scene.cpp`
- `harfang3d/languages/launcher_tests/run_launcher_regression.py`
- `harfang3d/tutorials/scene_capture_texture.lua`
- `harfang3d/doc/doc/CaptureTexture.md`
- `harfang3d/binding/bind_harfang.py`

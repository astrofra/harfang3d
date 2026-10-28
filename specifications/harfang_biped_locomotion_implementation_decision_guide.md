# Biped Locomotion in HARFANG: Implementation Decision Guide

**Research date:** 28 October 2026  
**Target:** A native C or C++ locomotion system integrated into HARFANG 3D  
**Preferred standard:** C++14; C++17 acceptable  
**Primary concern:** Convincing biped walking with gravity, acceleration, and a small dependency footprint  
**Decision status:** Architecture selection and prototype validation required

## 1. Recommended decision

No project identified in this research provides the entire requested combination as a maintained, immediately embeddable, low-dependency C++14 library: natural biped locomotion, articulated physical balance, gravity-aware control, and straightforward HARFANG integration.

The practical choice depends on what physical walking must mean in the application:

| Main requirement | Recommended starting point | Main implementation work |
| --- | --- | --- |
| Natural-looking walking, acceleration, braking, falling, and terrain adaptation | Daniel Holden's Motion Matching implementation, with HARFANG/Bullet handling character movement | Extract the controller from the demo, connect collision-aware movement, provide suitable animation data, and map poses to the HARFANG skeleton |
| The feet must physically support the character; disturbances must trigger balance corrections | Original SIMBICON 1.5, with Cartwheel 3D as an additional reference | Modernize the research code, define the articulated body, validate the controller, and adapt the physics interface |
| Fully procedural stepping without a motion database, while keeping dependencies small | A custom simplified balance and stepping controller plus two-bone IK | Implement gait planning, support transitions, terrain handling, and stabilization; use a small IK component where useful |

**Recommended implementation strategy:** Keep HARFANG/Bullet as the default physics backend and make locomotion independent of rendering and scene objects. Start with one narrowly scoped prototype before committing to a complete animation framework.

If physically maintained balance is a hard requirement, test the dynamic route first. A visually convincing capsule-driven character cannot establish that a dynamic balance controller will work.

These recommendations are engineering judgments based on source and documentation inspection. They are not the results of comparative runtime benchmarks.

## 2. Requirements and decision boundaries

### Known requirements

- Native C or C++ runtime.
- Prefer C++14; allow C++17 if the benefit justifies it.
- Minimize third-party runtime dependencies.
- Integrate with HARFANG rather than adopt a different rendering engine.
- Animate a biped with credible walking, gravity, and acceleration behavior.

### Requirements to settle before implementation

| Question | Why it changes the design |
| --- | --- |
| Must the character recover from a push by taking a corrective step? | This strongly favors physical balance control over animation following a capsule |
| Must walking change appropriately when gravity changes? | Applying a different gravity to the collision body does not automatically adapt the gait |
| Are authored or motion-captured animation clips acceptable? | Motion Matching requires a motion database; a procedural controller does not necessarily need one |
| Is the target a human, a stylized humanoid, or a robot? | Morphology and style affect acceptable gait quality, body modeling, and controller tuning |
| Which terrain features are required? | Flat ground, slopes, stairs, moving supports, and loose objects impose different control demands |
| How many characters must run simultaneously? | This determines the relevant CPU and memory budgets |
| Which platforms and distribution model are required? | These affect build validation, asset rights, and license selection |

Treat full C/C++ runtime and full C/C++ tooling as separate constraints. Offline conversion or training may be acceptable even when the deployed application must not contain Python. Build tools, demo dependencies, runtime dependencies, animation assets, and trained models should be recorded separately.

## 3. What each type of system actually solves

| Component | Responsibility | What it does not establish |
| --- | --- | --- |
| Character movement controller | Moves a collision proxy and handles grounding, slopes, collision response, and falling | That the animated legs are physically carrying the character |
| Animation runtime | Samples and blends skeletal poses | That the poses obey dynamics or maintain balance |
| Inverse kinematics, or IK | Positions limbs to meet targets such as ground contacts | When to step, where to step, or how much force to apply |
| Motion Matching | Selects suitable animation fragments using pose and trajectory features | Articulated force balance or automatic adaptation to arbitrary physical conditions |
| Gait planner | Chooses support phases, swing trajectories, and footstep targets | That the simulated body will successfully follow the plan |
| Dynamic locomotion controller | Uses measured state and contacts to generate forces or joint torques | Universal stability across arbitrary bodies and environments |

For example, a character may accelerate and fall under Bullet while its legs follow animation clips. This can satisfy many interactive graphics requirements. In a dynamically walking character, contact forces at the feet and controlled joint torques must also support the body and generate its motion.

## 4. Candidate comparison

Standard and dependency statements below refer to the inspected code or published build requirements. They are not certifications for every compiler or platform.

| Candidate | Standard / language | Main dependencies | Useful capability | Assessment for this project |
| --- | --- | --- | --- | --- |
| SIMBICON 1.5 | Historical C++; modern build unverified | ODE 0.9; legacy editor dependencies | Physically simulated biped control and balance | Best initial reference for the dynamic route; substantial integration work [S1], [S2], [S3] |
| Cartwheel 3D | C++ plus Python controller logic | ODE; historical Python and graphics infrastructure | More elaborate walking control and foot placement | Valuable reference; not a ready-made all-C++ runtime [S4], [S5], [S6], [S7] |
| Holden Motion Matching | Core headers passed a strict C++14 syntax check | Standard C/C++ libraries in the inspected core; raylib/raygui in the demo | Natural locomotion selected from a motion database | Strong candidate for the visual locomotion route [S8], [S9], [S10] |
| ozz-animation | Version 0.15 declares C++11; current runtime declares C++17 | Standard C/C++ libraries at runtime | Pose sampling, blending, and IK | Useful building block; does not supply a walking controller [S11], [S12], [S13], [S14] |
| TheComet/ik | C89 | Standard C library | General IK and weighted effectors | Small component for a custom system [S15] |
| SmartBody | C++; Worldforge fork states C++14 | Broad dependency set, depending on components | Locomotion and a wider character behavior platform | Larger integration scope than the immediate requirement [S16], [S17] |
| TOWR | C++11 in the documented build | Eigen, ifopt, Ipopt | Dynamics-constrained locomotion trajectory optimization | Planning component; requires tracking and skeletal integration [S18] |
| Bipedal Locomotion Framework | C++17 in inspected build helpers | Eigen, iDynTree, spdlog; additional module dependencies | Humanoid planning, IK, and control components | Relevant for robotics-oriented simulation; heavier integration [S19], [S20] |

## 5. Shortlist assessment

### SIMBICON 1.5: first dynamic-control prototype

SIMBICON demonstrates physically simulated locomotion, changes in direction and speed, and robustness to disturbances. Controllers can be authored with parameters or derived from motion-capture information. The official project includes 2D and 3D results. [S1]

The original 1.5 archive contains C++ controller and simulation sources and no Python source files. Its projects reference ODE 0.9; the editor also references Tcl/Tk and OpenGL-related libraries. The published instructions target Visual Studio 2008. [S2], [S3]

**Implementation consequence:** Separate the controller, physics adaptation, data loading, and visualization. Do not assume that compiling an old application automatically produces a clean reusable library.

**License finding:** The original distribution declares GPLv3 or later. License suitability is a selection gate for this exact codebase. [S3]

**Prototype objective:** Reproduce one stable walking example before changing the physics backend or character proportions. Establish a reference run so later failures can be attributed to the adapter, body model, or controller changes.

### Cartwheel 3D: advanced control reference

The associated walking-control method combines proportional-derivative tracking, inverted-pendulum foot placement, and gravity and velocity-error adjustments. The published examples include walking transitions, turns, different body proportions, and carrying loads. [S4]

The inspected repository contains substantial C++ control code, but also a Python `WalkController` derived from the C++ controller. Historical project files link Python 2.6 and graphics libraries. An all-C++ deployment therefore requires extracting or reimplementing relevant higher-level logic. [S5], [S6]

**License finding:** The Banbury mirror has an Apache 2.0 root license, while `Core/SimBiController.cpp` retains a GPLv3-or-later notice. Do not treat the repository's top-level license label as sufficient evidence for every source file. [S5], [S7]

**Decision:** Use Cartwheel to inform a dynamic implementation, particularly foot placement and balance behavior. Avoid selecting it on the assumption of immediate low-dependency integration.

### Holden Motion Matching: strongest lightweight visual route

This implementation provides example Motion Matching and Learned Motion Matching systems. The companion article explains how to coordinate simulated displacement, animation, trajectory adjustment, and foot locking. [S8], [S9]

The inspected mathematical, database, and network headers use standard C/C++ facilities. The graphical application uses raylib and raygui. The demo controller also contains contact-locking and two-bone IK logic. [S8], [S10]

**Verification performed:** A translation unit including all nine core headers passed `g++ -std=c++14 -pedantic-errors -fsyntax-only`. This confirms that these headers parse under the tested compiler. It does not validate the full demo, Microsoft compiler compatibility, runtime correctness, or HARFANG integration.

**Implementation consequence:** Extract a headless locomotion component, keep world collision and gravity in HARFANG/Bullet, and use the motion system to generate the visible skeleton pose.

**Asset rights:** The code is MIT-licensed, but the dataset identified by the repository is CC BY-NC-ND 4.0. Select animation data with rights appropriate to the intended deployment. The classical matching route does not require neural-network training; retraining the learned variant uses Python. [S8]

### ozz-animation and TheComet/ik: components for a custom system

ozz is engine- and renderer-independent. Its runtime has no third-party dependencies beyond standard C/C++ libraries; import tools and sample applications have separate dependencies. It supplies skeletal animation and IK primitives rather than high-level locomotion. [S11]

The inspected 0.15 build configuration declares C++11. The current project declares C++17. Keeping 0.15 for a C++14 application means reviewing later fixes, including IK fixes, rather than assuming equivalent behavior across versions. [S11], [S12], [S14]

The foot-IK example corrects leg targets, ankle orientation, and pelvis height. Its documentation explicitly says a complete foot-planting system still requires contact activation, transitions, unreachable-target handling, and suitable animation layering. [S13]

TheComet/ik is a smaller C89 alternative with standard-library-only dependencies. It supplies IK rather than gait planning or physical balance. [S15]

**Decision:** Add either component only if it fills a demonstrated gap. If HARFANG already supplies adequate animation playback, adopting another complete animation runtime may create unnecessary duplication.

## 6. Alternatives that should not drive the first implementation

| Project or category | Reason to defer |
| --- | --- |
| SmartBody | Broad animation and behavior scope. The Worldforge fork reports remaining integration, portability, and stability work [S16], [S17] |
| TOWR | Produces optimized trajectories; a real-time body controller and pose pipeline are still required [S18] |
| Bipedal Locomotion Framework | Useful modular robotics functionality, but more dependencies and an explicitly experimental API [S19] |
| roboticslab-uc3m/gait | Small and CMake-integrable, but inspected source contains incomplete functions and hard-coded physical parameters; needs further validation [S21] |
| DeepMimic | Distributed as a C++/Bullet simulation with Python and TensorFlow infrastructure; repository now marks the codebase deprecated [S22] |
| Replacing Bullet with another general-purpose physics engine | Collision handling, ragdolls, and joint motors still require a locomotion and balance controller |

These exclusions concern suitability for this implementation, not the scientific value or general quality of the projects.

## 7. Proposed HARFANG architecture

The design below is a proposal, not an existing HARFANG module.

### Common separation of responsibilities

| Layer | Owns | Boundary |
| --- | --- | --- |
| Application intent | Desired velocity, facing, stopping, and requested actions | Supplies commands without directly modifying the skeleton |
| Locomotion controller | Gait state, trajectory choice, support transitions, and controller state | Uses plain native data structures |
| Physics adapter | Collision queries, body state, contacts, forces, torques, and simulation timing | Translates between the controller and HARFANG/Bullet |
| Pose adapter | Joint mapping, coordinate conversion, rest-pose offsets, and final pose application | Translates controller output into HARFANG scene or skeleton transforms |
| Debug instrumentation | Contact state, foot targets, errors, and timing measurements | Observes the system without being a runtime dependency |

Avoid embedding HARFANG scene nodes or renderer types in the controller's core API. This makes headless validation and later backend changes easier.

### Visual locomotion pipeline

1. Convert input or navigation intent into a desired velocity and facing direction.
2. Move the collision proxy using collision-aware acceleration and gravity handling.
3. Feed actual displacement and a feasible future trajectory into animation selection.
4. Generate the base skeletal pose.
5. Apply support-aware foot locking, terrain IK, and pelvis correction.
6. Apply the resulting pose through the HARFANG adapter.

Choose one authority for root displacement. If physics owns the character trajectory, animation root motion must be reconciled with that trajectory rather than independently moving the character again.

Ground contact cannot be inferred from ankle height alone. Use animation contact information or explicit gait state, then confirm terrain suitability through physics queries. Release a foot lock when the motion requires a step or the target is no longer reachable.

### Dynamic locomotion pipeline

1. Read body poses, velocities, and contact information at a physics-step boundary.
2. Update the support state and choose the next footstep target.
3. Generate desired joint behavior and balance corrections.
4. Apply bounded joint torques or appropriate motor commands.
5. Advance the articulated-body simulation.
6. Map simulated body transforms to the visible skeleton.

The floating root must remain free to respond dynamically if the goal is physical balance. Prescribing its trajectory independently can conceal controller failure.

Mass distribution, inertia, joint axes, limits, friction, and collision geometry are part of the controller setup. Matching joint names alone is insufficient to transfer a research example to a new character.

### HARFANG-specific considerations

The documented API provides force and torque application, raycasts, collision events, 6DOF constraints, and a pre-tick callback. These are useful integration points; motor configuration and contact access must still be checked against the exact HARFANG/Bullet build selected for implementation. [S23], [S24], [S25]

- Run control against a fixed physics step and interpolate visual output as needed. Validate the chosen step instead of tying control gains to display frame rate.
- Respect HARFANG's scene/physics synchronization rules. Dynamic bodies obtain their world transforms from physics; writing a transform is not equivalent to applying a dynamic command. [S25]
- Define an explicit joint map, coordinate convention, unit scale, rest-pose conversion, and skinning relationship.
- Give every root and joint transform a clear owner. Prevent animation evaluation and physics synchronization from overwriting each other's final output.
- Do not assume a public wrapper exposes every Bullet capability required by the controller. Identify necessary extensions during the prototype.

### Retaining ODE versus adapting to Bullet

| Option | Benefit | Cost / risk |
| --- | --- | --- |
| Reproduce SIMBICON using its original ODE setup | Preserves the initial research configuration and simplifies behavioral comparison | Introduces another physics backend |
| Adapt the controller to HARFANG/Bullet | Keeps character/world interaction in one simulation | Solver differences, contact behavior, joint conventions, and gains require validation |
| Operate ODE and Bullet together | Can isolate an early experiment | Requires explicit world synchronization and interaction handling; bodies in separate solvers do not automatically exchange forces |

Prefer the original backend for a reference reproduction if needed, then use that reference to assess the Bullet adapter. Avoid committing to two interacting physics worlds without a concrete requirement.

## 8. Prototype plan and go/no-go gates

### Stage 0: freeze the experiment

Choose one character, one target platform, one build configuration, and a small required behavior set. Record whether clip-based animation and offline Python are acceptable. Review the exact source and asset licenses intended for the prototype.

**Exit condition:** The team can state whether success means visual plausibility or dynamically maintained balance, and can identify the code and data being evaluated.

### Stage 1: build a headless core

For the visual route, extract the Motion Matching core and a minimal pose-output path without the graphical demo. For the dynamic route, reproduce a supplied SIMBICON example before replacing ODE or its body model.

**Exit condition:** The selected code builds on the required compiler and runs a repeatable scenario. Runtime dependencies are explicitly inventoried.

### Stage 2: validate the HARFANG boundary

Display the generated pose on a HARFANG skeleton. Verify scale, axes, parent transforms, rest-pose offsets, and ownership of the character root. Use simple debug geometry before relying on a detailed character mesh.

For dynamic control, verify joint torque direction and contact information before attempting walking.

**Exit condition:** Pose mapping is correct and scene synchronization does not overwrite controller output.

### Stage 3: test the behaviors that distinguish the routes

Run the scenarios in Section 9. Do not spend time on secondary animation polish before testing push recovery or gravity changes if those are selection requirements.

**Exit condition:** Required behaviors pass with measured results; failures have a documented cause or a bounded next experiment.

### Stage 4: commit to an implementation

Pin exact source revisions and asset versions. Record the chosen architecture, accepted limitations, dependency list, and remaining work. Replace provisional prototype assumptions with measured budgets.

**Exit condition:** A completed decision record identifies why the selected approach satisfies the project, and why the alternatives were deferred.

## 9. Validation scenarios and measurements

Set numerical acceptance thresholds before final selection. The table defines what to measure without inventing project-specific budgets.

| Scenario | Visual-route check | Additional dynamic-route check |
| --- | --- | --- |
| Stand still | Stable feet and pose without visible jitter | Body maintains support without a hidden root constraint |
| Start, stop, and reverse | Appropriate transitions and limited foot sliding | Contact forces and balance remain controlled through transitions |
| Accelerate and turn | Animation follows actual displacement without excessive drift | Controller changes support placement and body motion successfully |
| Traverse a slope | Feet align to terrain; pelvis correction remains plausible | Sufficient traction and stable support under the tested slope |
| Step up and down | Targets remain reachable and swing feet clear obstacles | Body clears the step and transfers support successfully |
| Receive a lateral push | Physical root response and plausible visual reaction | Corrective stepping or another successful balance response occurs |
| Change gravity | Falling and grounding remain consistent | Gait remains successful or its supported gravity range is documented |
| Stand on a moving platform | Foot targets follow the support reference frame | Support motion and contact forces are handled consistently |
| Change display frame rate | Similar motion and transitions | Similar controller behavior with the same physics-step configuration |
| Run the target character count | Acceptable CPU and memory cost | Stable solver behavior at the required simulation workload |

Record at least:

- Ground-relative foot drift during intended support.
- Desired versus achieved velocity and heading.
- Position and orientation discontinuities at transitions.
- Falls or failed recoveries over repeated scenarios.
- Joint-limit violations, penetration, jitter, and torque saturation.
- CPU time per controller update and physics step, with workload and hardware stated.
- Memory use and motion-database size where applicable.
- Character dimensions, mass properties, friction, time step, and controller settings.

For gait changes or gravity changes, distinguish a controller that adapts online from one that succeeds only after manual retuning.

## 10. Main implementation risks

| Risk | Early evidence to collect | Response |
| --- | --- | --- |
| Choosing IK as if it were a walking controller | What generates step timing, foot targets, and balance commands? | Define missing gait/control components explicitly |
| Research code is difficult to reuse | Can one supplied example run without the original editor? | Establish a reference build and isolate dependencies before porting |
| Physics-backend substitution breaks walking | Compare the same body and commands across backends | Validate joints, inertia, contact settings, time step, and gains incrementally |
| Motion data does not cover required actions | Test stops, reversals, turns, and the intended speed range | Expand or replace the database, or narrow supported behavior |
| Source and asset licenses differ | Inspect exact files and dataset/model terms | Select compatible code and data before distribution |
| Two systems write the same transforms | Trace pose and root ownership through one frame | Define a single final owner and explicit handoff points |
| An old C++14-compatible release lacks later fixes | Review upstream changes affecting used features | Backport relevant fixes or accept C++17 |
| Claimed realism hides root assistance | Disable or expose root constraints and artificial support | Report the actual physical model and supported behaviors |

## 11. Decision record template

Copy and complete this block after the prototype:

```text
Decision:
Date:
Owner:

Required behaviors:
Physical balance required: yes / no
Animation clips permitted: yes / no
Offline Python permitted: yes / no

Chosen locomotion approach:
Chosen source revisions:
Chosen physics backend and version:
C/C++ standard and tested compilers:
Target platforms:
Runtime dependencies:
Offline and build dependencies:
Animation data or model sources and licenses:

HARFANG pose mapping verified:
Root and joint transform ownership:
Physics step and update order:
Required HARFANG/Bullet wrapper extensions:

Scenarios tested:
Measurements and acceptance thresholds:
Known failure cases:
Accepted limitations:

Alternatives rejected and reasons:
Remaining implementation tasks:
Conditions that would trigger reconsideration:
```

## 12. Evidence and limits of this research

The assessment combines primary project documentation with selected source and build-file inspection. It includes inspection of the original SIMBICON archive, Cartwheel's C++ and Python controller layers, ozz build configurations, and HARFANG physics declarations.

The Motion Matching syntax probe included these headers: `common.h`, `vec.h`, `quat.h`, `spring.h`, `array.h`, `character.h`, `database.h`, `nnet.h`, and `lmm.h`.

```sh
g++ -std=c++14 -pedantic-errors -fsyntax-only syntax_probe.cpp
```

The probe passed in the research environment. It was a header syntax check, not a complete application build or runtime test.

No full HARFANG integration, dynamic stability benchmark, cross-platform build matrix, or multi-character performance comparison was performed. Integration effort assessments are qualitative. The research did not freeze exact commit hashes for every inspected repository; do so before implementation. Moving branch links below may subsequently change.

## 13. Primary sources

- **S1** — [SIMBICON project and paper](https://www.cs.ubc.ca/~van/papers/simbicon.htm)
- **S2** — [SIMBICON 1.5 distribution](https://www.cs.ubc.ca/~van/simbicon_cef/index.html)
- **S3** — [SIMBICON 1.5 README and license declaration](https://www.cs.ubc.ca/~van/simbicon_cef/README.txt)
- **S4** — [Generalized Biped Walking Control](https://www.cs.ubc.ca/~van/papers/2010-TOG-gbwc/index.html)
- **S5** — [Cartwheel 3D repository mirror](https://github.com/Banbury/cartwheel-3d)
- **S6** — [Cartwheel Python walking controller](https://github.com/Banbury/cartwheel-3d/blob/master/Python/Controllers/WalkController.py)
- **S7** — [Cartwheel source containing the GPL notice](https://github.com/Banbury/cartwheel-3d/blob/master/Core/SimBiController.cpp)
- **S8** — [Motion Matching source and code/data licensing notes](https://github.com/orangeduck/Motion-Matching)
- **S9** — [Code vs Data Driven Displacement](https://theorangeduck.com/page/code-vs-data-driven-displacement)
- **S10** — [Motion Matching demo controller, contact handling, and IK](https://github.com/orangeduck/Motion-Matching/blob/main/controller.cpp)
- **S11** — [ozz-animation project, runtime requirements, and license](https://github.com/guillaumeblanc/ozz-animation)
- **S12** — [ozz 0.15 compiler settings](https://github.com/guillaumeblanc/ozz-animation/blob/0.15.0/build-utils/cmake/compiler_settings.cmake)
- **S13** — [ozz foot-IK example and stated limitations](https://guillaumeblanc.github.io/ozz-animation/samples/foot_ik/)
- **S14** — [ozz release notes](https://github.com/guillaumeblanc/ozz-animation/releases)
- **S15** — [TheComet/ik](https://github.com/TheComet/ik)
- **S16** — [Original SmartBody project](https://smartbody.ict.usc.edu/)
- **S17** — [Worldforge SmartBody fork and integration limitations](https://github.com/worldforge/smartbody)
- **S18** — [TOWR, dependencies, and non-ROS build](https://github.com/ethz-adrl/towr)
- **S19** — [Bipedal Locomotion Framework and module dependencies](https://github.com/gbionics/bipedal-locomotion-framework)
- **S20** — [Bipedal Locomotion Framework C++ build requirements](https://github.com/gbionics/bipedal-locomotion-framework/blob/master/cmake/AddBipedalLocomotionLibrary.cmake)
- **S21** — [UC3M gait source](https://github.com/roboticslab-uc3m/gait/blob/master/src/GaitLipm.cpp)
- **S22** — [DeepMimic requirements and deprecation notice](https://github.com/xbpeng/DeepMimic)
- **S23** — [HARFANG source repository](https://github.com/harfang3d/harfang3d)
- **S24** — [HARFANG 3.2.5 physics additions](https://dev.harfang3d.com/releases/3.2.5/)
- **S25** — [HARFANG physics synchronization documentation](https://dev.harfang3d.com/docs/3.2.7/man.physics/)


[S1]: https://www.cs.ubc.ca/~van/papers/simbicon.htm
[S2]: https://www.cs.ubc.ca/~van/simbicon_cef/index.html
[S3]: https://www.cs.ubc.ca/~van/simbicon_cef/README.txt
[S4]: https://www.cs.ubc.ca/~van/papers/2010-TOG-gbwc/index.html
[S5]: https://github.com/Banbury/cartwheel-3d
[S6]: https://github.com/Banbury/cartwheel-3d/blob/master/Python/Controllers/WalkController.py
[S7]: https://github.com/Banbury/cartwheel-3d/blob/master/Core/SimBiController.cpp
[S8]: https://github.com/orangeduck/Motion-Matching
[S9]: https://theorangeduck.com/page/code-vs-data-driven-displacement
[S10]: https://github.com/orangeduck/Motion-Matching/blob/main/controller.cpp
[S11]: https://github.com/guillaumeblanc/ozz-animation
[S12]: https://github.com/guillaumeblanc/ozz-animation/blob/0.15.0/build-utils/cmake/compiler_settings.cmake
[S13]: https://guillaumeblanc.github.io/ozz-animation/samples/foot_ik/
[S14]: https://github.com/guillaumeblanc/ozz-animation/releases
[S15]: https://github.com/TheComet/ik
[S16]: https://smartbody.ict.usc.edu/
[S17]: https://github.com/worldforge/smartbody
[S18]: https://github.com/ethz-adrl/towr
[S19]: https://github.com/gbionics/bipedal-locomotion-framework
[S20]: https://github.com/gbionics/bipedal-locomotion-framework/blob/master/cmake/AddBipedalLocomotionLibrary.cmake
[S21]: https://github.com/roboticslab-uc3m/gait/blob/master/src/GaitLipm.cpp
[S22]: https://github.com/xbpeng/DeepMimic
[S23]: https://github.com/harfang3d/harfang3d
[S24]: https://dev.harfang3d.com/releases/3.2.5/
[S25]: https://dev.harfang3d.com/docs/3.2.7/man.physics/

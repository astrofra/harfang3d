# Harfang Task Scheduler Specification

Date: 2026-09-05

Status: proposed architecture and implementation roadmap

This document specifies a reusable Harfang task scheduler and its optional
adapter for running independent Lua scripts. It is an engine-level facility,
not a Tau subsystem.

## Executive Summary

Harfang should gain one reusable, instance-owned CPU task scheduler in the
`foundation` layer. Engine subsystems may share an explicitly provided
scheduler, while applications may create a dedicated scheduler when isolation
is required.

The first implementation should provide:

- a bounded set of long-lived worker threads;
- fire-and-forget tasks, waitable task handles, and task groups;
- deterministic indexed `ParallelFor` partitioning;
- cooperative cancellation and explicit shutdown;
- worker-safe completion queues drained by the owning thread;
- synchronous operation when configured with zero background workers;
- metrics sufficient to detect undersubscription, oversubscription, queue
  pressure, and tasks that are too small.

Lua support must be layered on top of this native executor. Parallel Lua work
is allowed only when each independently executing script owns a distinct Lua
VM. A single `lua_State`, including the one held by `SceneLuaVM`, must never be
entered concurrently. Inputs and outputs cross VM and thread boundaries as
owned, data-only values; raw `LuaObject`, functions, coroutines, tables with
identity, userdata, and arbitrary Harfang object wrappers do not cross those
boundaries.

The scheduler must be useful beyond physics: asset-independent background
computation, procedural work, compression, data preparation, independent Lua
jobs, and future engine systems should all be able to use it. It must not be
hidden in `assetc`, Tau, Bullet, the renderer, or the Lua binding layer.

Tau integration and further Tau performance work are deliberately out of
scope for the implementation covered by this specification.

## Decision

Use this ownership and dependency direction:

```text
application / engine subsystem
             |
             v
  foundation::TaskScheduler
             ^
             |
   script::LuaTaskService
             |
             v
  isolated Lua VM sessions
```

The native scheduler knows nothing about:

- Lua;
- scenes;
- physics;
- rendering;
- assets;
- Fabgen;
- a particular application main loop.

The Lua adapter knows about the scheduler and Harfang's Lua VM helpers, but the
scheduler never depends on the adapter.

No process-global scheduler is required by this specification. An application
may own one scheduler and explicitly pass it to several consumers. This keeps
lifetime and shutdown visible and avoids accidentally creating one thread pool
per subsystem.

## Current Harfang Constraints

The design is based on the current local implementation:

- Harfang is built as C++14.
- `harfang/foundation/thread.h` exposes thread naming, priority, affinity, and
  system thread/core counts, but no task executor or worker pool.
- Existing uses of `std::thread` are local facilities rather than a reusable
  scheduler.
- `harfang/script/lua_vm.h` owns the VM creation, destruction, compilation,
  execution, call, environment, and watchdog helpers needed by the Lua layer.
- `LuaObject` contains a registry reference associated with one `lua_State`.
  Its lifetime and destruction therefore remain tied to that VM.
- `PushForeign` supports a limited set of cross-VM values, but it does not make
  Lua tables, closures, coroutines, VM state, or arbitrary wrapped native
  objects thread-safe.
- `SceneLuaVM` owns one VM and retains script objects in that VM. Its existing
  update path is serialized and must remain so.
- Several Harfang facilities, notably scene mutation, graphics, windows,
  input, and most externally owned engine objects, have main-thread or
  undocumented threading requirements.

These constraints rule out scheduling an existing Lua closure on an arbitrary
worker. They also rule out treating the full `hg` binding surface as worker
safe by default.

## Goals

### Native scheduler goals

1. Provide a small general-purpose CPU task API in `foundation`.
2. Reuse a fixed group of workers instead of creating threads per task.
3. Support both coarse independent tasks and deterministic indexed loops.
4. Make zero-worker synchronous execution a supported mode for tests,
   constrained platforms, and compatibility.
5. Make task and scheduler lifetimes explicit and safe during shutdown.
6. Support nested task submission and waits without deadlocking all workers.
7. Capture failures at the task boundary instead of letting them terminate a
   worker or cross an ABI boundary.
8. Provide stable task indices so consumers can produce deterministic output
   even when execution order varies.
9. Avoid steady-state allocation in recurring workloads after capacity has
   been established.
10. Expose enough diagnostics to make later parallel integrations measurable.

### Independent Lua execution goals

1. Run two or more independent Lua scripts concurrently on distinct VMs.
2. Serialize all work targeting the same Lua session.
3. Transfer bounded, owned, data-only inputs and outputs.
4. Report Lua errors, tracebacks, cancellation, and timeouts as task results.
5. Allow completion to be observed safely from a main Lua VM.
6. Keep existing `SceneLuaVM` behavior and public APIs unchanged.
7. Bind the new service through Fabgen as an additive API.

### Reuse goals

The scheduler must be suitable for future consumers such as:

- procedural geometry or navigation data preparation;
- compression, decompression, hashing, and content transforms;
- background parsing and data validation;
- independent simulation or tool workloads;
- parallel engine loops whose state ownership has been made explicit;
- physics backends, if and when their integration is resumed.

## Non-Goals

The first implementation will not:

- resume Tau optimization or change Tau simulation behavior;
- replace Bullet's internal scheduler;
- parallelize an existing `SceneLuaVM`;
- provide actor-style shared mutable Lua state;
- migrate a live Lua coroutine between VMs;
- accept Lua closures as background entry points;
- make all Harfang bindings thread-safe;
- provide a security sandbox for untrusted Lua;
- provide hard real-time guarantees;
- forcibly terminate worker threads or Lua VMs;
- promise identical task execution order across runs;
- implement distributed or process-based jobs;
- make every short loop worth parallelizing automatically.

## Terminology

**Scheduler**
: The owner of worker threads, queues, task records, and completion records.

**Background worker**
: A long-lived thread owned by the scheduler. `worker_count` always means the
  number of these threads; it does not include the thread calling the API.

**Task**
: One native callable submitted to the scheduler.

**Task group**
: A waitable set of tasks with one aggregate completion state.

**Owning thread**
: The thread that owns a scheduler-facing consumer or Lua API object and drains
  its completions. For engine and script integrations this is normally the
  main thread.

**Lua task session**
: A serialized execution lane owning exactly one independent Lua VM.

**Task value**
: An owned, bounded, data-only value that can safely cross a task, thread, or
  Lua VM boundary.

**Affinity lane**
: A logical queue whose tasks execute serially. A Lua task session uses one to
  ensure that its VM is never entered by two workers at once.

## Layering And Proposed Files

The names below are architectural guidance, not a frozen source ABI:

```text
harfang/foundation/task_scheduler.h
harfang/foundation/task_scheduler.cpp
harfang/foundation/task_value.h
harfang/foundation/task_value.cpp

harfang/script/lua_task_service.h
harfang/script/lua_task_service.cpp

harfang/tests/foundation/task_scheduler.cpp
harfang/tests/script/lua_task_service.cpp
```

Fabgen declarations belong with Harfang's other public bindings. They expose
the high-level scheduler and Lua task service, not worker internals or
`lua_State *`.

`TaskValue` may be placed in `foundation` if native consumers need the same
message format. Its implementation must remain C++14-compatible and therefore
must not require `std::variant`.

## Native Execution Model

### Worker count

`worker_count` is the number of background threads:

- `0`: tasks execute synchronously through the submitting/waiting thread;
- `1`: one background worker exists;
- `N`: exactly N background workers exist unless platform initialization
  fails.

This definition avoids the common ambiguity between worker count and total
participating thread count. A subsystem that exposes a total `thread_count`
can translate it explicitly to `max(thread_count - 1, 0)` background workers.

The default should be conservative and derived from
`get_system_core_count()`. It must leave capacity for the main/render thread
and must be overrideable. The scheduler must not apply CPU affinity or elevated
priority by default.

### Queue and wake-up policy

The first implementation should favor a simple, measured design:

- one bounded or capacity-retaining shared ready queue;
- a mutex and condition variable, or an equivalently well-tested C++14
  primitive;
- a separate completion queue;
- no polling or busy wait while idle;
- batched wake-up when a submitted group exposes multiple runnable tasks;
- retained task-record storage and queue capacity after warm-up.

Per-worker deques and work stealing are a later optimization. They should be
introduced only if measurements show contention or poor locality in the
shared queue. A sophisticated queue is not an acceptance criterion for the
first correct implementation.

### Caller participation and waits

Waiting must not strand runnable work.

- A worker waiting on a task owned by the same scheduler helps execute ready
  tasks until its dependency completes.
- The owning thread may also help while performing a blocking wait when the
  API call explicitly permits it.
- A non-helping wait remains available when the caller has thread-affine state
  and cannot legally execute arbitrary tasks.
- Code must never wait for a task while holding a lock needed by that task.

Nested submission is supported. Cyclic dependency graphs are programmer
errors; debug builds should detect direct self-waits and report useful task
identities where possible.

### Task states

A task has one terminal transition:

```text
Created -> Queued -> Running -> Completed
                            \-> Failed
                            \-> Cancelled

Created -> Cancelled
Queued  -> Cancelled
```

Cancellation after `Running` is cooperative. A running task observes a token
and exits at a safe point. The scheduler never kills a thread.

Completion publication uses release/acquire synchronization so that all task
writes are visible to a successful wait or poll.

### Handles and lifetime

`TaskHandle` is opaque and generation-safe. Reusing a task-record slot must not
make a stale handle refer to a new task.

A handle does not expose a raw task-record pointer. It may outlive completion,
but not scheduler destruction. Debug builds should diagnose use with the wrong
scheduler.

Completed records may be recycled only after:

- their work has terminated;
- completion has been published;
- retained result/error data is no longer referenced;
- no task group needs the record.

### Task groups

A group supports incremental addition before it is sealed, or a builder API
that submits a known count. Once sealed, completion occurs when every member is
terminal.

Group failure policy is explicit:

- all already-running tasks finish cooperatively;
- queued siblings may either continue or be cancelled according to the group
  policy selected at creation;
- the aggregate result preserves the first failure and the total number of
  failures;
- deterministic consumers must not depend on which worker observed a failure
  first.

### ParallelFor

`ParallelFor` partitions `[0, count)` into stable, contiguous chunks. Each
chunk receives a monotonically increasing `chunk_index` independent of the
worker that executes it.

Required inputs are:

- item count;
- minimum grain size or desired maximum chunk count;
- callable receiving begin, end, chunk index, and cancellation context.

The partition must be computed before execution. Work stealing may change
which worker executes a chunk, but never its bounds or index.

Consumers obtain deterministic results by:

1. writing disjoint output ranges, or
2. writing one task-local result per stable chunk, then
3. merging those results in ascending chunk order.

The scheduler guarantees neither floating-point associativity nor deterministic
side-effect order.

### Priorities

Version 1 needs only two advisory priorities:

- `Normal` for frame-related work;
- `Background` for throughput work that may yield to normal tasks.

There is no `RealTime` priority. Priority must not bypass dependency or lane
serialization rules, and starvation tests are required if more priority
levels are introduced.

## Illustrative Native API

The exact names may change during implementation. Semantics and ownership are
the contract that matters.

```cpp
namespace hg {

struct TaskSchedulerConfig {
    size_t worker_count = 0;
    size_t initial_task_capacity = 256;
    size_t max_queued_tasks = 0; // 0 means grow with a recorded high-water mark.
    std::string worker_name_prefix = "Harfang Worker";
};

enum class TaskPriority { Normal, Background };
enum class TaskStatus { Invalid, Queued, Running, Completed, Failed, Cancelled };

class TaskCancellationToken {
public:
    bool IsCancellationRequested() const;
};

struct TaskContext {
    TaskCancellationToken cancellation;
    size_t worker_index;
};

class TaskHandle;
class TaskGroup;

class TaskScheduler {
public:
    explicit TaskScheduler(const TaskSchedulerConfig &config);
    ~TaskScheduler();

    TaskHandle Submit(TaskFunction function,
        TaskPriority priority = TaskPriority::Normal);

    TaskGroup ParallelFor(size_t count, size_t minimum_grain,
        IndexedRangeTaskFunction function,
        TaskPriority priority = TaskPriority::Normal);

    TaskStatus GetStatus(TaskHandle handle) const;
    bool IsComplete(TaskHandle handle) const;
    void RequestCancellation(TaskHandle handle);

    bool TryWait(TaskHandle handle);
    void Wait(TaskHandle handle, WaitPolicy policy = WaitPolicy::Help);
    void Wait(TaskGroup group, WaitPolicy policy = WaitPolicy::Help);

    size_t DrainCompletions(CompletionFunction function,
        size_t maximum_count = size_t(-1));

    void Shutdown(ShutdownPolicy policy = ShutdownPolicy::Drain);
    TaskSchedulerStats GetStats() const;
};

} // namespace hg
```

The implementation may use templates at the native call site, but callable
storage must have a documented size and allocation policy. No exception,
`lua_State *`, or renderer/physics native type crosses this interface.

## Error Handling

Task failures are retained as owned diagnostic data:

- a stable error category;
- a human-readable message;
- an optional task label;
- an optional native diagnostic or Lua traceback;
- no pointer into worker-local or VM-local memory.

If C++ exceptions are enabled, the worker boundary catches them and records a
failure. Unknown exceptions become a generic failure. Tasks must not throw
through a C ABI, Fabgen wrapper, or worker entry point.

Builds that disable exceptions use explicit task result/error reporting. The
scheduler API and state machine must work in both configurations.

One task failure must not terminate a worker or silently prevent unrelated
tasks from running.

## Shutdown Contract

Shutdown is explicit, idempotent, and has two policies:

- `Drain`: reject new submissions, finish queued/running work, drain internal
  destruction work, then join every worker;
- `CancelPending`: reject new submissions, cancel queued work, request
  cooperative cancellation of running work, then join every worker.

The destructor invokes a documented default shutdown policy and never detaches
workers.

Objects used by tasks must outlive those tasks. Higher-level adapters should
capture owned state where practical and reject new work as soon as their own
shutdown begins.

For a Lua service, all VM destruction is scheduled on the VM's affinity lane
and completes before the native scheduler joins that worker.

## Main-Thread Completion Boundary

Worker tasks must not invoke arbitrary application or Lua callbacks directly.
Instead they publish owned completion records.

The owning thread calls `DrainCompletions` or the high-level equivalent from a
known safe point in its loop. Only that drain operation may:

- invoke a callback stored in the main Lua VM;
- mutate a scene;
- create or destroy renderer-facing resources;
- publish results to a UI;
- call APIs documented as main-thread only.

This boundary is mandatory even if a particular callback appears harmless. It
keeps completion behavior stable when the task later gains a different
implementation.

## Independent Lua Script Model

### Fundamental invariant

One `lua_State` is entered by at most one thread at a time.

That includes:

- compilation;
- execution;
- registry access;
- garbage collection;
- `LuaObject` creation and destruction;
- error and traceback extraction;
- calls into bound native functions.

A mutex around a shared VM is not considered parallel Lua execution. It may
serialize access safely, but it provides no independent script concurrency and
retains difficult ownership hazards.

### Session ownership

Each `LuaTaskSession` owns:

- one Lua VM created with `NewLuaVM`;
- one affinity lane;
- its initialization configuration;
- its compiled program/module state;
- its pending invocation queue;
- its cancellation/watchdog state;
- its data-only completion records.

VM creation, initialization, calls, collection, and destruction all happen on
the session lane. Calls submitted to the same session are strictly FIFO and
never overlap. Calls submitted to distinct sessions may run concurrently.

An affinity lane is initially pinned to one worker for simple lifetime and
thread-local-library behavior. Migration may be considered later only after
every bound module used by the session has been audited.

### One-shot and persistent scripts

The high-level adapter exposes two concepts:

1. A persistent session initializes a VM once and accepts serialized calls to
   named entry points.
2. A one-shot script creates a session, initializes it, performs one call, and
   destroys the VM after its result is published.

One-shot execution has clear isolation but pays VM startup cost. Persistent
sessions are the recommended mode for recurring jobs.

Version 1 must not silently pool or recycle a VM between unrelated sessions.
Lua globals, the registry, `package.loaded`, native module state, and finalizer
ordering make a complete reset difficult to guarantee. A future opt-in VM pool
requires a separately specified reset contract and isolation tests.

### Script entry points

A background Lua submission identifies executable code using one of:

- a source string copied into the session;
- preloaded byte/source data owned by the submission;
- an asset/module identifier resolved into owned bytes before dispatch;
- a named function already initialized inside a persistent session.

A closure or coroutine from the caller's VM cannot be submitted. Lua bytecode
is accepted only when it matches Harfang's Lua runtime/version and the normal
project trust policy; it is not a portable interchange format.

Asset access is not implicit. The application either loads the script before
submission or provides a read provider explicitly documented as thread-safe.
The scheduler itself never reads source or compiled assets.

### Data boundary

Lua task inputs and outputs use `TaskValue`, with these initial value kinds:

- nil;
- boolean;
- signed 64-bit integer;
- double;
- UTF-8 string;
- byte buffer;
- ordered array of task values;
- string-keyed ordered map of task values.

Every value is deeply owned. Limits are configurable for:

- maximum recursion depth;
- maximum collection element count;
- maximum string/blob bytes;
- maximum total serialized bytes per message.

Lua-to-`TaskValue` conversion rejects:

- cycles and repeated table identity that would require graph semantics;
- non-string map keys;
- mixed array/map tables unless a documented conversion rule selects one;
- functions and coroutines;
- userdata and light userdata;
- threads;
- tables with behavior-bearing metatables;
- non-finite numbers when the receiving format or API forbids them.

Conversion happens before a task is made runnable. The worker therefore never
reads a table belonging to the submitting VM.

The inverse conversion creates fresh tables and strings in the receiving VM.
No registry reference or borrowed pointer crosses the boundary.

`PushForeign` is not the transport contract for this API. Its support for some
wrapped native values does not imply thread safety, lifetime safety, or deep
copying for arbitrary objects.

Applications that need richer payloads may encode them into a byte buffer with
their own schema. Native handle transfer requires a future explicit,
thread-safe handle type and is denied by default.

### Bound API allowlist

An independent Lua VM does not automatically receive the full `hg` namespace.
Its binding profile is explicit and allowlisted.

The initial profile may contain:

- Lua standard libraries already accepted by Harfang's VM policy;
- pure math and immutable/value helpers proven thread-safe;
- task cancellation and progress-query helpers;
- explicitly audited data transforms.

It must exclude by default:

- `Scene` and scene nodes/components;
- `SceneLuaVM`;
- renderer, bgfx, GPU resource, window, and input APIs;
- mutable audio/device APIs;
- backend physics worlds and bodies;
- raw filesystem globals;
- native objects whose ownership or thread contract is not explicit.

An API is not allowlisted merely because its wrapper can be copied. The native
implementation and every object it reaches must be thread-safe for the exact
operation.

The scheduler is not a security sandbox. An allowlist limits accidental misuse
and documents thread safety; it does not make hostile scripts safe inside the
process.

### Modules and `require`

Each session has its own `package.loaded` and Lua globals.

Pure Lua modules may be loaded independently. Native Lua modules must be
audited for:

- mutable process globals;
- thread-local initialization;
- callbacks into main-thread systems;
- allocator assumptions;
- shutdown/finalizer ordering.

Unaudited native modules are rejected from worker sessions. Module search paths
and readers are configured at session creation and do not mutate process-global
search state.

### Cancellation and watchdogs

Cancellation is cooperative. The Lua adapter integrates with Harfang's
execution watchdog support so that a running script periodically checks:

- explicit cancellation;
- an optional wall-clock deadline;
- an optional instruction/time budget supported by the watchdog.

Cancellation raises a controlled Lua error at a hook boundary, captures the
traceback, and marks the invocation `Cancelled`, not `Failed`. Native calls
that block indefinitely cannot be made cancellable by the scheduler and should
not be exposed to worker sessions.

### Completion callbacks

A Lua completion callback belongs to the caller's Lua VM and remains stored
there. The worker publishes only the task identifier and `TaskValue` or error.

The callback is invoked only when the owning thread calls
`PumpCompletions`. Destroying the caller VM first cancels or discards its
pending callbacks without asking a worker to unreference objects in that VM.

Polling is always available, so callbacks are convenience rather than a
required lifetime mechanism.

## Illustrative Lua API

The final surface will follow existing Fabgen conventions. A representative
shape is:

```lua
local scheduler = hg.TaskScheduler({worker_count = 3})
local scripts = hg.LuaTaskService(scheduler)

local session = scripts:CreateSession({
    name = "terrain-analysis",
    binding_profile = "worker-safe"
})

local init = session:LoadSource([[
    function analyze(input)
        local total = 0
        for i, value in ipairs(input.samples) do
            if task.is_cancelled() then
                return nil
            end
            total = total + value
        end
        return {total = total, count = #input.samples}
    end
]])

init:Wait()

local job = session:Call("analyze", {
    samples = {1, 2, 3, 4}
})

while not job:IsComplete() do
    scripts:PumpCompletions()
end

if job:GetStatus() == hg.TaskCompleted then
    local result = job:GetResult()
    print(result.total, result.count)
else
    print(job:GetError())
end

session:Close()
scripts:Shutdown()
scheduler:Shutdown()
```

This example is descriptive. In particular, blocking `Wait` from Lua should
pump only explicitly safe completion work and must not re-enter the caller's
VM through arbitrary callbacks.

The binding should also provide one-shot convenience for coarse jobs:

```lua
local job = scripts:RunSource(source, "entry_point", input)
```

It must not provide this unsafe shape:

```lua
-- Forbidden: the closure belongs to the caller's lua_State.
scheduler:Submit(function() do work() end)
```

## Determinism Contract

The scheduler guarantees deterministic decomposition, not deterministic
execution timing.

For a fixed `ParallelFor` input and configuration:

- chunk bounds and chunk indices are stable;
- each accepted chunk executes at most once;
- group completion observes every member;
- canonical merging can use chunk-index order.

The scheduler does not guarantee:

- worker assignment;
- start or finish order;
- the order of unrelated side effects;
- deterministic racing access to shared state;
- bit-identical floating-point reductions performed in completion order.

Consumers requiring reproducibility must avoid shared writes, use task-local
buffers, and merge in a stable order. This is a consumer policy, supported but
not guessed by the scheduler.

## Memory And Allocation Policy

The scheduler should own capacity-retaining storage for:

- task records;
- ready-queue entries;
- completion records;
- task-group counters;
- worker scratch metadata.

After a recurring workload reaches a stable high-water mark, repeating the
same task graph should perform no scheduler-internal heap allocation.

Large captures and `TaskValue` payloads may allocate from owned storage. Their
bytes and high-water marks must be observable separately from scheduler record
allocation.

Task callables larger than the inline callable storage may allocate. The
inline size and overflow count should be documented and measured.

False sharing should be avoided for frequently written per-worker counters and
queue state. Cache-line padding is an implementation detail to justify with
profiles, not a reason to expose cache concepts in the public API.

## Instrumentation

The scheduler exposes aggregate counters and profiler scopes without enabling
verbose logging in normal builds.

Minimum counters are:

- configured and live worker counts;
- accepted, started, completed, failed, and cancelled tasks;
- ready and completion queue current/high-water depths;
- active and idle workers;
- caller-helped and worker-helped task counts;
- submitted group and `ParallelFor` chunk counts;
- task-record capacity/growth count;
- inline callable overflow count;
- total worker busy and wait time;
- task queue latency and execution-duration histograms;
- affinity-lane backlog and longest wait;
- completion records awaiting owner-thread drain.

Lua-specific counters add:

- live and high-water VM/session counts;
- VM create/init/destroy time;
- invocations by terminal status;
- input/output bytes and conversion time;
- watchdog cancellations and timeouts;
- completion backlog;
- rejected unsafe values/modules/bindings.

Worker threads use stable names through `set_thread_name`, for example
`Harfang Worker 0`. Profiler zones include a consumer-provided task label but
must avoid allocating a formatted string per execution.

Metrics must make these failure modes visible:

- too many workers competing with the render/main thread;
- a serial producer starving workers;
- tasks below useful scheduling grain;
- one long task dominating a group;
- completion callbacks not being pumped;
- Lua VM startup dominating one-shot script time.

## Thread-Safety Rules For Consumers

Submitting a callable does not make captured objects thread-safe.

Every integration must document:

- data read by workers;
- data written by workers;
- ownership during execution;
- the serial commit point;
- cancellation-safe partial state;
- canonical merge order, when relevant;
- the thread on which destruction occurs.

The preferred pattern is:

```text
serial snapshot / ownership handoff
              -> parallel disjoint computation
              -> stable serial merge or commit
```

Worker tasks should not retain references into containers that can reallocate
while work is in flight. Task-local outputs are preferred over locks around a
shared append-only vector.

## Backward Compatibility

The scheduler and Lua task service are additive.

Required compatibility properties are:

- Existing code that does not instantiate them behaves exactly as before.
- `SceneLuaVM` creation, execution order, callback behavior, and bindings are
  unchanged.
- Existing Lua scripts are never moved to workers implicitly.
- Existing scene, rendering, input, audio, Bullet, and Tau APIs keep their
  current thread of execution.
- No process-global pool is created during ordinary Harfang initialization.
- Zero-worker mode provides a deterministic compatibility path without
  background threads.
- Platforms without thread support compile and run using zero-worker mode.
- Public additions do not rename or remove existing Fabgen symbols.
- Scheduler shutdown is complete before libraries needed by tasks are
  unloaded.

The initial implementation must not change Tau or Bullet performance settings
as a side effect. Any future consumer integration is a separate patch with its
own behavior and performance acceptance criteria.

## Platform Requirements

The initial target set is the desktop platforms already supported by Harfang.
The implementation uses C++14 and Harfang's thread helpers.

Platform behavior must include:

- Windows, Linux, and macOS worker creation and naming where supported;
- a zero-worker fallback when threads are unavailable or deliberately
  disabled;
- no mandatory affinity policy;
- no dependence on a platform-specific lock-free primitive;
- clean behavior when a requested worker cannot be created.

If partial worker creation occurs, construction either succeeds with an
explicitly reported lower live count or fails cleanly before accepting work.
It must not silently advertise workers that do not exist.

Web builds without pthread support use synchronous mode. Pthread-enabled web
builds require a separate platform acceptance pass before enabling workers by
default.

## Security And Trust Boundary

Independent Lua execution is an isolation-of-state feature, not a hostile-code
sandbox.

Scripts still execute native code in the Harfang process. Memory use, CPU use,
native modules, and any exposed I/O can affect the application. Watchdogs and
payload limits provide operational bounds but not a security boundary.

Applications running untrusted code require a separate process sandbox and
are outside this specification.

## Testing Plan

### Native unit tests

1. Every accepted task executes exactly once.
2. Zero-worker mode executes successfully and does not create a thread.
3. A group completes only after all members publish terminal state.
4. `ParallelFor` covers every index exactly once with stable chunk bounds.
5. Distinct chunks can execute concurrently.
6. A nested worker wait helps runnable tasks and does not deadlock.
7. Direct self-wait is diagnosed in debug builds.
8. Cancellation before start prevents execution.
9. Running cancellation is observed cooperatively.
10. A failed task does not kill its worker or unrelated tasks.
11. Handle generations reject stale handles.
12. `Drain` shutdown finishes queued work and joins all workers.
13. `CancelPending` shutdown publishes terminal states and joins all workers.
14. Submissions racing with shutdown are either accepted or rejected with a
    defined result, never lost.
15. Completion writes are visible after poll/wait.
16. Retained capacity prevents allocations in a repeated warmed-up task graph.
17. Queue stress exceeds initial capacity without corruption.
18. Normal work makes progress in the presence of background work.
19. Worker names and reported live counts match the created workers.

Run race-enabled tooling such as ThreadSanitizer on supported configurations.
Use long randomized stress runs in addition to deterministic unit cases.

### Lua adapter tests

1. Two sessions overlap execution on two workers using a controlled barrier.
2. Two calls to one session remain FIFO and never overlap.
3. VM creation, use, `LuaObject` cleanup, and destruction occur on its lane.
4. Plain nested data round-trips with integer, number, string, blob, array,
   map, boolean, and nil semantics documented and verified.
5. Cyclic tables, functions, userdata, unsafe metatables, excessive depth, and
   excessive bytes are rejected before dispatch.
6. No Lua stack or registry reference crosses between VMs.
7. Source compilation and runtime errors return owned message/traceback data.
8. Cancellation and watchdog timeout terminate at a safe Lua hook boundary.
9. A completion callback runs only during owner-thread completion pumping.
10. Destroying the caller VM safely discards its callbacks while worker jobs
    complete or cancel.
11. Closing a session with queued calls publishes terminal results and destroys
    the VM exactly once.
12. Repeated create/run/destroy cycles have stable memory use.
13. An unaudited native module or binding profile entry is rejected.
14. Existing `SceneLuaVM` tests and script QAs remain unchanged.

### Integration tests

Provide one native sample and one Lua-facing QA that do useful CPU work without
touching scene or renderer state from workers.

The Lua QA should:

- launch at least two independent persistent sessions;
- exchange `TaskValue` tables;
- display progress/results only after main-thread polling;
- run identically with zero and multiple workers, except for timing;
- shut down explicitly without leaking a VM or worker.

### Performance tests

Measure separately:

- construction and shutdown cost;
- single-task submit-to-start latency;
- throughput of coarse native tasks;
- throughput and queue contention for small tasks;
- `ParallelFor` scaling by grain size and worker count;
- idle CPU usage;
- caller-helping behavior;
- warm recurring workload allocation count;
- one-shot Lua VM startup cost;
- persistent-session Lua call throughput;
- `TaskValue` conversion cost by payload size;
- completion-pump cost and backlog behavior.

The first acceptance target is correctness with low idle and scheduling
overhead. A scheduler microbenchmark win does not justify integrating it into
an engine loop whose tasks are too small or whose data ownership is unsafe.

## Implementation Roadmap

### Phase 0 - Freeze contracts and measurements

- Approve worker-count, shutdown, cancellation, and determinism semantics.
- Add microbenchmark scaffolding and allocation counters.
- Identify platforms that require synchronous fallback.
- Confirm exception-enabled and exception-disabled build behavior.

Exit condition: the state machine and ownership rules have no unresolved
ambiguity.

### Phase 1 - Foundation executor

- Add scheduler, worker, task record, handle, group, shared queue, completion
  queue, and synchronous mode.
- Add stable `ParallelFor` partitioning.
- Implement helping waits, cancellation, and shutdown.
- Integrate thread names and base metrics.
- Complete native unit and stress tests.

Exit condition: native tests pass with 0, 1, and multiple workers, including
sanitizer runs where available.

### Phase 2 - Data and completion boundary

- Add C++14-compatible `TaskValue` and bounded deep-copy conversion helpers.
- Add owner-thread completion mailboxes.
- Test generation-safe results and destruction during shutdown.
- Bind native status/result primitives additively through Fabgen.

Exit condition: native code can submit worker computation and safely publish a
data-only result to the owning thread.

### Phase 3 - Independent Lua sessions

- Add `LuaTaskService` and pinned serialized session lanes.
- Create and destroy each VM on its lane.
- Add restricted binding profiles and module policy.
- Add source/module initialization and named calls.
- Integrate cancellation with Lua watchdog hooks.
- Add polling and owner-thread completion callbacks.
- Add Fabgen bindings and a focused Lua QA/sample.

Exit condition: distinct VMs demonstrate concurrent work; one session remains
serialized; forbidden cross-VM values are rejected; existing Lua behavior is
unchanged.

### Phase 4 - Hardening

- Profile queue contention, allocation, false sharing, and completion backlog.
- Tune grain-size guidance based on measured hardware.
- Validate shutdown during application and VM teardown.
- Run desktop platform CI and thread sanitizers.
- Document worker-safe APIs and binding profiles.

Exit condition: the scheduler can be enabled in a shipping application without
idle CPU burn, leaks, shutdown races, or implicit main-thread API calls.

### Phase 5 - Consumer integrations

Integrate consumers one at a time. Each integration must bring its own:

- ownership diagram;
- serial snapshot and commit boundaries;
- deterministic merge rule;
- task grain threshold;
- correctness QA;
- performance/no-regression matrix.

No Tau consumer is part of the current milestone. A later decision may reuse
the same scheduler for physics, but it must not distort the generic scheduler
API around Tau's internal data structures.

## Acceptance Criteria

The scheduler milestone is accepted when all of the following are true:

- It lives outside Tau, Bullet, asset tooling, renderer, and Lua VM internals.
- Zero-worker mode requires no background thread and passes the full API test
  suite.
- Multiple-worker mode executes native tasks concurrently and shuts down
  cleanly.
- Nested waits cannot deadlock solely because all workers are waiting on work
  queued to the same scheduler.
- Stable `ParallelFor` decomposition supports canonical deterministic merges.
- Task failure and cancellation cannot terminate a worker or disappear.
- A repeated warmed-up native workload has no scheduler-record allocation.
- Independent Lua scripts run concurrently only through distinct VMs.
- Calls within one Lua session are serialized.
- No raw Lua or unaudited Harfang object crosses a worker/VM boundary.
- Lua results and callbacks are delivered on the owning thread.
- Existing `SceneLuaVM`, physics, rendering, and scripting QAs remain
  unchanged.
- No Tau performance or behavior change is bundled into the scheduler patch.

## Open Decisions Before Implementation

These choices should be resolved during Phase 0 without weakening the
invariants above:

1. Whether the first queue is bounded with submission back-pressure or grows
   while recording a high-water mark.
2. Whether owner-thread blocking waits help by default or require an explicit
   `WaitPolicy::Help` at every call site.
3. The inline callable storage size and behavior when it is exceeded.
4. The default background worker formula for application-created schedulers.
5. Whether `TaskValue` maps preserve insertion order or use canonical lexical
   key order at the transport boundary.
6. Which pure/value Harfang APIs form the initial worker-safe Lua binding
   profile.
7. Whether asset/module bytes are always preloaded on the owner thread in
   version 1, or whether a proven thread-safe read provider is accepted.
8. The exact watchdog hook interval and default payload limits.

None of these decisions permits concurrent access to one Lua VM or implicit
worker access to main-thread Harfang systems.

## Recommended First Slice

Implement only Phase 1 first:

- instance-owned native scheduler;
- zero-worker synchronous mode;
- shared queue and fixed workers;
- handles, groups, stable `ParallelFor`, helping waits, cancellation, shutdown;
- counters and comprehensive native tests.

Keep Lua out of that first source patch while retaining the contracts in this
document. The second slice can then add `TaskValue` and the completion boundary,
which are prerequisites for a safe Lua adapter.

This sequence validates the reusable foundation before introducing VM
lifetime, binding-profile, and cross-VM data-conversion complexity.

## Local Sources Reviewed

- `harfang/foundation/thread.h`
- `harfang/platform/win32/thread.cpp`
- `harfang/platform/posix/thread.cpp`
- `harfang/script/lua_vm.h`
- `harfang/script/lua_vm.cpp`
- `harfang/engine/lua_object.h`
- `harfang/engine/lua_object.cpp`
- `harfang/engine/scene_lua_vm.h`
- `harfang/engine/scene_lua_vm.cpp`
- `binding/bind_harfang.py`
- `harfang/engine/scene_bullet3_physics.cpp`
- `specifications/SPECS_TAU_POOL_OF_OBJECTS_PERFORMANCE_ROADMAP.md`

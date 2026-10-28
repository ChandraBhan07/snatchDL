# Architecture

## 1. System Overview

The project is a multithreaded URL collection and downloading application. It monitors the clipboard for URLs, identifies known content services, schedules download jobs, manages cookies, executes downloads through `yt-dlp`, and records job history.

The architecture consists of the following primary components:

- **Clipboard Listener** — Interfaces with the operating system to receive clipboard notifications.
- **Clipboard Worker** — Processes clipboard text, appends it to the input file, and submits incoming work.
- **Job Manager** — Coordinates job processing, scheduling, retry policy, and job history.
- **Downloader** — Executes downloads through child processes, manages download timing, and reports execution results.
- **Cookie Manager** — Loads cookie files, associates them with services, and tracks cookie health and reuse.
- **Service Registry** — Resolves known services by service key or hostname.
- **Thread-Safe Queue** — Provides synchronized communication, blocking waits, notifications, and shutdown behavior.

Supporting components include CLI, Config, Service, Job, Job Builder, JobHistory, Databus, and the `spread` utility.

The architecture separates orchestration from execution. The Job Manager decides which jobs should run and what happens after an attempt, while the Downloader performs the attempt and reports its outcome.

## 2. Component Relationships

The main runtime communication paths are:

1. The Clipboard Listener communicates with the operating system and supplies clipboard events to the clipboard-processing side of the application.
2. The Clipboard Worker submits incoming work through the incoming queue.
3. The Job Manager consumes incoming work and dispatches scheduled jobs through the outgoing queue.
4. The Downloader consumes jobs from the outgoing queue and returns updated jobs through the incoming queue.
5. The Cookie Manager supplies cookies and processes download outcome reports.
6. The Service Registry resolves known service definitions used by job construction and cookie association.
7. JobHistory handles persistent history and duplicate detection.

The incoming queue is shared by the Clipboard Worker and Downloader threads, with the Job Manager acting as the consumer. This makes the Job Manager the central orchestration point even though multiple threads can submit work.

## 3. Startup Configuration

### 3.1 CLI

CLI handles command-line argument parsing, help output, and version information. It keeps command-line syntax and validation separate from the application's runtime components.

### 3.2 Config

Config produces the finalized runtime configuration object, `Config`.

Instead of passing individual configuration values throughout the application, components receive the configuration object and access the values relevant to their responsibilities.

The configuration contains:

|Field|Responsibility|
|---|---|
|`inputFile`|Path to the input URL file.|
|`runMode`|Selects the application's operating mode.|
|`batchSize`|Number of jobs grouped into a download batch.|
|`eachDelay`|Delay between individual downloads.|
|`batchDelay`|Physical pause between batches.|
|`startDelay`|One-time delay before downloading begins.|
|`limit`|Optional download limit.|
|`bandwidth`|Optional bandwidth limit.|

The supported run modes are `Default`, `ListenOnly`, and `DownloadOnly`. Their operational behavior is determined by the application's startup and orchestration logic.

The three download delays are enforced by the Downloader rather than the Job Manager. This keeps execution timing separate from scheduling:

- `eachDelay` separates individual downloads.
- `batchDelay` separates batches.
- `startDelay` delays the beginning of download activity once.

A job's `readyAt` value determines scheduling eligibility. Configured download delays govern execution timing and are not substitutes for job scheduling.

## 4. Job and Ownership

`Job` is the central data structure passed through the job-processing pipeline. Ownership of a Job moves between components as it progresses through the system.

A Job contains:

|Field|Responsibility|
|---|---|
|`state`|Current execution state.|
|`attempts`|Attempt counter.|
|`id`|Identifier used for job identity and history.|
|`url`|URL submitted to the download engine.|
|`serviceKey`|Known service identifier or the reserved `others` key.|
|`readyAt`|Earliest scheduled execution time.|
|`lastAttempt`|Time of the most recent attempt.|

The four states are:

- `RUNNING`
- `WAITING`
- `SUCCEEDED`
- `FAILED`

The Downloader reports the result of an execution attempt. The Job Manager applies retry policy and determines whether further work is required or the job has reached a terminal state.

The precise point at which `attempts` is incremented must remain consistent with the implementation so that retry accounting is not duplicated.

## 5. Job Manager

The Job Manager is the orchestration component between incoming work, scheduling, download execution, and persistence.

Its responsibilities are:

- Consume newly submitted work.
- Coordinate job construction and history checks.
- Schedule eligible jobs.
- Dispatch jobs to the Downloader.
- Receive completed jobs.
- Apply retry limits.
- Handle successful and failed terminal jobs through JobHistory.

The Job Manager does not execute downloads. It determines when work is dispatched and how the application responds to the reported outcome.

### 5.1 Scheduling

The Job Manager assigns scheduling times through `readyAt`. It uses the shared `spread` utility to distribute job times instead of scheduling all jobs at the same instant.

The Downloader uses these times to determine when jobs become eligible for execution.

## 6. Downloader

The Downloader performs the actual download attempt and reports its result to the Job Manager.

It depends on two external executables:

- `yt-dlp` — The download engine.
- `ffmpeg` — Used when merging separate audio and video streams.

The format expression `bv*+ba/best` requests the best available video and audio combination, with a fallback to the best available format.

### 6.1 Child-process management

The Downloader starts `yt-dlp` as a child process and monitors its output and completion.

On Windows, it uses process and pipe mechanisms to capture standard output and standard error. Since `yt-dlp` runs as a command-line tool, interactive standard input is not required.

When the child process completes, the Downloader records its exit code and updates the Job's execution result:

- Successful process completion produces a successful result.
- Unsuccessful process completion produces a failed result.

The updated Job is returned through the incoming queue.

The Downloader does not decide whether to retry. Retry policy belongs to the Job Manager.

### 6.2 Download timing and batching

The Downloader applies the configured `startDelay`, `eachDelay`, and `batchDelay` during execution.

It also handles batch formation according to `batchSize`.

These mechanisms regulate download execution without artificially delaying job scheduling in the Job Manager.

### 6.3 Blocking and scheduled-time waits

The Downloader uses two distinct waiting mechanisms.

**Outgoing queue wait**

The Downloader waits for work on `outgoingQueue`. When the Job Manager enqueues a job, queue notification wakes a waiting consumer so it can attempt to retrieve that job.

**Scheduled-time wait**

A job may already be queued but not yet eligible for execution. The Downloader uses a condition variable and `cv.wait_until()` to wait for the relevant `readyAt` time.

A minimum-priority queue keeps the earliest scheduled job at the top. This prevents the Downloader from waiting for a later job while an earlier job is ready to run.

These mechanisms avoid busy-waiting and reduce unnecessary CPU consumption.

## 7. Databus and Thread-Safe Queue

### 7.1 Databus

`Databus` is the bidirectional communication structure between the Job Manager and Downloader. It contains two queues:

|Queue|Producer|Consumer|Purpose|
|---|---|---|---|
|`outgoingQueue`|Job Manager|Downloader|Transfers jobs for execution.|
|`incomingQueue`|Clipboard Worker and Downloader|Job Manager|Transfers newly submitted work and completed job results.|

Queue names are defined from the Job Manager's perspective.

### 7.2 Thread-safe queue

The queue implementation protects concurrent operations with locking and condition-variable mechanisms.

Its important behaviors are:

- Synchronized enqueue and pop operations.
- Blocking waits when work is unavailable.
- Notifications when new work is enqueued.
- A stop mechanism that wakes blocked consumers.

The queue's `wait()` operation returns a Boolean indicating whether work is available according to its contract. Consumers then attempt to retrieve an item using `tryPop()`.

The `incomingQueue` is shared by multiple producer threads. Its synchronization ensures that the Clipboard Worker and Downloader threads can submit work while the Job Manager consumes it.

When a queue is stopped, condition-variable notifications such as `cv.notify_all()` wake blocked consumers so they can exit instead of waiting indefinitely.

The exact distinction between an empty queue and a stopped queue must follow the queue's actual return-value and stop semantics.

## 8. Service

A `Service` represents a known content provider, such as YouTube, Instagram, or Twitter.

It contains:

- `name` — Human-readable service name.
- `key` — Stable, hardcoded service identifier.
- `hosts` — Known hostnames and subdomains.
- `cookies` — Cookie information associated with the service.
- `extractIds()` — Extracts a content identifier from a service URL.
- `makeUrls()` — Constructs a canonical URL from a content identifier.

### 8.1 Service keys

Each known service has a unique, hardcoded key. Examples include `yt`, `ig`, and `tw`.

Service keys are used throughout the project for service lookup, cookie association, job identity, and history organization.

They are stable identifiers rather than display names.

### 8.2 Identifier extraction and canonical URLs

`extractIds()` extracts the meaningful content identifier from a known service's URL.

`makeUrls()` constructs a canonical download URL from that identifier.

This design avoids repeatedly storing URLs that differ only in tracking parameters or other irrelevant details. History can record a service key and content identifier, and the application can reconstruct a canonical URL when needed.

This also improves duplicate detection by allowing different URL forms referring to the same content to share the same logical identity.

## 9. Service Registry

The Service Registry stores actual Service objects in an unordered map keyed by service key.

Its principal lookup functions are:

- `findByKey()` — Returns a pointer to a Service using its key.
- `findByHost()` — Resolves a hostname, including recognized subdomains, to a Service.

The Registry provides centralized lookup while leaving service-specific behavior inside the Service objects themselves.

The distinction is that a Service defines provider-specific data and rules, while the Registry finds the appropriate Service.

## 10. Cookie Manager

The Cookie Manager loads cookie files, associates them with known services, tracks their health, and controls their reuse.

### 10.1 Loading and service association

Cookie files are placed in `data/cookies/` and must use Netscape cookie-file format.

The Cookie Manager reads these files, identifies the corresponding known service, and associates them with the appropriate Service through the Service Registry.

There is currently no separate mechanism that validates whether a cookie is authenticated, whether its credentials have expired, or whether the service will accept it. Successful loading and service association do not guarantee successful authentication.

### 10.2 Cookie health

Each cookie starts with a health value of `100`.

The current policy is:

- Each use reduces health by 10 points.
- Successful downloads restore a small amount of health.
- Failed downloads reduce health further.
- Health reaching zero marks the cookie as dead and removes it from the active cookie lists.

The Downloader reports outcomes through the Cookie Manager. The Cookie Manager owns health accounting and removal decisions.

The precise recovery increment, failure decrement, and reporting semantics are implementation-specific.

### 10.3 Resting delays

Cookies have resting delays that control how frequently they are reused.

A successful download receives a shorter rest than a failed download. The Cookie Manager uses `spread` to produce randomized delay values within supplied minimum and maximum bounds.

Cookie health and resting delays are reuse controls, not guarantees of authentication or immunity to service-side detection.

Cookie management is an evolving component, and its selection, health, and timing policies may be refined independently of the job pipeline.

## 11. The `spread` Utility

`spread` is a shared utility for generating randomized values within a supplied minimum and maximum range.

Its implementation divides the range into five slots and selects a value according to its slot-based distribution strategy.

The purpose is to spread selected timing values rather than allowing every generated value to cluster arbitrarily close together.

It is used by:

- **Job Manager** — To distribute job `readyAt` values.
- **Cookie Manager** — To generate cookie resting delays.

Both components reuse the same utility rather than maintaining separate randomization implementations.

The exact slot boundaries and behavior for small or uneven ranges are determined by the implementation.

## 12. Job Builder and Unknown Services

The Job Builder converts a URL into a Job appropriate for the service it belongs to.

For known services, job construction uses the corresponding service key and extracted content identifier.

For URLs that do not belong to a known service, the application uses a reserved key:

`others`

This key is not registered as a normal Service. It allows generic URLs to enter the same job pipeline without creating arbitrary Service objects for unlisted providers.

For an unknown URL, the Job Builder constructs a Job using:

- `serviceKey = "others"`
- `id = Url::discardFragment(parts)`
- `url = id`

Removing the fragment produces a normalized URL used as both the identifier and download URL.

The Downloader then attempts the URL through `yt-dlp`. The download succeeds or fails according to the tool's capabilities and the source's accessibility.

## 13. JobHistory

JobHistory owns persistent job-history file operations and duplicate detection.

The current result directories are:

- `logs/failed/`
- `logs/succeeded/`

JobHistory handles the associated file reading and writing, keeping persistence separate from scheduling and download execution.

For known services, the service key and content identifier are used to identify jobs consistently. This allows URL variations representing the same content to be recognized as duplicates.

The `others` category uses the normalized URL as its identifier.

JobHistory determines whether incoming work should be accepted or rejected according to the recorded history. The Job Manager coordinates those decisions with scheduling and terminal-result handling.

## 14. Shutdown Architecture

Shutdown combines operating-system signal handling, queue termination, and explicit component stopping.

### 14.1 OS integration

The main thread initializes the shutdown mechanism, starts the application components and their threads, and then waits for a shutdown request.

On Windows:

- `SetConsoleCtrlHandler()` registers the console control handler.
- `WaitForSingleObject()` blocks the main thread until the shutdown event is signaled.

On Linux:

- `sigprocmask()` configures signal handling.
- `sigwait()` blocks until the relevant signal is received.

`initialize()` performs platform-specific shutdown initialization, while `awaitShutdown()` blocks the main thread until shutdown is requested.

### 14.2 Coordinated stopping

When shutdown is requested, the application closes the pipelines first. The queue stop mechanism notifies blocked consumers, allowing waiting threads to detect shutdown and return.

The application then explicitly calls the Downloader and Listener stop methods.

The Job Manager does not require an independent manual stop call because its queue-processing loop is released by the stopped incoming queue.

The Downloader normally waits for an ongoing download to finish, avoiding unnecessary interruption of active work. A second Ctrl+C can trigger a more immediate shutdown path that interrupts the active download.

The exact ordering of thread joins and resource destruction must remain consistent with the implementation to prevent threads from accessing components that have already been destroyed.

## 15. Responsibility Boundaries

|Component|Primary responsibility|
|---|---|
|CLI|Argument parsing, help, and version information.|
|Config|Finalized runtime configuration.|
|Clipboard Listener|Operating-system clipboard integration.|
|Clipboard Worker|Clipboard text processing, input-file appending, and work submission.|
|Job Builder|Converts URLs into known-service or generic Jobs.|
|Service|Provider-specific identifiers, hosts, cookies, and URL construction.|
|Service Registry|Lookup of known services by key and host.|
|Cookie Manager|Cookie loading, association, health, and resting delays.|
|Job|Represents a unit of work and its execution state.|
|Job Manager|Scheduling, retries, orchestration, and history integration.|
|Downloader|Child-process execution, batches, timing, and execution results.|
|Databus|Connects the Job Manager and Downloader through two queues.|
|Thread-Safe Queue|Synchronization, blocking waits, notifications, and stopping.|
|JobHistory|History-file I/O and duplicate detection.|
|`spread`|Shared randomized timing generation.|

The central architectural boundary is between orchestration and execution: the Job Manager decides what should happen next, while the Downloader performs the current attempt and reports what happened.

Service resolution, cookie management, history persistence, and thread synchronization remain separate responsibilities, allowing their internal implementations to evolve without collapsing the application into a single control flow.
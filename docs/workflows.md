# Workflows

This document describes how data moves through the application, from clipboard collection to download execution and final job-history recording.

The system is composed of loosely coupled components that communicate through queues. The pipeline describes the flow of work between components; the thread-safe queue is the concrete mechanism that implements that flow.

The central data structure is `Job`. It travels through the application, carrying its identity, URL, scheduling information, attempt count, and execution state.

## 1. System Workflow Overview

The application has three closely related runtime flows:

- **Clipboard and job ingestion:** Clipboard events are processed, URLs are built into jobs, and duplicate or invalid work is filtered before entering the job-processing pipeline.
- **Job scheduling and execution:** The Job Manager schedules jobs and places them into the outgoing queue. The Downloader independently consumes available work, executes downloads, and returns updated jobs through the incoming queue.
- **Result handling and persistence:** The Job Manager receives updated jobs, applies retry policy, and coordinates terminal-result recording through JobHistory.

The components communicate through defined interfaces and queues. The Job Manager does not need to know about `yt-dlp`, `ProcessRunner`, or the Downloader's internal scheduling and batching mechanisms. Likewise, the Downloader does not need to know the Job Manager's retry policy.

## 2. Clipboard Flow

The clipboard workflow begins with the operating system and ends with work entering the incoming queue. It also involves Job Builder and JobHistory, which help determine how submitted URLs become acceptable jobs.

**_Source diagram:_** [`diagrams/clipboardflow.mmd`](diagrams/clipboardflow.mmd)

### 2.1 Listener and Worker

The Clipboard Listener interfaces with the operating system and installs the required hooks to receive clipboard notifications. It is responsible for detecting clipboard activity, not for building jobs or executing downloads.

The Listener Worker processes the clipboard text. Its primary responsibilities are:

1. Read and process clipboard text.
2. Append the text to the configured input file.
3. Submit discovered URL work through the incoming queue.

This separation keeps operating-system integration distinct from application-level text processing.

### 2.2 Job construction and history

The incoming work is processed through the job-ingestion path, which uses Job Builder and JobHistory.

Job Builder determines how a URL should be represented as a Job. JobHistory supplies the history and duplicate-detection functionality used to determine whether work should be accepted.

Accepted work is submitted through the incoming queue for the Job Manager to consume. Rejected work does not proceed as a new download job.

The Listener Worker does not need to understand the Job Manager's scheduling policy, and the Job Manager does not need to know how clipboard notifications are generated.

For the complete relationship between these components, refer to `clipboardflow.mmd`.

## 3. Job Building, Filtering, and Rejection

Job Builder converts submitted URLs into the application's central `Job` representation. It distinguishes between known services and URLs that do not match a registered service.

**_Source diagram:_** [`diagrams/jobBuilder.mmd`](diagrams/jobBuilder.mmd)

### 3.1 Known services

For a recognized service, the Service Registry resolves the service using its key or hostname.

The associated Service provides two important functions:

- `extractIds()` extracts the content identifier from a recognized URL.
- `makeUrls()` reconstructs a canonical URL from that identifier.

The resulting Job uses the stable service key and extracted identifier. This avoids treating tracking parameters and other irrelevant URL variations as separate content items.

### 3.2 Unknown services

URLs that do not match a known service use the reserved key `others`. This key is not registered as a normal Service.

The Job Builder creates the Job using:

- `serviceKey = "others"`
- `id = Url::discardFragment(parts)`
- `url = id`

The URL fragment is removed, and the normalized URL becomes both the identifier and the download URL.

This allows generic URLs to enter the same downstream pipeline without creating arbitrary Service objects.

### 3.3 Filtering and rejection

Job building and job acceptance are distinct responsibilities.

Job Builder constructs the Job representation. JobHistory provides the persistence and duplicate-detection operations used by the ingestion path to decide whether the work should be accepted.

A URL rejected as a duplicate should not be submitted as a new download job. Accepted work proceeds through the incoming queue.

The exact validation and rejection conditions are defined by the implementation and are detailed in the Job Builder flowchart.

## 4. Job Manager Workflow

The Job Manager consumes incoming work, performs scheduling, and places eligible jobs into the outgoing queue. It also consumes completed jobs from the incoming queue and applies retry and terminal-result handling.

**_Source diagram:_** [`diagrams/jobmanager.mmd`](diagrams/jobmanager.mmd)

### 4.1 Receiving incoming work

The Job Manager waits on the incoming queue. This queue carries both newly submitted work and updated Jobs returned after download attempts.

When work arrives, the Job Manager processes it according to the job's current state and the applicable history and scheduling rules.

The Job Manager is the central orchestration component, but it is not the owner of every downstream operation. It does not directly invoke the Downloader or control its internal execution.

### 4.2 Scheduling jobs

For jobs that require execution, the Job Manager determines their scheduling times and uses `spread` to distribute `readyAt` values across the relevant range.

Once a job is ready to be dispatched, the Job Manager places it into the outgoing queue.

That enqueue operation is the boundary between scheduling and execution. The Job Manager does not need to know which Downloader thread consumes the job, how the Downloader batches work, or how a child process is started.

The outgoing queue makes the job available to its consumers through the queue's synchronization and notification mechanisms.

### 4.3 Handling completed jobs

When a Job returns through the incoming queue, the Job Manager examines its state and attempt count.

- A successful job proceeds to successful terminal-result handling.
- A failed job with retries remaining can be scheduled for another attempt.
- A failed job that has exhausted its retry allowance proceeds to failed terminal-result handling.

JobHistory handles the relevant persistence operations.

The Downloader reports the outcome of an attempt; the Job Manager determines what should happen next.

### 4.4 Blocking behavior

The Job Manager waits for incoming work instead of repeatedly polling an empty queue.

When a producer enqueues work, the thread-safe queue notifies waiting consumers. When the queue is stopped, the stop mechanism wakes blocked consumers so they can exit.

The Job Manager's control flow therefore depends on the queue's contract rather than on direct calls from the Listener Worker or Downloader.

## 5. Job Flow Between Job Manager and Downloader

The Job Manager and Downloader communicate through `Databus`, which contains the concrete incoming and outgoing queues.

**_Source diagram:_** [`diagrams/jobflow.mmd`](diagrams/jobflow.mmd)

### 5.1 The two queues

The queue names are defined from the Job Manager's perspective.

|Queue|Producers|Consumer|Purpose|
|---|---|---|---|
|`outgoingQueue`|Job Manager|Downloader|Carries jobs ready for execution.|
|`incomingQueue`|Clipboard Worker and Downloader threads|Job Manager|Carries newly submitted work and updated job results.|

The pipeline is the logical flow between these components. The queues and their synchronization mechanisms implement the actual communication.

### 5.2 Job ownership and movement

A typical Job follows this path:

1. The ingestion path builds and accepts the Job.
2. The Job enters the incoming queue.
3. The Job Manager receives it and performs the required scheduling.
4. The Job Manager enqueues it into the outgoing queue when it is ready to be dispatched.
5. The Downloader consumes the Job and performs its execution workflow.
6. The Downloader updates the Job's execution result and returns it through the incoming queue.
7. The Job Manager consumes the updated Job and applies retry or terminal-result handling.

Ownership moves with the Job as it progresses through the pipeline. The queues do not implement the job policy; they transport work safely between concurrent components.

### 5.3 Loose coupling

The Job Manager and Downloader do not need direct knowledge of each other's internal implementations.

The Job Manager knows that it can enqueue a Job and later receive a result. It does not know how the Downloader's priority queue, batching logic, cookie handling, or process execution work.

The Downloader knows how to consume a Job and return an updated result. It does not know how the Job Manager decides which jobs to schedule or whether a failed job should be retried.

This separation allows either component's internal implementation to evolve while preserving the queue-based communication contract.

## 6. Downloader Stage 1: Preparation, Scheduling, and Batching

The Downloader has two internal stages. Stage 1 prepares and organizes jobs for execution; Stage 2 performs the actual download.

**_Source diagram:_** [`diagrams/downloaderS1.mmd`](diagrams/downloaderS1.mmd)

Stage 1 is responsible for preparing work before the actual download operation.

### 6.1 Receiving work

The Downloader waits on the outgoing queue for new jobs. When the Job Manager enqueues a Job, queue notification allows the Downloader to attempt to retrieve it.

This wait is independent of the Downloader's scheduled-time wait. Receiving a job does not necessarily mean it is ready to execute immediately.

### 6.2 Priority queue and `readyAt`

The Downloader maintains a minimum-priority queue organized around job readiness. The job with the earliest `readyAt` time is at the top.

If the next job is not yet ready, the Downloader uses a condition variable and `cv.wait_until()` to wait until the relevant scheduled time rather than repeatedly checking the clock.

The priority queue is important because the Downloader must not wait for a later job's deadline while an earlier job is already eligible.

The queue-arrival wait and scheduled-time wait serve different purposes:

- The outgoing queue wait detects the arrival of work.
- The condition-variable wait enforces scheduled eligibility.

Together, they avoid unnecessary polling and CPU consumption.

### 6.3 Batch formation

Stage 1 also handles batch preparation according to the configured `batchSize`.

Batching determines how jobs are grouped for execution. It is separate from the Job Manager's responsibility for assigning `readyAt` values and dispatching work through the outgoing queue.

The configured delays are applied at the Downloader level:

- `startDelay` delays the beginning of download activity once.
- `eachDelay` separates individual downloads.
- `batchDelay` introduces a physical pause between batches.

These settings control execution timing rather than job scheduling.

The precise order in which jobs leave the priority queue, enter batches, and become eligible for Stage 2 is represented in `downloaderS1.mmd`.

## 7. Downloader Stage 2: Actual Download Execution

Stage 2 performs the actual download operation. It prepares the `yt-dlp` command, executes it through ProcessRunner, and uses the returned process exit code to determine the Job's execution result.

**_Source diagram:_** [`diagrams/downloaderS2.mmd`](diagrams/downloaderS2.mmd)


### 7.1 Preparing the download command

The Downloader prepares the command-line arguments required to execute `yt-dlp` for the current Job.

The command uses the Job's URL and the applicable download configuration, including any relevant cookie file, format selection, and configured download limits.

The application relies on `yt-dlp` as its primary download engine and `ffmpeg` when separate audio and video streams must be merged.

The format-selection expression `bv*+ba/best` requests the best available video and audio combination, with a fallback to the best available format.

Command construction belongs to the Downloader. ProcessRunner is responsible for executing the prepared command through operating-system process mechanisms.

### 7.2 ProcessRunner

ProcessRunner is the small, OS-facing component that starts and monitors the child process.

It handles the operating-system-specific process creation and the associated pipes used to capture standard output and standard error.

The Downloader does not need interactive standard input because `yt-dlp` is operated as a command-line tool.

ProcessRunner waits for the child process to finish and returns its exit code as an integer.

This is the execution boundary between the Downloader's application-level logic and the operating system's process machinery.

### 7.3 Interpreting the exit code

The Downloader receives the integer exit code returned by ProcessRunner and uses it to determine the Job's execution result.

- A successful exit code results in `Job::state` being set to `SUCCEEDED`.
- An unsuccessful exit code results in `Job::state` being set to `FAILED`.

The Downloader also updates the attempt-related information according to the implementation and returns the updated Job through the incoming queue.

The exit code communicates the child process's result. Retry policy remains outside ProcessRunner and belongs to the Job Manager.

### 7.4 Reporting cookie outcomes

When a cookie is used, the Downloader reports the outcome through the Cookie Manager's reporting interface.

- Successful downloads use `reportSuccess()`.
- Applicable non-terminal failures use `reportSoftFailure()`.
- The Cookie Manager handles hard-failure removal internally when cookie health reaches zero.

The Downloader reports what happened without taking ownership of the Cookie Manager's health and selection policy.

### 7.5 Returning the Job

After execution and result reporting, the Downloader enqueues the updated Job into the incoming queue.

The Downloader's responsibility is to execute the attempt, update the execution result, and return the Job. It does not decide whether the Job should be retried or permanently recorded as failed.

The Job Manager receives the returned Job and continues the orchestration workflow.

## 8. Cookie Lifecycle

Cookie management operates alongside download execution rather than replacing the job pipeline.

Cookie files are loaded from `data/cookies/` and must use Netscape cookie-file format. The Cookie Manager associates recognized cookie files with the appropriate known Service through the Service Registry.

The current implementation does not independently validate whether a cookie contains valid authenticated credentials or whether it has expired.

### 8.1 Health and resting delays

Each cookie starts with a health value of `100`.

- Every use reduces health by 10 points.
- A successful download restores a small amount of health.
- A failed download reduces health further.
- Health reaching zero marks the cookie as dead and removes it from the active cookie lists.

The Cookie Manager also assigns resting delays. Successful downloads receive shorter rests than failed downloads.

The `spread` utility generates randomized delay values within the supplied minimum and maximum range. Its slot-based distribution is shared with job scheduling, avoiding separate timing-randomization implementations.

Cookie health and resting delays influence reuse, but do not guarantee that a cookie remains authenticated or will be accepted by a service.

## 9. Job History and Terminal Results

JobHistory handles the file operations and duplicate detection associated with job history.

The current result directories are:

- `logs/succeeded/`
- `logs/failed/`

For known services, the service key and extracted content identifier are used to identify jobs consistently. For unknown services, the normalized URL is used as the ID under the `others` category.

JobHistory participates in two points of the workflow:

1. **Admission:** The ingestion path uses history to determine whether incoming work should be accepted or rejected as a duplicate.
2. **Completion:** The Job Manager coordinates the recording of terminal successful and failed jobs.

JobHistory owns file-reading and file-writing operations. The Job Manager coordinates when history checks and terminal-result recording occur.

This keeps persistence separate from scheduling and download execution.

## 10. Shutdown Workflow

Shutdown uses operating-system mechanisms to notify the main thread, followed by queue termination and explicit component stopping.

### 10.1 Waiting for shutdown

On Windows, the application uses `SetConsoleCtrlHandler()` to register console control handling and `WaitForSingleObject()` to block the main thread until the shutdown event is signaled.

On Linux, it uses `sigprocmask()` to configure signal handling and `sigwait()` to block until the relevant signal is received.

The `initialize()` method configures the shutdown mechanism, while `awaitShutdown()` blocks the main thread until shutdown is requested.

### 10.2 Stopping the pipelines

When the main thread resumes from the shutdown wait:

1. The application closes the pipelines by stopping their underlying queues.
2. The queue stop mechanism notifies blocked consumers using condition-variable notifications such as `cv.notify_all()`.
3. Waiting consumers detect the stopped queue and return.
4. The application explicitly invokes the Downloader's stop method.
5. The application explicitly invokes the Listener's stop method.
6. The relevant threads finish according to their component-specific shutdown behavior.

The Job Manager does not require a separate manual `stop()` call because it waits on the incoming queue and exits when the stopped queue releases it.

The Downloader and Listener are explicitly stopped because they have their own execution and operating-system responsibilities.

### 10.3 Active downloads and repeated Ctrl+C

During graceful shutdown, the Downloader normally waits for an ongoing download to finish before stopping. This avoids unnecessarily interrupting active work.

If Ctrl+C is pressed a second time, the application can take a more immediate path that interrupts the active download.

The queue stop mechanism and the explicit component stop methods work together: stopping the queues releases blocked consumers, while the Downloader's stop behavior determines how active process execution is handled.

## 11. End-to-End Workflow

The complete workflow can be understood as a sequence of loosely coupled operations:

1. **Initialize:** CLI and Config establish the runtime configuration; services, cookies, queues, history, and worker components are initialized.
2. **Collect:** The Listener receives clipboard events, and the Worker processes clipboard text and appends it to the input file.
3. **Build:** Job Builder constructs a Job for a known service or assigns the `others` key to a generic URL.
4. **Filter:** The ingestion path uses JobHistory to reject duplicate work and admit accepted jobs.
5. **Submit:** Accepted work enters the incoming queue.
6. **Schedule:** The Job Manager assigns scheduling times and places eligible jobs into the outgoing queue.
7. **Prepare:** The Downloader's Stage 1 handles readiness, priority ordering, and batching.
8. **Execute:** Stage 2 prepares the `yt-dlp` command and delegates child-process execution to ProcessRunner.
9. **Report:** ProcessRunner returns an integer exit code; the Downloader updates the Job's execution state, reports cookie outcomes, and submits the Job to the incoming queue.
10. **Decide:** The Job Manager applies retry policy and coordinates terminal-result recording through JobHistory.
11. **Repeat or stop:** The pipeline continues until work is complete or shutdown is requested.

The essential principle is that each component performs its own responsibility and communicates through defined interfaces. The queues transport work; the Job Manager owns scheduling and retry decisions; the Downloader owns download execution; ProcessRunner owns OS-level process execution; and JobHistory owns persistent history operations.
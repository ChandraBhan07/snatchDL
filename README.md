# Snatch DL

**Snatch DL** is a terminal-based download orchestrator built on top of [yt-dlp](https://github.com/yt-dlp). It automatically monitors clipboard links, queues downloads, and intelligently manages cookies to ensure smooth, efficient downloading. Designed for flexibility and extensibility, Snatch DL provides structured logging, configurable settings, and multi-threaded job management—making it ideal for automating media downloads from supported platforms.

For more information about the underlying engine, visit the [yt-dlp repository](https://github.com/yt-dlp).



## Motivation / Why

Many existing download wrappers are either simple process spawners that quickly exhaust cookies, or paid GUI-based solutions built on open-source engine. I wanted a lightweight, efficient alternative that:

- **Manages downloads intelligently** with proper job scheduling.
- **Keeps thorough logs** for both successful and failed downloads.
- **Supports configuration** via a `config.json` for flexibility.
- **Includes a cookie manager** that respects rate limits, tracks cookie health, and ensures robust retries.

The goal was to build a solution that combines the reliability of a managed engine with the transparency and extensibility of open source, while remaining simple to use and extend.



## Features

- **Clipboard Monitoring:** Automatically detects copied links in real-time and adds them to the download queue.
- **Multi-Threaded Job Management:** Efficiently handles multiple downloads concurrently with a dedicated job manager.
- **Intelligent Retry System:** Jobs are retried automatically, first without cookies, then with cookies if needed, while logging failures separately.
- **Cookie Health Management:** Tracks cookie usage, assigns cooldowns for failures, and avoids using dead cookies.
- **Structured Logging:** Maintains separate logs for successful and failed downloads per source, enabling easy tracking and debugging.
- **Configurable Setup:** Uses a `config.json` for paths, retries, worker threads, and other parameters.
- **Graceful Shutdown:** Supports Ctrl+C or terminal close events to safely stop all threads and listeners without losing progress.
- **Known Sources Supported:** Currently supports YouTube, Instagram, and Twitter, with extensible support for additional platforms.



## Installation, Requirements & Usage

**Snatch DL** is a terminal-based application. It **must be run from the command line (CMD / PowerShell)**. 

### Requirements

- **Operating System:** Windows (tested on Windows 10/11)
- **Compiler:** Modern C++ compiler (C++17 or newer, e.g., MSVC, MinGW)
- **Dependencies:**
  - [yt-dlp](https://github.com/yt-dlp) (included in `data/others`)
  - Standard C++ libraries
- **Optional:** Netscape-format cookies for sites that require authentication

------

### Usage / Running the Program

Snatch DL monitors clipboard links and manages queued downloads automatically. To run the program, provide a links file (can be empty) as an argument:

```
app.exe links.txt
```

------

### Installation Options

#### Option 1: Build from Source (Technical Users)

1. Clone the repository:

   ```
   git clone https://github.com/ChandraBhan07/snatch-dl.git
   cd snatch-dl
   ```

2. Open the project in your preferred IDE (Visual Studio recommended).

3. Build the solution (`Release` recommended).

4. Run `app.exe` from the command line with a links file:

   ```
   app.exe links.txt
   ```

------

#### Option 2: Precompiled Zip (For Easy Use)

1. Download the latest release ZIP from the Releases page or click here [Download Latest Release](https://github.com/ChandraBhan07/snatchDL/releases/latest).

2. Extract the ZIP to any folder.

3. Ensure `yt-dlp.exe` is present in `data/others`.

4. Optionally, add cookies in the `data/cookies` folder.

5. Run `app.exe` from the extracted folder using the command line:

   ```
   app.exe links.txt
   ```

> ⚠️ **Note:** Running `app.exe` by double-clicking in Explorer **will not work**. Use CMD / PowerShell to provide arguments and see logs.



## Architecture / How it Works

**Snatch DL** is designed as a modular, multi-threaded terminal application that orchestrates downloads efficiently while handling clipboard links, cookies, and job queues.

### 1. Core Components

- **Clipboard Listener**
   Continuously monitors the system clipboard for new links. Once a valid link is detected, it forwards it to the parser and job manager.
  - Runs on a dedicated thread.
  - Gracefully shuts down on Ctrl+C.
- **Parser**
   Validates URLs, removes duplicates, and manages a set of seen links.
  - Ensures only new, valid links are queued for download.
  - Appends processed links to log files.
- **Job Manager**
   Handles queued download jobs in a thread-safe manner.
  - Each job has retry logic and a ready timestamp.
  - Worker threads acquire jobs when ready, respecting `maxRetries`.
  - Uses **condition variables** to efficiently wait without busy-waiting.
- **Downloader**
   Wraps `yt-dlp` to perform actual downloads, optionally using cookies.
  - Returns success or failure codes.
  - Supports automatic retry for failed downloads.
- **Cookie Manager**
   Maintains a pool of Netscape-format cookies per source.
  - Handles soft and hard failures.
  - Rate-limits usage to avoid bans.
- **Logger**
   Records success and failure of downloads in structured log files.
  - Logs are later used to avoid duplicate downloads.

### 2. Threading & Synchronization

- Worker threads (`worker_d`) fetch jobs from the job manager and perform downloads.
- Clipboard thread (`worker_l`) waits for new clipboard content.
- **Condition variables** (`std::condition_variable`) are used to:
  - Make threads wait efficiently for new jobs or shutdown events.
  - Wake threads immediately on Ctrl+C or when new jobs are added.
- **Atomic flags** (`std::atomic<bool>`) are used for graceful shutdown signals.

### 3. Shutdown Flow

- Ctrl+C triggers a console control handler.
- Atomic flags are set to `false`, notifying all threads using condition variables.
- Clipboard listener and job manager gracefully stop, releasing resources.
- Main thread joins all worker threads to ensure clean exit.

### 4. Data Flow

```
Clipboard → Parser → Job Manager → Worker Threads → Downloader → Logger
       ↘ Seen Links Log ↗
```

- New links detected in clipboard are parsed and checked against logs.
- Valid new links are added to the job queue.
- Worker threads process queued jobs using available cookies.
- Results are logged for future reference and duplicate prevention.

## Known Sources

**Snatch DL** currently supports multiple sources with source-specific handling to ensure proper cookie management, logging, and rate limiting.

### Supported Sources

- **Instagram (`ig`)**
- **YouTube (`yt`)**
- **Twitter (`tw`)**
- **Facebook (`fb`)**
- **Vimeo (`vm`)** etc

### Source-Specific Handling

1. **Cookies**
   - Each source maintains a dedicated pool of Netscape-format cookies.
   - Worker threads acquire cookies from the pool when needed, respecting rate limits to avoid bans.
   - Soft and hard failures are tracked per cookie to prevent repeated failed attempts.
2. **Logs**
   - Each source maintains separate log files for successful and failed downloads.
   - Logs are used to prevent duplicate downloads and to resume interrupted sessions seamlessly.
   - Format is consistent across sources, making it easy to track job history per source.
3. **Download Behavior**
   - Jobs are queued independently but processed with awareness of source-specific limitations.
   - Cookies and retries are applied per source, ensuring robust handling of authentication and throttling rules.

This design ensures that multiple sources can be processed in parallel while maintaining source-specific reliability and avoiding conflicts.



## Configuration (`config.json`)

Snatch DL uses a `config.json` file to allow users to customize paths, jobs, and cookie handling.

### Example `config.json`

```
{
  "paths": {
    "logs": "logs",
    "downloads": "data/downloads",
    "cookies": "data/cookies",
    "yt_dlp_path": "data/others/yt-dlp.exe"
  },
  "jobs": {
    "max_retries": 3,
    "worker_threads": 1
  },
  "cookies": {
    "initial_pool_per_source": 2
  }
}
```

### Configurable Sections

1. **Paths**
   - `logs`: Directory where success/failure logs are stored.
   - `downloads`: Directory where downloaded files are saved.
   - `cookies`: Directory containing Netscape-format cookies for authenticated downloads.
   - `yt_dlp_path`: Path to the `yt-dlp.exe` executable.
2. **Jobs**
   - `max_retries`: Maximum number of download attempts per job.
   - `worker_threads`: Number of concurrent worker threads for processing jobs.
3. **Cookies**
   - `initial_pool_per_source`: Number of cookies preloaded per source to start downloading.

### Notes

- All paths can be relative to the application directory or absolute.
- Increasing `worker_threads` can speed up downloads but may hit rate limits depending on source.
- Adjust `initial_pool_per_source` according to the number of cookies you have available to avoid exhausting them too quickly.

## Graceful Shutdown & Ctrl+C Handling

Snatch DL is designed to handle interruptions smoothly, ensuring no downloads or jobs are left in an inconsistent state.

### Key Features

1. **Ctrl+C or Console Close**
   - Pressing **Ctrl+C**, closing the terminal, or shutting down the system triggers a graceful shutdown.
   - The application immediately stops monitoring the clipboard and prevents new jobs from being added.
2. **Worker Thread Coordination**
   - All active download threads check a shared atomic flag (`g_running`) to determine if shutdown has been requested.
   - Threads that are waiting on jobs or sleeping are woken up using condition variables (`std::condition_variable`) and exit cleanly.
   - This prevents the need to press Ctrl+C multiple times, unlike previous versions.
3. **Resource Cleanup**
   - Clipboard listeners are stopped and windows handles are cleaned up.
   - The job queue is shut down safely, preventing new jobs from being processed.
   - All threads are joined, ensuring memory is released and no dangling processes remain.

### How It Works (Internally)

- A global atomic flag (`g_running`) indicates if the application should continue running.
- Worker threads use `condition_variable::wait` or `wait_until` to sleep until either:
  - A new job is available.
  - Shutdown is requested.
- When a shutdown is requested, `g_running` is set to `false` and all condition variables are notified, waking threads immediately.
- Threads detect the shutdown flag, complete any ongoing tasks safely, and exit.

**Outcome:** Users experience an immediate, clean exit without leaving partial downloads, dangling threads, or zombie resources.





## Contributing

Snatch DL is designed to be modular and developer-friendly. Contributions are welcome, whether it’s bug fixes, feature enhancements, or support for new sources.

### How to Contribute

1. **Fork the repository** and create a new branch for your feature or fix.
2. Make sure your changes follow the project’s coding style and are well-tested.
3. Submit a pull request with a clear description of your changes.

### Starter Ideas for Contributors

- **Cookie Manager Enhancements**
  - Improve cookie allocation per source.
  - Add automatic refreshing of expired cookies.
  - Support additional authentication formats.
- **New Sources & Formats**
  - Add support for additional platforms (e.g., TikTok, Vimeo).
  - Implement source-specific URL parsing, download logic, and cookie handling.
- **Logging & Metrics Improvements**
  - Track download statistics, retry counts, and cookie usage more comprehensively.
  - Optionally integrate with external monitoring tools.
- **Cross-Platform Support**
  - Replace Windows-specific clipboard and console handling to run on Linux/macOS.

> **Note:** Currently, the main focus is on improving the cookie manager for more reliable and intelligent downloads. Contributors can help accelerate this development.



> ## License & Disclaimer
>
> **Snatch DL** is an open-source project licensed under the **MIT License**. You are free to use, modify, and distribute this software in accordance with the license, as long as the original copyright notice and license are included.
>
> **Ownership:**
>  This project and all its source code remain the intellectual property of the original author. Attribution is appreciated when using or extending this project.
>
> **Disclaimer:**
>  Snatch DL is a **wrapper tool around [yt-dlp](https://github.com/yt-dlp)**. It is intended **for personal use and educational purposes only**. The author **does not promote or condone the unauthorized downloading of copyrighted content**. Users are solely responsible for complying with applicable laws and the terms of service of the content sources they access. The author is **not liable** for any misuse of this software.

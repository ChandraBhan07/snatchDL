#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include <condition_variable>
#include <mutex>

#include "link.h"
#include "parser.h"
#include "job.h"
#include "jobManager.h"
#include "cookieManager.h"
#include "downloader.h"
#include "logger.h"
#include "clipboardListener.h"
#include "config.h"
#include "fileManager.h"
#include "source.h"
#include "commandRunner.h"
#include "windows.h"
#include "cmdOptions.h"

std::atomic<bool> g_running{true};
std::condition_variable cv;
BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT ||
        signal == CTRL_BREAK_EVENT ||
        signal == CTRL_CLOSE_EVENT ||
        signal == CTRL_SHUTDOWN_EVENT)
    {
        std::cout << "\nCtrl+C detected. Shutting down...\n";
        g_running = false;
        cv.notify_all();
        return TRUE; // handle flag
    }
    return FALSE;
}

void worker_l(ClipboardListener &listener, std::unordered_set<std::string> &seen_id, JobManager &jobManager, Parser &parser, std::vector<Link> &pending, std::string &filename, Downloader &downloader, std::atomic<int> &counter)
{
    while (g_running)
    {
        auto text = listener.waitForChange(); // blocks until new clipboard content

        if (!text.empty() && parser.isHttpsUrl(text))

            if (parser.addLinkIfNew(text, seen_id, pending))
            {
                const auto &link = pending.back();
                std::cout << "\n[New Link Detected] Source: "
                          << link.source << " | ID: " << link.id << "\n";

                Job job(link.id, link.source);
                jobManager.add_job(job);

                std::string url = Source::instance().getCanonicalForm(link.source) + "/" + link.id;
                parser.appendToFile(url, filename);
                counter++;
                std::cout << "Pending : " << counter << "\n";
            }
    }
}

void worker_d(JobManager &jobManager, Downloader &downloader, CookieManager &cookieManager, Logger &logger, std::atomic<int> &counter)
{
    while (g_running)
    {
        auto job = jobManager.acquire_job();
        if (!job) break;

        Cookie *cookie = nullptr;
        if (job->attempts >= 1)
        {
            cookie = cookieManager.acquire(job->source);
        }

        DWORD result = downloader.processLink(job->id, job->source, cookie);

        if (result == 0)
        {
            if (cookie)
                cookieManager.report_success(*cookie);
            logger.success(job->source, job->id);
            jobManager.mark_success(job);
            counter--;
        }
        else
        {
            job->attempts++;
            jobManager.mark_failure(job);

            if (job->attempts >= jobManager.get_max_retries())
            {
                if (cookie)
                    cookieManager.report_hard_failure(*cookie);
                logger.failure(job->source, job->id);
                counter--;
            }
            else
            {
                if (cookie)
                    cookieManager.report_soft_failure(*cookie);
            }
        }
        std::cout << "Pending : " << counter << "\n";
    }
    cv.notify_all();
}

int main(int argc, char *argv[])
{
    // ----- Console Control Handler - Windows ----
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    try
    {

        CmdOptions options = CmdOptions::parse(argc, argv);

        if (options.show_help())
        {
            std::cout << "Usage: app.exe [filename] [--nolisten]\n";
            return 0;
        }

        // -------------- Config --------------
        ConfigData defaultConfig{
            {"logs", "data/downloads", "data/cookies", "data/others/yt-dlp.exe"},
            {3, 1},
            {2}};

        Config config;
        auto exeDir = FileManager::instance().getExeDir();
        config.handle_config(exeDir, defaultConfig);

        // -------------- Parser --------------
        Parser parser;

        if (options.filename().empty())
        {
            std::cerr << "Usage: app.exe <file-to-read.txt>\n";
            return 1;
        }

        std::string filename = options.filename();
        parser.validate_input_file(filename);

        std::unordered_set<std::string> seen_id;
        std::cout << "\nParsing input file ... \n";

        parser.load_logs(exeDir / config.data.paths.logs / "succeed", seen_id);
        parser.load_logs(exeDir / config.data.paths.logs / "failed", seen_id);

        std::vector<Link> pending = parser.compute(filename, seen_id);
        std::atomic<int> counter = pending.size();
        std::cout << "Pending : " << counter << "\n";

        // ---------------- Jobs ----------------
        int maxRetries = config.data.jobs.max_retries; // total attempts
        JobManager jobManager(maxRetries, options.listen());

        for (auto &link : pending)
        {
            Job job(link.id, link.source);
            jobManager.add_job(job);
        }

        // ---------------- Cookie Manager ----------------
        CookieManager cookieManager;

        std::cout << "\nScanning cookies ... \n";
        cookieManager.scanCookies(exeDir / config.data.paths.cookie);

        int res = cookieManager.getPoolSize();
        std::cout << "Cookie pool initialized, size " << res << "\n";
        if (res == 0)
            std::cout << "No cookies found. Add netscape style cookies in " << config.data.paths.cookie << " directory.\n";

        // ---------------- Downloader ----------------
        Downloader downloader(exeDir / config.data.paths.downloads, exeDir / config.data.paths.yt_dlp);

        // ------------------ Logger ------------------
        Logger logger(exeDir / config.data.paths.logs);

        // ---------------- Worker Threads ----------------
        std::vector<std::thread> workers;

        int num_workers = config.data.jobs.worker_threads;

        for (size_t i = 0; i < num_workers; ++i)
        {
            workers.emplace_back(worker_d, std::ref(jobManager), std::ref(downloader), std::ref(cookieManager), std::ref(logger), std::ref(counter));
        }

        // -------------- Clipboard Listener --------------
        ClipboardListener listener;

        if (options.listen())
        {
            listener.start();
            std::cout << "\nListening for clipboard events ...\nPass --nolisten to stop listening to clipboard\n";
            workers.emplace_back(worker_l, std::ref(listener), std::ref(seen_id), std::ref(jobManager), std::ref(parser), std::ref(pending), std::ref(filename), std::ref(downloader), std::ref(counter));
        }

        // ----------------Graceful Shutdown ----------------
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]
                {
                    if(!options.listen()){
                        if(counter == 0) return true;
                    }
                    return !g_running;
                });

        listener.stop();
        jobManager.shutdown();

        for (auto &t : workers)
            t.join();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

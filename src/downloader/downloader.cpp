#include "downloader.h"
#include "processRunner.h"
#include "../runtime/threadSafeQueue.h"
#include "../config/config.h"
#include "../cookie/cookie.h"
#include "../cookie/cookieManager.h"

#include <filesystem>
#include <iostream>

void Downloader::init()
{
    std::string cmd;
    if (std::filesystem::is_empty(config.ytDlpDir))
    {
        cmd = "yt-dlp --version";
        ytDlpSourceType = YtDlpSourceType::System;
    }

    else
    {
        cmd = config.ytDlpDir.string() + "/yt-dlp --version";
        ytDlpSourceType = YtDlpSourceType::Local;
    }

    int errorcode = ProcessRunner::run(cmd);

    // cerr and not exception as this runs in thread and requires separate try catch
    if (errorcode != 0)
    {
        std::cerr << "yt-dlp can't be used. Install it or add to PATH.\n";
        std::exit(EXIT_FAILURE);
    }

    std::cout << "yt-dlp found, proceeding ..\n";
}

void Downloader::run()
{

    if (config.startDelay.count())
    {
        std::unique_lock<std::mutex> lock(mtx);

        std::cout << "Waiting for: start-delay to expire. \n";

        cv.wait_for(lock, config.startDelay, [this]
                    { return !running.load(); });

        config.expireStartDelay();
    }

    const std::size_t batchSize = config.batchSize;
    size_t batchitr = 0;

    while (outgoingQueue.wait())
    {
        std::unique_lock<std::mutex> lock(mtx);

        // batchSize == 0 => infinite batching
        size_t count = 0;

        while (running.load() && !outgoingQueue.empty() && (batchSize == 0 || count < batchSize))
        {
            auto job = outgoingQueue.tryPop();
            if (!job)
                break;

            queue.push(std::move(*job));
            ++count;
        }

        cv.wait_for(lock, config.batchDelay, [this]
                    { return !running.load(); });

        std::cout << "Proceeding with batch #" << ++batchitr << '\n';

        // process priority queue
        while (running.load() && !queue.empty())
        {
            std::cout << "Pending jobs: " << queue.size() << '\n';

            Job job = queue.top();

            if (job.isReady())
            {
                std::cout << "Downloading job: " << job.id << "\n";
                queue.pop();
                job.state = Job::State::RUNNING;

                std::string cmd = ytDlpSourceType == YtDlpSourceType::System ? "yt-dlp " : config.ytDlpDir.string() + "/yt-dlp ";

                Cookie *cookie = nullptr;

                if (job.attempts > 1)
                    cookie = cookieManager.acquire(job.serviceKey);

                if (cookie)
                {
                    std::cout << "Applying cookie " << cookie->path.filename() << "\n";
                    cmd += " --cookies \"" + cookie->path.string() + "\" ";
                }

                // // ALWAYS best video + best audio, fallback safe
                cmd += "-f \"bv*+ba/best\" "; // requires ffmpeg

                // limit bandwidth
                if (!config.limitRate.empty())
                    cmd += "--limit-rate " + config.limitRate;

                // Let yt-dlp choose best extraction method per site
                cmd += " --concurrent-fragments 4 ";

                // Output naming
                cmd += "-o \"" +
                       (config.downloadDir / job.serviceKey).string() + "/%(channel)s %(id)s.%(ext)s" +
                       "\" ";

                // cmd += "-q --progress --no-warnings ";
                cmd += "--no-warnings ";

                cmd += job.url;

                // std::cout << cmd << "\n";

                int errorcode = ProcessRunner::run(cmd, true);
                // std::cout << "runner spawned and exit code = " << errorcode << "\n";

                if (errorcode != 0)
                {
                    job.state = Job::State::FAILED;
                    std::cout << "Job failed: " << job.id << "\n";
                    if (cookie)
                        cookieManager.reportSoftFailure(cookie);
                }
                else
                {
                    job.state = Job::State::SUCCEEDED;
                    std::cout << "Job succeeded: " << job.id << "\n";
                    if (cookie)
                        cookieManager.reportSuccess(cookie);
                }
                std::cout << "\n\n";

                job.attempts++;

                incomingQueue.enqueue(std::move(job));
            }

            else
                cv.wait_until(lock, job.readyAt, [this]
                              { return !running.load(); });

            cv.wait_for(lock, config.eachDelay, [this]
                        { return !running.load(); });
        }

        // delete this later
        cookieManager.resetCookieStates();
    }
}

void Downloader::stop()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        running.store(false);
    }

    cv.notify_all();
}
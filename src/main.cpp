#include "cli/cliOptions.h"
#include "runtime/threadSafeQueue.h"
#include "service/serviceRegistry.h"
#include "input/inputFile.h"

#include "job/job.h"
#include "job/jobHistory.h"
#include "job/jobBuilder.h"
#include "job/jobManager.h"
#include "listener/listener.h"
#include "cookie/cookieManager.h"
#include "downloader/downloader.h"
#include "listener/listenerWorker.h"
#include "shutdown/shutdown.h"

#include <thread>
#include <iostream>

// data bus is named wrt job manager and downloader
struct DataBus
{
    ThreadSafeQueue<Job> incomingQueue;
    ThreadSafeQueue<Job> outgoingQueue;
};

int main(int argc, char *argv[])
{
    try
    {
        const CliOptions options = CliOptions::parse(argc, argv);

        // syntantic checks
        if (options.showHelp)
        {
            CliOptions::printHelp();
            return 0;
        }
        if (options.showVersion)
        {
            CliOptions::printVersion();
            return 0;
        }
        if (options.errorState == ErrorState::Fatal)
        {
            std::cerr << options.errorMessage << '\n';
            return 1;
        }

        if (options.errorState == ErrorState::Warning)
            std::cout << options.errorMessage << '\n';

        // semantic checks
        FinalizeResult result = options.config.finalize();
        if (result.state == ErrorState::Fatal)
        {
            std::cerr << result.message << "\n";
            return 1;
        }

        if (result.state == ErrorState::Warning)
            std::cerr << result.message << "\n";

        if (options.config.mode == RunMode::Default)
            std::cout << "Default mode, both listening and downloading\n";

        if (options.config.mode == RunMode::ListenOnly)
            std::cout << "Listen only mode\n";

        if (options.config.mode == RunMode::DownloadOnly)
            std::cout << "Download only mode.\n";

        if (options.config.mode != RunMode::ListenOnly)
        {
            if (options.config.startDelay.count())
                std::cout << "Downloader start delay: " << options.config.startDelay.count() << "s\n";

            if (options.config.batchSize > 0)
                std::cout << "Batch size: " << options.config.batchSize << "\n";

            if (options.config.batchDelay.count())
                std::cout << "Batch additional delay: " << options.config.batchDelay.count() << "s\n";

            if (options.config.eachDelay.count())
                std::cout << "Each additional delay: " << options.config.eachDelay.count() << "s\n";
        }

        DataBus databus;
        ServiceRegistry registry;

        InputFile inputFile(options.config.inputFile);
        inputFile.read();

        JobHistory history(options.config.logsDir);
        history.loadLogs();
        std::cout << "Loading logs ...\n";

        JobBuilder jobBuilder(registry, history);
        for (auto text : inputFile.getContent())
        {
            auto job = jobBuilder.build(text);

            if (!job)
                continue;

            databus.incomingQueue.enqueue(std::move(*job));
        }

        CookieManager cmanager(registry, options.config.cookiesDir);
        cmanager.scanCookies();

        ThreadSafeQueue<std::string> listenerQueue;

        // mode = default           => jobmanager + listener + downloader
        // mode = --listen-only     => listener
        // mode = --download-only   => jobmanager + downloader

        JobManager jobManager(databus.incomingQueue, databus.outgoingQueue, history, options.config.startDelay);

        Listener listener(listenerQueue); // setup
        ListenerWorker listenerWorker(listenerQueue, databus.incomingQueue, jobBuilder, inputFile);
        Downloader downloader(databus.incomingQueue, databus.outgoingQueue, options.config, cmanager);

        std::vector<std::thread> threads;

        if (options.config.mode == RunMode::Default)
        {
            downloader.init();
            threads.emplace_back(&JobManager::run, &jobManager);
            threads.emplace_back(&Listener::run, &listener);

            threads.emplace_back(&ListenerWorker::run, &listenerWorker);
            threads.emplace_back(&Downloader::run, &downloader);
        }

        else if (options.config.mode == RunMode::ListenOnly)
        {
            threads.emplace_back(&Listener::run, &listener);
            threads.emplace_back(&ListenerWorker::run, &listenerWorker);
        }

        else if (options.config.mode == RunMode::DownloadOnly)
        {
            downloader.init();
            threads.emplace_back(&JobManager::run, &jobManager);
            threads.emplace_back(&Downloader::run, &downloader);
        }

        Shutdown::initialize();

        // Waiting for shutdown
        Shutdown::awaitShutdown();

        std::cout << "Ctrl+C detected, gracefully shutting down ...\n";

        listenerQueue.stop();
        databus.incomingQueue.stop();
        databus.outgoingQueue.stop();

        std::cout << "All queues stopped\n";

        downloader.stop();
        std::cout << "Downloader stopped\n";
        listener.stop();
        std::cout << "Listener stopped\n";

        // if (options.config.mode == RunMode::Default)
        // {
        //     downloader.stop();
        //     listener.stop();
        // }

        // else if (options.config.mode == RunMode::ListenOnly)
        //     listener.stop();

        // else if (options.config.mode == RunMode::DownloadOnly)
        //     downloader.stop();

        for (auto &thread : threads)
        {
            thread.join();
        }
    }

    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
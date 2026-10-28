#include "listenerWorker.h"
#include "../runtime/threadSafeQueue.h"
#include "../job/job.h"
#include "../job/jobBuilder.h"
#include "../input/inputFile.h"

#include <iostream>

void ListenerWorker::run()
{
    while (listenerQueue.wait())
    {
        auto text = listenerQueue.tryPop();
        if (!text)
            continue;

        auto job = jobBuilder.build(*text);

        if (!job)
            continue;

        std::cout << "New link added: " << job->serviceKey << " " << job->id << "\n";

        inputFile.append(*text); // preserve original url
        incomingQueue.enqueue(std::move(*job));
    }
}

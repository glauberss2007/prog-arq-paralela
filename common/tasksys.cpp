#include "tasksys.h"
#include <pthread.h>
#include <vector>
#include <stdio.h>

struct ThreadData {
    IRunnable* runnable;
    int startIdx;
    int endIdx;
};

void* threadFunc(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->startIdx; i < data->endIdx; i++) {
        data->runnable->run();
    }
    return NULL;
}

TaskSystem::TaskSystem(int numThreads) : numThreads(numThreads) {}

TaskSystem::~TaskSystem() {}

void TaskSystem::run(IRunnable* runnable, int numTasks) {
    if (numThreads == 1 || numTasks == 1) {
        for (int i = 0; i < numTasks; i++) {
            runnable->run();
        }
        return;
    }

    std::vector<pthread_t> threads(numThreads);
    std::vector<ThreadData> threadData(numThreads);
    
    int tasksPerThread = numTasks / numThreads;
    int remainingTasks = numTasks % numThreads;
    
    int startIdx = 0;
    for (int i = 0; i < numThreads; i++) {
        int endIdx = startIdx + tasksPerThread + (i < remainingTasks ? 1 : 0);
        
        threadData[i].runnable = runnable;
        threadData[i].startIdx = startIdx;
        threadData[i].endIdx = endIdx;
        
        pthread_create(&threads[i], NULL, threadFunc, &threadData[i]);
        startIdx = endIdx;
    }
    
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

TaskGroup* TaskSystem::createTaskGroup(IRunnable* runnable, int numTasks) {
    // Implementação simplificada - retorna nullptr pois não é usado pelo ISPC
    return nullptr;
}

TaskSystem* TaskSystem::create(int numThreads) {
    return new TaskSystem(numThreads);
}
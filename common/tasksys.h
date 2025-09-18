#ifndef __TASKSYS_H__
#define __TASKSYS_H__

class IRunnable {
public:
    virtual void run() = 0;
    virtual ~IRunnable() {}
};

class TaskGroup {
public:
    TaskGroup() {}
    virtual ~TaskGroup() {}
    virtual void wait() = 0;
};

class TaskSystem {
public:
    TaskSystem(int numThreads);
    virtual ~TaskSystem();
    
    virtual void run(IRunnable* runnable, int numTasks);
    virtual TaskGroup* createTaskGroup(IRunnable* runnable, int numTasks);
    
    static TaskSystem* create(int numThreads);
    
protected:
    int numThreads;
};

#endif
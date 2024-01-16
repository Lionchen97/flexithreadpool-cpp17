#include "threadpool.h"
///////////////////////////// 线程池类型方法实现
// 线程池构造
ThreadPool::ThreadPool() : initThreadSize_(0), taskSize_(0), taskQueMaxThreshHold_(TASK_MAX_THRESHOLD), threadSizeThreshHold_(THREAD_MAX_THRESHHOLD), curThreadSize_(0), poolMode_(PoolMode::MODE_FIXED), isPoolRunning_(false), threadIdelSize_(0)
{}

// 线程池析构
ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;
    // 等待线程池中所有的线程返回（系统线程：阻塞&正在运行中）
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    // 唤醒所有阻塞的线程
    notEmpty_.notify_all();
    // 主线程阻塞
    exitCond_.wait(lock, [&]() -> bool
                   { return threads_.size() == 0; });
}

// 设置线程池的工作模式
void ThreadPool::setPoolMode(PoolMode mode)
{
    if (checkRunningState())
        return;
    poolMode_ = mode;
}

// 设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(size_t threshhold)
{

    if (checkRunningState())
        return;
    taskQueMaxThreshHold_ = threshhold;
}

// 设置线程池cached模式下的线程上限阈值
void ThreadPool::setThreadMaxThreshHold(size_t threshhold)
{
    {
        if (checkRunningState())
            return;
        if (poolMode_ == PoolMode::MODE_CACHED)
        {
            threadSizeThreshHold_ = threshhold;
        }
    }
}

// 指定初始化线程数量，并开启线程池
void ThreadPool::start(size_t initThreadSize )
{
 // 设置线程池的运行状态
        isPoolRunning_ = true;
        // 记录初始线程个数
        initThreadSize_ = initThreadSize;
        curThreadSize_ = initThreadSize;
        // 创建线程对象的时候，把线程函数给到thread线程对象
        for (int i = 0; i < initThreadSize_; i++)
        {
            // 创建新线程
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadHandler, this, std::placeholders::_1)); // C++14
            // std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadHandler, this))); // C++11
            size_t threadId = ptr->getId();
            threads_.emplace(threadId, std::move(ptr));
            // threads_.emplace_back(std::move(ptr)); //  emplace_back会进行拷贝，而unique_ptr不支持拷贝
        }

        // 启动所有对象
        for (int i = 0; i < initThreadSize_; i++)
        {
            threads_[i]->start(); // 需要执行一个线程函数
            threadIdelSize_++;    // 记录初始空闲线程的数量
        }
}

// 定义线程函数
void ThreadPool::threadHandler(size_t threadId)
{
     auto lastTime = std::chrono::high_resolution_clock().now();
        // 所有任务必须执行完成，线程池才可以回收所有线程资源
        for (;;)
        {
            Task task; 
            {

                /*
                线程池析构时：主线程先notify，再获取锁
                    1.线程在执行任务，执行完毕后，线程函数执行isRunning_状态检查然后析构并notify主线程（不会死锁）
                    2.线程在等待任务，主线程notify可以唤醒线程，让线程函数执行isRunning_状态检查然后析构并notify主线程（不会死锁）
                    3.线程非上述情况，刚进入线程函数循环体。（线程会发生死锁，和析构的主线程争夺同一把锁）
                        1)如主线程先获得锁，此时线程至少有一个没被析构，主线程进入wait状态，释放锁；由于线程没有任务，也会进入wait状态（状态2.会困在while中不断地wait_for），此时就没有线程进行notify主线程
                        2)线程池线程先获得锁，线程无论在状态1.还是状态2.都会进入等待（状态2.会困在while中不断地wait_for）并不会析构，导致主线程一直也在wait状态。
                线程池析构时：主线程先获取锁，再notify
                    以上状态都不会发生死锁
                */
                std::unique_lock<std::mutex> lock(taskQueMtx_);

                std::cout << "tid: " << std::this_thread::get_id() << " try to get the task ..." << std::endl;
                // cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该回收多余的线程。
                // 当前时间-上次线程执行时间
                while (taskQue_.size() == 0)
                {
                    // 线程池结束
                    if (!isPoolRunning_)
                    {
                        threads_.erase(threadId); // 自己生成的线程id
                        std::cout << "threadid: " << std::this_thread::get_id() << " exit!" << std::endl;
                        exitCond_.notify_all(); // 通知主线程
                        return; // 结束线程函数，即结束当前线程
                    }
                    // MODE_CACHED模式：开始回收空闲线程
                    if (poolMode_ == PoolMode::MODE_CACHED)
                    {
                        // 超时返回
                        if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                        {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if (dur.count() >= THREAD_IDLE_TIME && curThreadSize_ > initThreadSize_)
                            {
                                // 把线程对象从线程列表容器中删除
                                threads_.erase(threadId); // 自己生成的线程id
                                // 记录线程数量的相关变量的值修改
                                curThreadSize_--;
                                threadIdelSize_--;
                                std::cout << "空闲线程："
                                          << "threadid: " << std::this_thread::get_id() << " exit!" << std::endl;
                                return;
                            }
                        }
                    }
                    else
                    {
                        notEmpty_.wait(lock); // 最终问题点
                    }
                }

                threadIdelSize_--;
                std::cout << "tid: " << std::this_thread::get_id() << " got the task.." << std::endl;
                // 从任务队列中取一个任务
                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;

                // 如果依然有剩余任务，继续通知其他线程执行任务
                if (taskQue_.size() > 0)
                {
                    notEmpty_.notify_all();
                }
                // 取出一个任务，进行通知，任务队列不为满，可以继续提交生产任务
                notFull_.notify_all();
            } // 让锁释放

            // 当前线程负责执行这个任务，不能等任务执行完才释放锁
            if (task != nullptr)
            {
                // task->run();
                //  执行任务，并把任务的返回值给setVal

                task();
            }
            threadIdelSize_++;
            lastTime = std::chrono::high_resolution_clock().now(); // 更新线程执行完任务的时间
        }
}

// 检查线程池的运行状态
bool ThreadPool::checkRunningState() const
{
     return isPoolRunning_;
}

///////////////////////////// 线程类型方法实现
// 线程构造函数
Thread::Thread(ThreadFunc func): func_(func), threadId_(generateId_++)
{}

// 启动线程
void Thread::start()
{
    /*  创建一个线程来执行线程函数 线程池的所有线程从任务队列里面消费任务
    线程对象t 线程函数func_(C++11)pthread_detach(Linux) */
    std::thread t(func_, threadId_);

    /*  出了作用域线程对象会析构，为了保护线程线程函数，需要设置为分离线程 pthread_detach (Linux) 如果需要回收线程的pcb需要主线程执行join */
    t.detach();
}
// 获取线程id
size_t Thread::getId() const
{
      return threadId_;
}
size_t Thread::generateId_ = 0;
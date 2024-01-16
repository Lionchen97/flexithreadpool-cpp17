// #ifdef THREADPOOL_H
// #define THREADPOOL_H
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <future>
#include <functional>
#include <typeinfo>
////////////////////////////////////// 线程池方法实现
const size_t TASK_MAX_THRESHOLD =2;
const size_t THREAD_MAX_THRESHHOLD = 1024;
const size_t THREAD_IDLE_TIME = 2; // 单位：秒
// 线程池支持的模式
enum class PoolMode // C++防止不同枚举类型，但是枚举项同名
{
    MODE_FIXED,  // 固定数量的线程
    MODE_CACHED, // 线程数量可动态增长
};

// 线程类型
class Thread
{
public:
    // 定义线程函数对象类型
    using ThreadFunc = std::function<void(size_t)>;
    // 线程构造函数
    Thread(ThreadFunc func);
    // 线程析构函数
    ~Thread() = default;
    // 启动线程
    void start();
    // 获取线程id
    size_t getId() const;
private:
    ThreadFunc func_;
    static size_t generateId_; // 自定义线程id
    size_t threadId_;          // 保存线程id
};

/*
example:
ThreadPool pool;
pool.start(4);

int sum1(int a,int b){
   
    return a+b;
}
int sum2(int a,int b,int c)
{
    return a+b+c;
}

pool.submitTask(sum1,1,2);
pool.submitTask(sum2,1,2,3);
*/

// 线程池类型
class ThreadPool
{
public:
    // 线程池构造
    ThreadPool();
    // 线程池析构
    ~ThreadPool();

    // 设置线程池的工作模式
    void setPoolMode(PoolMode mode);
    
    // 设置task任务队列上限阈值
    void setTaskQueMaxThreshHold(size_t threshhold);
   
    // 设置线程池cached模式下的线程上限阈值
    void setThreadMaxThreshHold(size_t threshhold);
    

    // 给线程池提交任务
    // 使用可变参模板编程，让 submitTask可以接受任意任务函数和任意数量的参数
    template <typename Func, typename... Args>
    auto submitTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {

        using RType = decltype(func(args...)); // 推导并获取func函数的返回类型
        // RRype()不带参数是由于使用bind绑定了函数对象和参数  make_shared(Args&&... args)
        auto task = std::make_shared<std::packaged_task<RType()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<RType> result = task->get_future();
        // 获取锁
        std::unique_lock<std::mutex> lock(taskQueMtx_);
        /*
        满足1：线程的通信 等待任务队列有空余
            wait 等待，直到满足条件。
            写法1：
            while(taskQue_.size()==taskQueMaxThreshHold_)
            {
                notFull_.wait(lock); // 阻塞状态
            }
            写法2：
            notFull_.wait(lock,[&]()->bool{return taskQue_.size() <taskQueMaxThreshHold_;});
        满足2：用户提交任务，最长不能阻塞超过1s，否则判断任务提交失败，返回
            wait_for 最多等待一段时间 wait_until等待到一个时间点，且都有返回值。
        */
        if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]() -> bool
                               { return taskQue_.size() < taskQueMaxThreshHold_; }))
        {
            // false，表示not_Full_等待1s钟，条件依然没有满足
            std::cerr << "task queue is full, sumbit task failed." << std::endl;
            // return task->getResult(); // 不允许的操作，因为线程函数执行完任务，任务就被析构了
            auto task  = std::make_shared<std::packaged_task<RType()>> ([]()->RType {return RType();});
            (*task)();
            return task->get_future();
        }

        /* 如果有空余，把任务放入任务队列中
           taskQue_.emplace(sp);
           using Task = std::function<void()>;
           封装执行任务的lambda表达式 */
        taskQue_.emplace([task](){(*task)(); });
        taskSize_++;

        // 因为放了新任务，任务队列不为空，在notEmpty_上通知，赶快分配执行任务。
        notEmpty_.notify_all();

        /* 需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
            cached模式 任务处理比较紧急 场景：小而快的任务*/
        if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > threadIdelSize_ && curThreadSize_ < threadSizeThreshHold_)
        {
            std::cout << ">>> create new thread..." << std::endl;
            // 创建新的线程对象
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadHandler, this, std::placeholders::_1)); // C++14
            size_t threadId = ptr->getId();
            threads_.emplace(threadId, std::move(ptr));
            // 启动新增的线程
            threads_[threadId]->start();
            // 修改线程个数相关的变量
            curThreadSize_++;
            threadIdelSize_++;
        }

        // 返回任务的Result对象
        std::cout << "successful submition !" << std::endl;
        return result;
    }

    // 指定初始化线程数量，并开启线程池
    void start(size_t initThreadSize = std::thread::hardware_concurrency());

    // 禁止拷贝、赋值
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    // 定义线程函数
    void threadHandler(size_t threadId);
    // 检查线程池的运行状态
    bool checkRunningState() const;
    // std::vector<std::unique_ptr<Thread>> threads_; // 线程列表
    std::unordered_map<size_t, std::unique_ptr<Thread>> threads_; // 线程列表
    size_t initThreadSize_;                                       // 初始的线程数量（无符号整形）
    std::atomic_size_t curThreadSize_;                            // 记录当前线程池中线程的总数量
    std::atomic_size_t threadIdelSize_;                           // 记录空闲线程数量
    size_t threadSizeThreshHold_;                                 // 线程数量上限阈值
    // Task 任务 =》 函数对象
    using Task = std::function<void()>;
    std::queue<Task> taskQue_;         // 任务队列(（实际上封装了执行任务的lambda表达式）
    std::atomic_size_t taskSize_;      // 任务的数量（无符号原子整形）
    size_t taskQueMaxThreshHold_;      // 任务队列数量的上限阈值
    std::mutex taskQueMtx_;            // 保证任务队列的线程安全
    std::condition_variable notFull_;  // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空
    std::condition_variable exitCond_; // 等待线程资源全部回收
    PoolMode poolMode_;                // 当前线程池的工作模式
    std::atomic_bool isPoolRunning_;       // 表示当前线程池的启动状态
};

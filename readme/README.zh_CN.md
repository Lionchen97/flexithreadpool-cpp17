# FlexiThreadPool

## 项目介绍

FlexiThreadPool是一个高度灵活且功能丰富的 C++ 线程池库，它为并发任务处理提供了一个高效、可扩展的解决方案，特别适用于需要处理大量异步任务的场景。用户可以自定义创建线程、管理线程，并让它处理任何你想实现的任务。

## 运行环境
- C++标准：C++20
- GNU：8.5及以上
- CMake：2.8及以上
- 操作系统: Linux/Win

## 知识背景

- 熟练基于C++ 11标准的面向对象编程 

  组合和继承、继承多态、STL容器、智能指针、函数对象、绑定器、可变参模板编程等。

- 熟悉C++11多线程编程

   thread、mutex、atomic、condition_variable、unique_lock等。

- C++17和C++20标准的内容

   C++17的any类型和C++20的信号量semaphore。
- 熟悉多线程理论 

  多线程基本知识、线程互斥、线程同步、原子操作、CAS等。

## 主要特点

- **可配置模式**：支持固定模式（`MODE_FIXED`）和缓存模式（`MODE_CACHED`），允许根据应用需求灵活配置线程池行为。
- **动态任务处理**：通过一个队列管理提交的任务，提供线程安全的任务提交和执行机制。
- **异步结果获取**：任务提交者可以获取一个 `future` 对象，用于异步获取任务执行结果。
- **资源管理优化**：自动回收长时间空闲的线程，优化资源使用。
- **线程同步**：使用条件变量和互斥量实现线程间的同步。
- **通用结果存储**：通过可变参模板，用户可以更方便的提交任意任务，同时支持返回不同类型的任务结果。
- **跨平台兼容性**：项目构建配置（通过 CMake）兼容 Linux 和 Windows，确保在不同操作系统上的可移植性。

## 技术挑战

- 实现高效的线程同步机制，确保线程安全。
- 设计通用的结果返回机制，使其能够适应不同类型的任务。
- 管理动态增长的线程和任务队列，优化性能和资源利用。

## 技术细节


### 线程池类（ThreadPool）

- 提供灵活的配置选项，如设置任务队列的最大阈值和线程的最大数量。
- 使用std::unique_ptr智能指针管理线程对象，自动处理资源释放。
- 使用条件变量来同步任务提交和任务执行，优化性能。

### 线程类（Thread）
- 每个线程有一个唯一ID，便于管理和调试。
- 线程通过分离的方式运行，避免了主线程需要等待子线程结束的问题。

### 任务提交
- 通过submitTask函数，线程池可以接受不同签名的函数和参数。
- 使用std::packaged_task封装任务，允许异步获取任务结果。

### 线程调度与管理
- 在缓存模式下，根据当前任务量和线程空闲状态动态调整线程数量。
- 使用原子操作和互斥锁来保证多线程环境下数据的一致性和线程安全。
### 资源与异常管理

- 在任务队列满时，提供异常处理，确保程序的健壮性。
- 线程池析构时，确保所有线程正确退出，防止资源泄露。

## 使用说明

### 1. 启动设置

```c++
// "threadpool.h"
// 可选模式
enum class PoolMode
{
    MODE_FIXED, 
    MODE_CACHED,
};
// 设置模式
void ThreadPool::setPoolMode(PoolMode mode);
// 设置初始线程
void start(size_t initThreadSize = std::thread::hardware_concurrency());
```

#### fixed模式(默认模式)

线程池里面的线程个数是固定，默认根据当前机器的CPU核心数量进行指定。

#### cached模式

线程池里面的线程个数是可动态增长的，根据任务的数量动态的增加线程的数量，但是会设置一个线程数量的阈值，任务处理完成，如果动态增长的线程空闲一段时间还没有处理其它任务，那么关闭线程，保持池中最初数量的线程。

#### 示例

```c++
#include "threadpool.h"
int main()
{
    Threadpool pool;
    pool.setPoolMode(PoolMode::MODE_FIXED); // 设置为cached模式
    pool.start(4);  // 设置4个初始线程
    /*
    code
    */
}
```

### 2. 其他参数

```c++
// "threadpool.h"
const size_t TASK_MAX_THRESHOLD = INT32_MAX;
const size_t THREAD_MAX_THRESHHOLD = 1024;
const size_t THREAD_IDLE_TIME = 2; // 单位：秒
```

- TASK_MAX_THRESHOLD 设置任务队列最大任务数量
- THREAD_MAX_THRESHHOLD 设置cached模式线程上限阈值
- THREAD_IDLE_TIME 设置cached模式线程最大空闲时间

### 3. 设置并提交任务

```c++
#include "threadpool.h"
ThreadPool pool;
pool.start(4);

// 创建Task函数
int sum1(int a, int b)
{
    return a + b;
}

std::future<int> r1 = pool.submitTask(sum1, 1, 2);
```

### 4. 完整示例

**Example:**  

```cpp
// "main.cpp"
#include "threadpool.h"
#include <future>
using namespace std;

int sum1(int a, int b)
{
    return a + b;
}
int sum2(int a, int b, int c)
{
    return a + b + c;
}
int main()
{
  { 
    this_thread::sleep_for(chrono::seconds(2));
    // return 0;
    ThreadPool pool;
    // pool.setPoolMode(PoolMode::MODE_CACHED);
    pool.start(1);
    std::future<int> r1 = pool.submitTask(sum1, 1, 2);
    std::future<int> r2 = pool.submitTask(sum1, 1, 2);
    std::future<int> r3 = pool.submitTask(sum2, 1, 2, 3);
    std::future<int> r4 = pool.submitTask(sum2, 1, 2, 3);
    pool.submitTask(sum2, 1, 2, 3);
    pool.submitTask(sum2, 1, 2, 3);

    std::future<int> r5 = pool.submitTask([]() -> int
                                          {
      int sum=0;
      for(int i=0;i<=1000000;i++)
      {
        sum +=i;
      }
    return sum; });
    cout << r1.get() << endl;
    cout << r2.get() << endl;
    cout << r3.get() << endl;
    cout << r4.get() << endl;
    cout << r5.get() << endl;
  }
  getchar();
}
```

### 5. 编译
#### Window/Linux
##### CMake

```shell
$ mkdir build && cd build
$ cmake ..
$ make
```
#### Linux
##### 1). 直接编译
```shell
$ g++ -o main ./src/*.cpp -pthread -I ./inc -std=c++20
```
##### 2). 动态库
##### 生成动态库

```shell
$ mkdir lib
$ g++ -shared -fPIC -o lib/libtdpool.so src/threadpool.cpp -Iinc -std=c++20 -pthread
```

##### 安装库到标准位置并更新系统库缓存

```shell
# 1. 存储动态库，头文件（用于编译）
$ mv libtdpool.so /usr/local/lib
$ mv ./inc/threadpool.h /usr/local/include/ 
# 2. 编译
$ g++ -o main ./src/main.cpp -std=c++20 -ltdpool -lpthread
# 3. 添加自定义动态库路径
$ cd /etc/ld.so.conf.d
 # 创建新的配置文件
$ vim mylib.conf
 # 写入动态库路径
 /usr/local/lib
# 4. 刷新缓存
$ ldconfig
```

##### 使用本地库和头文件

```shell
$ g++ -o main src/main.cpp -Iinc -Llib -ltdpool -pthread -std=c++20
```




## 项目输出

### 应用到项目中

- 高并发网络服务器 

  处理io线程 void io_thread 接收listenfd 网络连接事件

  worker线程 void thread(int clientfd)处理已连接用户的读写线程

- master-slave线程模型 

  耗时计算

- 耗时任务处理

  文件传输 

**C++17 版本：**[address](https://github.com/Lionchen97/flexithreadpool-cpp11/tree/main)
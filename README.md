# FlexiThreadPool

## Project Introduction

FlexiThreadPool is a highly flexible and feature-rich C++ thread pool library, offering an efficient and scalable solution for concurrent task processing. It is particularly suitable for scenarios requiring handling of a large number of asynchronous tasks. Users can customize thread creation, manage threads, and assign any desired tasks for execution.

## README.md

\- zh_CN [简体中文](readme/README.zh_CN.md)

\- en [English](README.md)

## Running Environment

- C++ Standard: C++20 
- GNU: 8.5 and above
- CMake: 2.8 and above
- Operating Systems: Linux/Windows

## Knowledge Background

- Proficient in C++11 standard object-oriented programming:

  Composition and inheritance, polymorphism, STL containers, smart pointers, function objects, binders, variadic template programming, etc.

- Familiar with C++11 multi-threading programming:

  thread, mutex, atomic, condition_variable, unique_lock, etc.

- Contents of C++17 and C++20 standards:

  C++17's any type and C++20's semaphore.

- Familiar with multi-threading theory:

  Basic knowledge of multi-threading, thread mutex, thread synchronization, atomic operations, CAS, etc.

## Main Features

- **Configurable Modes**: Supports fixed mode (`MODE_FIXED`) and cached mode (`MODE_CACHED`), allowing flexible configuration of thread pool behavior according to application requirements.
- **Dynamic Task Processing**: Manages submitted tasks through a queue, providing thread-safe task submission and execution mechanisms.
- **Asynchronous Result Retrieval**: Task submitters can obtain a `future` object for asynchronously retrieving task execution results.
- **Resource Management Optimization**: Automatically recycles long-idle threads, optimizing resource usage.
- **Thread Synchronization**: Implements inter-thread synchronization using condition variables and mutexes.
- **Generic Result Storage**: Supports submission of various tasks and returns different types of task results through variadic templates.
- **Cross-Platform Compatibility**: Project build configuration (via CMake) is compatible with Linux and Windows, ensuring portability across different operating systems.

## Technical Challenges

- Implementing efficient thread synchronization mechanisms to ensure thread safety.
- Designing a universal result return mechanism to accommodate different types of tasks.
- Managing dynamically growing threads and task queues to optimize performance and resource utilization.

## Technical Details

### ThreadPool Class

- Provides flexible configuration options, such as setting the maximum threshold for the task queue and the maximum number of threads.
- Manages thread objects using `std::unique_ptr` smart pointers for automatic resource release.
- Uses condition variables to synchronize task submission and execution, optimizing performance.

### Thread Class

- Each thread has a unique ID for easy management and debugging.
- Threads run in a detached manner, avoiding the need for the main thread to wait for child threads to finish.

### Task Submission

- The `submitTask` function allows the thread pool to accept functions and parameters of different signatures.
- Uses `std::packaged_task` to encapsulate tasks, allowing asynchronous retrieval of task results.

### Thread Scheduling and Management

- In cached mode, dynamically adjusts the number of threads based on current task volume and thread idle status.
- Uses atomic operations and mutexes to ensure data consistency and thread safety in a multi-threading environment.

### Resource and Exception Management

- Provides exception handling when the task queue is full, ensuring the robustness of the program.
- Ensures all threads exit correctly when the thread pool is destructed, preventing resource leaks.

## Usage Instructions

### 1. Startup Settings

```c++
// "threadpool.h" 
// Available modes 
enum class PoolMode { 
    MODE_FIXED,
    MODE_CACHED, 
}; 
// Set mode 
void ThreadPool::setPoolMode(PoolMode mode); 
// Set initial threads 
void start(size_t initThreadSize = std::thread::hardware_concurrency());
```

#### Fixed Mode (Default Mode)

The number of threads in the thread pool is fixed, defaulting to the number of CPU cores on the current machine.

#### Cached Mode

The number of threads in the thread pool can dynamically grow based on the number of tasks. However, a threshold for the maximum number of threads is set. After task completion, if dynamically added threads remain idle for a certain period without handling other tasks, those threads are closed, maintaining the pool at its initial thread count.

#### Example

```c++
#include "threadpool.h"
int main()
{
    ThreadPool pool;
    pool.setPoolMode(PoolMode::MODE_FIXED); // Set to cached mode
    pool.start(4);  // Set 4 initial threads
    /*
    code
    */
}
```

### 2. Other Parameters

```c++
// "threadpool.h"
const size_t TASK_MAX_THRESHOLD = INT32_MAX;
const size_t THREAD_MAX_THRESHHOLD = 1024;
const size_t THREAD_IDLE_TIME = 2; // Unit: seconds
```

- TASK_MAX_THRESHOLD sets the maximum number of tasks in the task queue.
- THREAD_MAX_THRESHHOLD sets the maximum thread threshold in cached mode.
- THREAD_IDLE_TIME sets the maximum idle time for threads in cached mode.

### 3. Setting and Submitting Tasks

```c++
#include "threadpool.h"
ThreadPool pool;
pool.start(4);

// Create a Task function
int sum1(int a, int b)
{
    return a + b;
}
std::future<int> r1 = pool.submitTask(sum1, 1, 2);
```

### 4. Complete Example

**Example:**

```c++
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
    ThreadPool pool;
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

### 5. Compilation

#### Windows/Linux

##### CMake

```shell
$ mkdir build && cd build
$ cmake ..
$ make
```

#### Linux

##### 1). Direct Compilation

```shell
$ g++ -o main ./src/*.cpp -pthread -I ./inc -std=c++20
```

##### 2). Dynamic Library

##### Generating Dynamic Library

```shell
$ mkdir lib
$ g++ -shared -fPIC -o lib/libtdpool.so src/threadpool.cpp -Iinc -std=c++11 -pthread
```

##### Install the library to the standard location and update the system library cache

```shell
# 1. Store the dynamic library and header file (for compilation)
$ mv libtdpool.so /usr/local/lib
$ mv ./inc/threadpool.h /usr/local/include/ 
# 2. Compile
$ g++ -o main ./src/main.cpp -std=c++11 -ltdpool -lpthread
# 3. Add custom dynamic library path
$ cd /etc/ld.so.conf.d
 # Create a new configuration file
$ vim mylib.conf
 # Write the dynamic library path
 /usr/local/lib
 # 4. Refresh the cache
$ ldconfig
```

##### Using Local Library and Header Files

```shell
$ g++ -o main src/main.cpp -Iinc -Llib -ltdpool -pthread -std=c++20
```

## Project Output

### Application in Projects

- **High-Concurrency Network Servers**:

  Handle io threads `void io_thread` for receiving `listenfd` network connection events. Worker threads `void thread(int clientfd)` process read-write tasks for connected users.

- **Master-Slave Thread Model**:

  Time-consuming computations.

- **Time-Consuming Task Processing**:

  File transfer.

**C++17 version: ** [address](https://github.com/Lionchen97/flexithreadpool-cpp11/tree/main)


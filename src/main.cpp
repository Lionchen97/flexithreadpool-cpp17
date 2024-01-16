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
    { // packaged_task<int(int,int)> task(sum1);
        // future<int> res = task.get_future();

        // thread t(move(task),10,29);
        // /* 在C++中，如果一个 std::thread 对象被销毁而它仍然是可联结状态（即没有被分离或联结），程序将调用 std::terminate。为了避免这个问题，您需要在 main 函数结束前联结或分离线程： */
        // t.detach();
        // cout<<res.get()<<endl;
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
       for(int i=0;i<=1000000;i++){
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
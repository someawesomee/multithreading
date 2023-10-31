#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
using namespace std;

std::queue<int> produced_nums;
std::mutex m;
std::condition_variable cond_var;
std::atomic<bool> done{false};

class Generator
{
private:
    int num, max;
public:
    Generator(int mx)
    {
        num = 0;
        max = mx;
    }
    int nextNumber()
    {
        return num++;
    }
    bool isEnd()
    {
        return num == max;
    }
};

class ThreadProducer
{
private:
    std::thread thread;
    std::unique_ptr<Generator> gen;
    int id;
    void function()
    {
        while (!done)
        {
            {
                std::unique_lock<std::mutex> lock(m);
                cond_var.wait(lock, [] { return !produced_nums.empty(); });
                if (done)
                {
                    return;
                }
                std::cout << id << " consuming " << produced_nums.front() << '\n';
                produced_nums.pop();
            }
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
public:
    ThreadProducer(int id0, Generator *gn) : thread(&ThreadProducer::function, this), id(id0)
    {
        gen.reset(gn);
    }
    void start()
    {
        thread.join();
    }
    ~ThreadProducer()
    {
        if (thread.joinable()) thread.join();
    }
};

class ThreadConsumer
{
private:
    std::thread thread;
    std::unique_ptr<Generator> gen;
    int id;
    void function()
    {
        while (!done)
        {
            int current;
            {
                std::unique_lock<std::mutex> lock(m);
                if (gen->isEnd())
                {
                    done = true;
                    lock.unlock();
                    cond_var.notify_all();
                    return;
                }
                current = gen->nextNumber();
                std::cout << id << " producing " << current << '\n';
                produced_nums.push(current);
            }
            cond_var.notify_one();
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
public:
    ThreadConsumer(int id0, Generator *gn) : thread(&ThreadConsumer::function, this), id(id0)
    {
        gen.reset(gn);
    }
    void start()
    {
        thread.join();
    }
    ~ThreadConsumer()
    {
        if (thread.joinable()) thread.join();
    }
};

int main()
{
    const int num_producers = 5;
    const int num_consumers = 5;
    std::vector<std::unique_ptr<ThreadProducer>> producers;
    std::vector<std::unique_ptr<ThreadConsumer>> consumers;
    Generator gen(20);

    for (int i = 0; i < num_producers; ++i)
    {
        producers.emplace_back(std::make_unique<ThreadProducer>(i, &gen));
    }

    for (int i = 0; i < num_consumers; ++i)
    {
        consumers.emplace_back(std::make_unique<ThreadConsumer>(i, &gen));
    }

    for (auto &producer : producers)
    {
        producer->start();
    }

    for (auto &consumer : consumers)
    {
        consumer->start();
    }

    return 0;
}

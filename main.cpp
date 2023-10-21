#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;
queue<int> buffer;
mutex mtx;
condition_variable cv;


void producer(){
    for (int i = 1; i <= 5; ++i) {
        lock_guard<mutex>lock(mtx);
        buffer.push(i);
        cout << "Произведено:"<< i << endl;
        cv.notify_one();
        this_thread::sleep_for(chrono::milliseconds(500));

    }
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !buffer.empty(); });
        int data = buffer.front();
        buffer.pop();
        std::cout << "Потреблено: " << data << std::endl;
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main() {
    std::thread producerThread(producer);
    std::thread consumerThread(consumer);

    producerThread.join();
    consumerThread.join();

    return 0;
}
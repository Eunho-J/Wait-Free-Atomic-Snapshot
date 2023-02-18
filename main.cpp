#include "AtomicSnapshot.hpp"
#include <cassert>
#include <string>

using namespace std;

#define RUNTIME_PER_THREAD 60

//-------------------------AtomicSnapshot TEST----------------------

int num_thread = 4; //default thread is 4
AtomicSnapshot<int>* atomic_snapshot;
int finished = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void* thread_func(void* arg)
{
    int tid = int((long)arg);
    int* update_cnt = new int(0);
    shared_ptr<int> value_to_write = make_shared<int>(rand());

    struct timespec timeBegin, timeEnd;

    clock_gettime(CLOCK_MONOTONIC, &timeBegin);
    clock_gettime(CLOCK_MONOTONIC, &timeEnd);

    while (timeEnd.tv_sec - timeBegin.tv_sec <= RUNTIME_PER_THREAD)
    {
        atomic_snapshot->update(tid, value_to_write);
        // ----------if want to print snap------------
        // shared_ptr<vector<shared_ptr<int>>> snap = atomic_snapshot->scan(tid);

        // string snap_str = "tid[" + to_string(tid) + "]: ";
        // for (size_t i = 0; i < num_thread; i++)
        // {
        //     snap_str += to_string(*(*snap)[i]) + " ";
        // }
        //---------------------------------------------
        *update_cnt += 1;
        value_to_write = make_shared<int>(rand());
        clock_gettime(CLOCK_MONOTONIC, &timeEnd);
    }

    // lock only for check finish
    pthread_mutex_lock(&mutex);
    finished++;
    if (finished == num_thread)
    {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return (void*)update_cnt;
}


int main(int argc, char const *argv[])
{
    if (argc > 1) num_thread = stoi(argv[1]);
    
    atomic_snapshot = new AtomicSnapshot<int>(num_thread, make_shared<int>(0));

    pthread_t threads[num_thread];

    for (long i = 0; i < num_thread; i++) {
        if (pthread_create(&threads[i], 0, thread_func, (void*)i) < 0) {
        printf("pthread_create error!\n");
        return 0;
        }
    }

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    int* result_cnt[num_thread];
    int total_update = 0;
    for (int i = 0; i < num_thread; i++) {
        pthread_join(threads[i], (void**)&result_cnt[i]);
        total_update+= *result_cnt[i];
    }

    cout << total_update << endl;
    for (int i = 0; i < num_thread; i++)
    {
        delete result_cnt[i];
    }
    return 0;
}

//-------------------------AtomicSnapshot TEST----------------------
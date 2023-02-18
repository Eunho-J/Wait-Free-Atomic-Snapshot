#include <gtest/gtest.h>
#include "AtomicSnapshot.hpp"


int loop = 100000;

//-------------------------AtomicSingleRegister TEST----------------
AtomicSingleRegister<int>* asr;


void* threadfunc_single_reg(void* arg)
{
    shared_ptr<int> last_read_val;
    for (int i = 0; i < loop; i++)
    {
        shared_ptr<int> read_val = asr->read();
        if (i != 0) {
            EXPECT_GE(*read_val, *last_read_val);
        }
        last_read_val = read_val;
        // cout << *read_val << endl;
    }
    return nullptr;
}

TEST(AtomicSingleRegisterTest, AtomicReadAndWrite)
{
    shared_ptr<int> test_int_ptr = make_shared<int>(0);
    asr = new AtomicSingleRegister<int>(test_int_ptr);


    pthread_t thread;
    pthread_create(&thread, 0, threadfunc_single_reg, nullptr);

    for (int i = 0; i < loop; i++)
    {
        asr->write(make_shared<int>(i));
    }
    
    pthread_join(thread, nullptr);
    cout << "test SingleRegsiter fin" << endl;
    delete asr;

}
//-------------------------AtomicSingleRegister TEST----------------

//-------------------------AtomicMRSWRegister TEST------------------

AtomicMRSWRegister<int>* amr;
int num_reader = 6;

void* threadfunc_mrsw_reg(void* arg)
{
    int tid = int((long)arg);

    shared_ptr<int> last_read = amr->read(tid);
    while (true)
    {
        shared_ptr<int> read = amr->read(tid);
        EXPECT_LE(*last_read, *read);
        // cout << "[" << tid << "]: " << *read << endl;
        last_read = read;
        if(*last_read == loop) return nullptr;
    }
    return nullptr;
}


TEST(AtomicMRSWRegisterTest, AtomicReadAndWrite)
{
    shared_ptr<int> test_int_ptr = make_shared<int>(0);
    amr = new AtomicMRSWRegister<int>(num_reader, test_int_ptr);


    pthread_t thread[num_reader];
    for (long i = 1; i < num_reader; i++)
    {
        pthread_create(&thread[i], 0, threadfunc_mrsw_reg, (void*)i);
    }
    
    for (int i = 0; i <= loop; i++)
    {
        amr->write(0, make_shared<int>(i));
    }
    
    for (long i = 1; i < num_reader; i++)
    {
        pthread_join(thread[i], nullptr);
    }
    cout << "test MRSW fin" << endl;
    delete amr;
}

//-------------------------AtomicMRSWRegister TEST------------------



//-------------------------AtomicSnapshot TEST----------------------

#define RUNTIME_PER_THREAD 10
int num_thread = 4; //default thread is 4
AtomicSnapshot<int>* atomic_snapshot;
int finished = 0;
pthread_mutex_t mutex_snap = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_snap = PTHREAD_COND_INITIALIZER;


void* thread_func_atomic_snapshot(void* arg)
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
        if ( (timeEnd.tv_nsec - timeBegin.tv_nsec) % 10000 == 0)
        {
            shared_ptr<vector<shared_ptr<int>>> snap = atomic_snapshot->scan(tid);

            string snap_str = "tid[" + to_string(tid) + "] snap: ";
            for (int i = 0; i < num_thread; i++)
            {
                snap_str += to_string(*(*snap)[i]) + " ";
            }
            cout << snap_str << endl;
        }
        //---------------------------------------------
        *update_cnt += 1;
        value_to_write = make_shared<int>(rand());
        clock_gettime(CLOCK_MONOTONIC, &timeEnd);
    }

    // lock only for check finish
    pthread_mutex_lock(&mutex_snap);
    finished++;
    if (finished == num_thread)
    {
        pthread_cond_signal(&cond_snap);
    }
    pthread_mutex_unlock(&mutex_snap);
    return (void*)update_cnt;
}


TEST(AtomicSnapshotTest, AtomicReadAndWrite)
{
    
    atomic_snapshot = new AtomicSnapshot<int>(num_thread, make_shared<int>(0));

    pthread_t threads[num_thread];

    for (long i = 0; i < num_thread; i++) {
        EXPECT_GE(pthread_create(&threads[i], 0, thread_func_atomic_snapshot, (void*)i), 0);
    }

    pthread_mutex_lock(&mutex_snap);
    pthread_cond_wait(&cond_snap, &mutex_snap);
    pthread_mutex_unlock(&mutex_snap);

    int* result_cnt[num_thread];
    int total_update = 0;
    for (int i = 0; i < num_thread; i++) {
        pthread_join(threads[i], (void**)&result_cnt[i]);
        total_update+= *result_cnt[i];
    }

    cout << "total updates: " << total_update << endl;
    for (int i = 0; i < num_thread; i++)
    {
        delete result_cnt[i];
    }
    delete atomic_snapshot;
}

//-------------------------AtomicSnapshot TEST----------------------
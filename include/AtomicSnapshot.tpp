#include "AtomicSnapshot.hpp"
using namespace std;

//-------------------------------------AtomicSingleRegister-----------------------------------
template <typename T>
shared_ptr<T> AtomicSingleRegister<T>::read()
{
    StampedValue<T>* value = r_value;
    StampedValue<T>* last = last_read;
    StampedValue<T>* result = (value->label > last->label) ? value : last;
    last_read = result;
    return result->value;
}

template <typename T>
void AtomicSingleRegister<T>::write(shared_ptr<T> value_to_write)
{
    uint64_t label = last_label + 1;
    not_free_queue.push(r_value);
    r_value = new StampedValue<T>(label, value_to_write);
    last_label = label;

    StampedValue<T>* last = last_read;

    last_free = (last->label < last_free->label) ? last_free : last;
    
    while (!not_free_queue.empty())
    {
        if (not_free_queue.front()->label == last_free->label)
            break;
        delete not_free_queue.front();
        not_free_queue.pop();
    }
}

template <typename T>
AtomicSingleRegister<T>::AtomicSingleRegister()
{
    last_label = 0;
    r_value = new StampedValue<T>();
    last_read = r_value;
    last_free = r_value;
}

template <typename T>
AtomicSingleRegister<T>::AtomicSingleRegister(shared_ptr<T> init_value)
{
    last_label = 0;
    r_value = new StampedValue<T>(last_label, init_value);
    last_read = r_value;
    last_free = r_value;
}

template <typename T>
AtomicSingleRegister<T>::~AtomicSingleRegister()
{
    while (!not_free_queue.empty())
    {
        delete not_free_queue.front();
        not_free_queue.pop();
    }
    delete r_value;
}

//-------------------------------------AtomicSingleRegister-----------------------------------


//--------------------------------------AtomicMRSWRegister------------------------------------

template <typename T>
AtomicMRSWRegister<T>::AtomicMRSWRegister(int num_reader, shared_ptr<T> init_value)
{
    this->num_reader = num_reader;
    int loop = num_reader * num_reader;
    a_table.reserve(loop);
    shared_ptr<StampedValue<T>> init_stamped_value = make_shared<StampedValue<T>>(0, init_value);
    for (int i = 0; i < loop; i++)
    {
        a_table.emplace_back(init_stamped_value);
    }
    last_label = 0;
}

template <typename T>
AtomicMRSWRegister<T>::~AtomicMRSWRegister()
{
    a_table.clear();
}

template <typename T>
shared_ptr<T> AtomicMRSWRegister<T>::read(int tid)
{
    int my_index = tid * num_reader + tid;
    shared_ptr<StampedValue<T>> value = a_table[my_index].read();
    for (int i = 0; i < num_reader; i++)
    {
        int row_index = tid * num_reader + i;
        shared_ptr<StampedValue<T>> tmp = a_table[row_index].read();
        if (tmp->label > value->label)
            value = tmp;
    }
    for (int i = 0; i < num_reader; i++)
    {
        int col_index = i * num_reader + tid;
        a_table[col_index].write(value);
    }
    return value->value;
}

template <typename T>
void AtomicMRSWRegister<T>::write(int tid, shared_ptr<T> value_to_write)
{
    uint64_t label = last_label + 1;
    last_label = label;
    shared_ptr<StampedValue<T>> value = make_shared<StampedValue<T>>(label, value_to_write);
    for (int i = 0; i < num_reader; i++)
    {
        int col_index = i * num_reader + tid;
        a_table[col_index].write(value);
    }
}

//--------------------------------------AtomicMRSWRegister------------------------------------


//--------------------------------------------SnapValue---------------------------------------

template <typename T>
SnapValue<T>::SnapValue()
{
    label = 0;
    value = make_shared<T>();
    snap = nullptr;
}

template <typename T>
SnapValue<T>::SnapValue(uint64_t label, shared_ptr<T> value, shared_ptr<vector<shared_ptr<T>>> snap)
{
    this->label = label;
    this->value = value;
    this->snap = snap;
}

template <typename T>
SnapValue<T>::SnapValue(shared_ptr<T> init_value)
{
    this->label = 0;
    this->value = init_value;
    this->snap = nullptr;
}
//--------------------------------------------SnapValue---------------------------------------


//---------------------------------------AtomicSnapshot---------------------------------------

template <typename T>
AtomicSnapshot<T>::AtomicSnapshot(int num_registers, shared_ptr<T> init_value)
{
    this->num_registers = num_registers;
    shared_ptr<SnapValue<T>> init_snapvalue = make_shared<SnapValue<T>>(init_value);
    registers.reserve(num_registers);
    for (int i = 0; i < num_registers; i++)
    {
        registers.emplace_back(num_registers, init_snapvalue);
    }
}

template <typename T>
AtomicSnapshot<T>::~AtomicSnapshot()
{
    registers.clear();
}

template <typename T>
shared_ptr<vector<shared_ptr<SnapValue<T>>>> AtomicSnapshot<T>::collect(int tid)
{
    shared_ptr<vector<shared_ptr<SnapValue<T>>>> copy = make_shared<vector<shared_ptr<SnapValue<T>>>>();
    for (int i = 0; i < num_registers; i++)
    {
        copy->push_back(registers[i].read(tid));
    }
    return copy;
}

template <typename T>
shared_ptr<vector<shared_ptr<T>>> AtomicSnapshot<T>::getValues(shared_ptr<vector<shared_ptr<SnapValue<T>>>> copy)
{
    shared_ptr<vector<shared_ptr<T>>> snap = make_shared<vector<shared_ptr<T>>>();
    for (int i = 0; i < num_registers; i++)
    {
        snap->push_back((*copy)[i]->value);
    }
    return snap;
}

template <typename T>
shared_ptr<vector<shared_ptr<T>>> AtomicSnapshot<T>::scan(int tid)
{
    shared_ptr<vector<shared_ptr<SnapValue<T>>>> old_copy;
    shared_ptr<vector<shared_ptr<SnapValue<T>>>> new_copy;
    bool moved[num_registers] = {false, };
    old_copy = collect(tid);
    while (true)
    {
        new_copy = collect(tid);
        for (int i = 0; i < num_registers; i++)
        {
            if ((*old_copy)[i]->label != (*new_copy)[i]->label)
            {
                if (moved[i])
                {
                    return (*new_copy)[i]->snap;
                }
                else
                {
                    moved[i] = true;
                    old_copy = new_copy;
                    break;
                }
            }
            else if ( i == num_registers - 1)
            {
                return getValues(new_copy);
            }
        }
    }
}

template <typename T>
void AtomicSnapshot<T>::update(int tid, shared_ptr<T> value_to_update)
{
    shared_ptr<vector<shared_ptr<T>>> snap = scan(tid);
    shared_ptr<SnapValue<T>> old_snapvalue = registers[tid].read(tid);
    shared_ptr<SnapValue<T>> new_snapvalue = make_shared<SnapValue<T>>(old_snapvalue->label+1, value_to_update, snap);
    registers[tid].write(tid, new_snapvalue);
}

//---------------------------------------AtomicSnapshot---------------------------------------
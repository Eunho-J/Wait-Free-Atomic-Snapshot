#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#ifndef __ATOMICSNAPSHOT_HPP__
#define __ATOMICSNAPSHOT_HPP__

using namespace std;


template <typename T>
class StampedValue{
public:
    uint64_t label;
    shared_ptr<T> value;

    StampedValue():label(0), value(nullptr) {};
    StampedValue(uint64_t label, shared_ptr<T> init_value): label(label) {value = init_value;};
    ~StampedValue() {};
};

template <typename T>
class AtomicSingleRegister{
private:
    uint64_t last_label;
    StampedValue<T>* last_read;
    StampedValue<T>* r_value;
    StampedValue<T>* last_free;
    queue<StampedValue<T>*> not_free_queue;
public:
    shared_ptr<T> read();
    void write(shared_ptr<T> value_to_write);

    AtomicSingleRegister();
    AtomicSingleRegister(shared_ptr<T> init_value);
    ~AtomicSingleRegister();
};

template <typename T>
class AtomicMRSWRegister{
private:
    int num_reader;
    uint64_t last_label;
    vector<AtomicSingleRegister<StampedValue<T>>> a_table; 
public:
    shared_ptr<T> read(int tid);
    void write(int tid, shared_ptr<T> value_to_write);

    // AtomicMRSWRegister();
    AtomicMRSWRegister(int num_reader, shared_ptr<T> init_value);
    ~AtomicMRSWRegister();
};

template <typename T>
class SnapValue{
public:
    uint64_t label;
    shared_ptr<T> value;
    shared_ptr<vector<shared_ptr<T>>> snap;

    SnapValue();
    SnapValue(uint64_t label, shared_ptr<T> value, shared_ptr<vector<shared_ptr<T>>> snap);
    SnapValue(shared_ptr<T> init_value);
    ~SnapValue() {};
};

template <typename T>
class AtomicSnapshot{
private:
    int num_registers;
    vector<AtomicMRSWRegister<SnapValue<T>>> registers;
    shared_ptr<vector<shared_ptr<SnapValue<T>>>> collect(int tid);
    shared_ptr<vector<shared_ptr<T>>> getValues(shared_ptr<vector<shared_ptr<SnapValue<T>>>> copy);
public:
    shared_ptr<vector<shared_ptr<T>>> scan(int tid);
    void update(int tid, shared_ptr<T> value_to_update);

    AtomicSnapshot(int num_registers, shared_ptr<T> init_value);
    ~AtomicSnapshot();
};

#include "AtomicSnapshot.tpp"
#endif
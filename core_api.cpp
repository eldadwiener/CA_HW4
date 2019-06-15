/* 046267 Computer Architecture - Spring 2019 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <vector>
#include <list>
#include <assert.h>

using namespace std;

typedef enum MODE {BLOCKED, FINEGRAIN} MODE;

CPU blockedcpu(BLOCKED), finegraincpu(FINEGRAIN);

class CPU
{
public:
    CPU(MODE mode) : _mode(mode), _clock(0) { _switchPenalty = (mode == FINEGRAIN)? 0:Get_switch_cycles(); }
    simReset(unsigned numThreads);
    void run(); // TODO: should we return anything?
    double getCPI();
    void getThreadContext(tcontext* bcontext, int threadid);

private:
    unsigned _numThreads;
    MODE _mode;
    unsigned _clock;
    unsigned _switchPenalty;
    vector<Thread> _threads;
    list<unsigned> _liveThreads;

};

class Thread
{
public:
    Thread(MODE mode);
    unsigned run(unsigned curClock);
    unsigned getTotalInstructions();
    void copyContext(tcontext* dst);
    
private:
    void arith_command();
    void mem_command();
    MODE _mode;
    Latency _latency;
    unsigned _idleUntil;
    tcontext _context;
    unsigned _currentInst;

};




void CPU::run()
{
    while( !_liveThreads.empty() )
    {
        unsigned curThread = _liveThreads.front();
        _liveThreads.pop_front(); // after iteration we will push it to the back if not done
        unsigned clocksRan = _threads[curThread].run(_clock); // run the thread until it goes to idle or 1 clock if we are in finegrain
        assert(clocksRan > 0);
        _clock += _switchPenalty + clocksRan;
        if(_threads[curThread].done == false)
            _liveThreads.push_back(curThread);
    }
}

void CPU::simReset(unsigned numThreads)
{
    // clear old simulation data if it existed
    _threads.clear();
    _liveThreads.clear();
    _numThreads = numThreads;
    _clock = 0;
    // create new thread objects
    for (int i = 0; i < numThreads; ++i)
    {
        _threads.push_back(Thread(_mode, i));
        _liveThreads.push_back(i);
    }
}

double CPU::getCPI()
{
    unsigned totalInsts = 0;
    for(int i=0; i < _numThreads; ++i)
        totalInsts += _threads[i].getTotalInstructions();
    return ( (double)_clock ) / totalInsts;
}

void CPU::getThreadContext(tcontext* bcontext, int threadid)
{
    _threads[threadid].copyContext(bcontext);
}

Status Core_blocked_Multithreading(){
    // reset the blocked simulation and rerun it
    blockedcpu.simReset(Get_thread_number());
    blockedcpu.run();
    return Success;
}


Status Core_fineGrained_Multithreading(){
    // reset the finegrain simulation and rerun it
    finegraincpu.simReset(Get_thread_number());
    finegraincpu.run();
    return Success;
}


double Core_finegrained_CPI(){
    return finegraincpu.get_cpi();
}
double Core_blocked_CPI(){
    return blockedcpu.get_cpi();
}

Status Core_blocked_context(tcontext* bcontext,int threadid){
    blockedcpu.getThreadContext(bcontext, threadid);
    return Success;
}

Status Core_finegrained_context(tcontext* finegrained_context,int threadid){
    finegraincpu.getThreadContext(bcontext, threadid);
    return Success;
}

/* 046267 Computer Architecture - Spring 2019 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <vector>
#include <list>

using namespace std;

typedef enum MODE {BLOCKED, FINEGRAIN} MODE;

class CPU
{
public:
    CPU(unsigned numThreads, MODE mode);
    void run(); // TODO: should we return anything?
    double getCPI();
    void getThreadContext(tcontext* bcontext, int threadid);
    
private:
    unsigned _numThreads;
    unsigned _clock;
    MODE _mode;
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

Status Core_blocked_Multithreading(){
	return Success;
}


Status Core_fineGrained_Multithreading(){

	return Success;
}


double Core_finegrained_CPI(){
	return 0;
}
double Core_blocked_CPI(){
	return 0;
}

Status Core_blocked_context(tcontext* bcontext,int threadid){
	return Success;
}

Status Core_finegrained_context(tcontext* finegrained_context,int threadid){
	return Success;
}

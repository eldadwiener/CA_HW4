/* 046267 Computer Architecture - Spring 2019 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <vector>
#include <list>
#include <assert.h>

using namespace std;

typedef enum MODE {BLOCKED, FINEGRAIN} MODE;

class Thread
{
public:
    Thread(MODE mode, unsigned Tid);
    unsigned run(unsigned curClock);
    unsigned getTotalInstructions();
    void copyContext(tcontext* dst);
    bool done;

private:
    void arith_command(Instuction* Inst);
    void mem_command(Instuction* Inst, unsigned time);

    MODE _mode;
    Latency _latency;
    unsigned _idleUntil;
    tcontext _context;
    unsigned _currentInst;
    unsigned _Tid;
    bool _waitingLoad;
};


class CPU
{
public:
    CPU(MODE mode) : _mode(mode), _clock(0) {}
    void simReset(unsigned numThreads);
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

CPU blockedcpu(BLOCKED), finegraincpu(FINEGRAIN);


void CPU::run()
{
    while( !_liveThreads.empty() )
    {
        unsigned curThread = _liveThreads.front();
        _liveThreads.pop_front(); // after iteration we will push it to the back if not done
        unsigned clocksRan = _threads[curThread].run(_clock); // run the thread until it goes to idle or 1 clock if we are in finegrain
        assert(clocksRan > 0);
        _clock += clocksRan;
        if(_threads[curThread].done == false)
            _liveThreads.push_back(curThread);
        if(!_liveThreads.empty() )
            _clock += _switchPenalty; // need to switch to another thread.
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
    for (unsigned i = 0; i < numThreads; ++i)
    {
        _threads.push_back(Thread(_mode, i));
        _liveThreads.push_back(i);
    }
     _switchPenalty = (_mode == FINEGRAIN)? 0:Get_switch_cycles();
}

double CPU::getCPI()
{
    unsigned totalInsts = 0;
    for(unsigned i=0; i < _numThreads; ++i)
        totalInsts += _threads[i].getTotalInstructions();
    return ( (double)_clock ) / totalInsts;
}

void CPU::getThreadContext(tcontext* bcontext, int threadid)
{
    _threads[threadid].copyContext(bcontext);
}

Thread::Thread(MODE mode, unsigned Tid) : 
	done(false), _mode(mode), _idleUntil(0), _currentInst(0), _Tid(Tid), _waitingLoad(false)
{
	//set the regs to 0
	for (int i = 0; i < REGS; ++i) {
		_context.reg[i] = 0;
	}
	//get the lantencies
	int latencyArr[2] = { 0 };
	Mem_latency(latencyArr);
	_latency.Load_latecny = latencyArr[0];
	_latency.Store_latency = latencyArr[1];
}

unsigned Thread::run(unsigned curClock) {
    //if the thread is in idle state
    int curRun = 0;
    if (_idleUntil != 0 && curClock < _idleUntil) {
            return ++curRun;
    }
    //do the intructions
    while (true) {
        Instuction Inst;
        bool needSwitch = false;
        if (_waitingLoad) {
            _waitingLoad = false;
            ++curRun;
        }
        else
        {
            SIM_MemInstRead(_currentInst, &Inst, _Tid);
            switch (Inst.opcode) {
                case CMD_NOP:
                    ++curRun;
                    _currentInst++;
                    break;
                case CMD_ADD:  // dst <- src1 + src2
                case CMD_SUB:  // dst <- src1 - src2
                case CMD_ADDI: // dst <- src1 + imm
                case CMD_SUBI: // dst <- src1 - imm
                    ++curRun;
                    arith_command(&Inst);
                    _currentInst++;
                    break;
                case CMD_LOAD:   // dst <- Mem[src1 + src2]  (src2 may be an immediate)
                case CMD_STORE:  // Mem[dst + src2] <- src1  (src2 may be an immediate) 
                    ++curRun;
                    mem_command(&Inst, curRun + curClock);
                    _currentInst++;
                    needSwitch = true;
                    break;
                case CMD_HALT: //the end of these thread
                    done = true;
                    _currentInst++;
                    return ++curRun;
            }
        }
        //if it's FINEGRAIN mode or the thread is in idle state 
        if (_mode == FINEGRAIN || needSwitch ) {
            break;
        }
    }
    return curRun;
}

void Thread::arith_command(Instuction* Inst) {
	cmd_opcode opcode = Inst->opcode;
	int dst_index = Inst->dst_index;
	int src1_index = Inst->src1_index;
	int src2_index_imm = Inst->src2_index_imm;
	switch (opcode) {
		case CMD_ADD:  // dst <- src1 + src2
			_context.reg[dst_index] = _context.reg[src1_index] + _context.reg[src2_index_imm];
			break;
		case CMD_SUB:  // dst <- src1 - src2
			_context.reg[dst_index] = _context.reg[src1_index] - _context.reg[src2_index_imm];
			break;
		case CMD_ADDI: // dst <- src1 + imm
			_context.reg[dst_index] = _context.reg[src1_index] + src2_index_imm;
			break;
		case CMD_SUBI: // dst <- src1 - imm
			_context.reg[dst_index] = _context.reg[src1_index] - src2_index_imm;
			break;
	}
	return;
}

void Thread::mem_command(Instuction* Inst, unsigned time) {
    cmd_opcode opcode = Inst->opcode;
    int dst_index = Inst->dst_index;
    int src1_index = Inst->src1_index;
    int src2_index_imm = Inst->src2_index_imm;
    bool isSrc2Imm = Inst->isSrc2Imm;
    uint32_t addr;
    switch (opcode) {
        case CMD_LOAD:   // dst <- Mem[src1 + src2]  (src2 may be an immediate)
            addr = (isSrc2Imm == true) ? _context.reg[src1_index] + src2_index_imm :
                                          _context.reg[src1_index] + _context.reg[src2_index_imm];
            SIM_MemDataRead(addr, _context.reg + dst_index);
            _idleUntil = time + _latency.Load_latecny;
            _waitingLoad =  true;
            break;
        case CMD_STORE:  // Mem[dst + src2] <- src1  (src2 may be an immediate) 		_context.reg[dst_index] = _context.reg[src1_index] - src2_index_imm;
            addr = (isSrc2Imm == true) ? _context.reg[dst_index] + src2_index_imm :
                                      _context.reg[dst_index] + _context.reg[src2_index_imm];
            SIM_MemDataWrite(addr, _context.reg[src1_index]);
            _idleUntil = time + _latency.Store_latency;
            break;
    }
    return;
}

unsigned Thread::getTotalInstructions()
{
    return _currentInst;
}


void Thread::copyContext(tcontext* dst)
{
    for(int i=0; i < REGS; ++i)
        dst->reg[i] = _context.reg[i];
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
    return finegraincpu.getCPI();
}
double Core_blocked_CPI(){
    return blockedcpu.getCPI();
}

Status Core_blocked_context(tcontext* bcontext,int threadid){
    blockedcpu.getThreadContext(bcontext, threadid);
    return Success;
}

Status Core_finegrained_context(tcontext* finegrained_context,int threadid){
    finegraincpu.getThreadContext(finegrained_context, threadid);
    return Success;
}

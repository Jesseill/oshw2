// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------



//<TODO?DONE>
// Declare sorting rule of SortedList for L1 & L2 ReadyQueue
// Hint: Funtion Type should be "static int"

// Jess added uncertain

static int sortingRuleL1(Thread *A , Thread*B){ 
    // L1: shortes remaining burst time first
    int valA = A->getRemainingBurstTime();
    int valB = B->getRemainingBurstTime();

    if(valA<valB) return -1;

    if(valA==valB) return 0;

    if(valA>valB) return 1;

}
static int sortingRuleL2(Thread *A , Thread*B){
    //L2 : smaller ID First
    int valA = A->getID();
    int valB = B->getID();

    if(valA<valB) return -1;

    if(valA==valB) return 0;

    if(valA>valB) return 1;
    
}
	
//<TODO?DONE>

Scheduler::Scheduler()
{
//	schedulerType = type;
    // readyList = new List<Thread *>; 
    //<TODO ?DONE>
    // Initialize L1, L2, L3 ReadyQueue

    // all added by Jess
    L1ReadyQueue = new SortedList<Thread* >(sortingRuleL1);
    L2ReadyQueue = new SortedList<Thread* >(sortingRuleL2);
    L3ReadyQueue = new List<Thread*>;

    //<TODO ?DONE>
	toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //<TODO DONE>
    // Remove L1, L2, L3 ReadyQueue
    // all added by Jess
    delete L1ReadyQueue;
    delete L2ReadyQueue;
    delete L3ReadyQueue;

    //<TODO DONE>
    // delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    Statistics* stats = kernel->stats;
    //<TODO ?>
    // According to priority of Thread, put them into corresponding ReadyQueue.
    // After inserting Thread into ReadyQueue, don't forget to reset some values.
    // Hint: L1 ReadyQueue is preemptive SRTN(Shortest Remaining Time Next).
    // When putting a new thread into L1 ReadyQueue, you need to check whether preemption or not.


    int prio = thread->getPriority();
    
    //reset what value??? status??
    thread->setStatus(READY);


    if(prio > 99){ // 100~149 in L1
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread:[" << thread->getID() <<"] is inserted into queue L[1]");
        L1ReadyQueue->Insert(thread); //uncertain

        if(thread->getRemainingBurstTime()< kernel->currentThread->getRemainingBurstTime() - kernel->currentThread->getRunTime())
        {
            kernel->interrupt->YieldOnReturn(); //preempty??
        }
        
        
    }else if(prio > 49){ //50~99 in L2
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread:[" << thread->getID() <<"] is inserted into queue L[2]");
        L2ReadyQueue->Insert(thread); //uncertain too
       
    }else{ // 0~49 L3
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread:[" << thread->getID() <<"] is inserted into queue L[3]");
        L3ReadyQueue->Append(thread);
        

    }


    //<TODO ?>
    // readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    /*if (readyList->IsEmpty()) {
    return NULL;
    } else {
        return readyList->RemoveFront();
    }*/

    //<TODO?DONE>
    // a.k.a. Find Next (Thread in ReadyQueue) to Run
    Thread * next = NULL;

    if(!L1ReadyQueue->IsEmpty()){
        next = L1ReadyQueue->Front();
        L1ReadyQueue->RemoveFront();
        return next;
    }else if(!L2ReadyQueue->IsEmpty()){
        next = L2ReadyQueue->Front();
        L2ReadyQueue->RemoveFront();
        return next;
    }else if(!L3ReadyQueue->IsEmpty()){ //RoundRobin??
        next = L3ReadyQueue->Front();
        L3ReadyQueue->RemoveFront();
        return next;
    }
    return next;
    //<TODO?DONE>
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
 
//	cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;
   
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	     toBeDestroyed = oldThread;
    }
   
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (oldThread->space != NULL) {	// if this thread is a user program,

        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    cout << "Switching from: " << oldThread->getID() << " to: " << nextThread->getID() << endl;
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << kernel->currentThread->getID());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
#ifdef USER_PROGRAM
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        DEBUG(dbgThread, "toBeDestroyed->getID(): " << toBeDestroyed->getID());
        delete toBeDestroyed;
	    toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // readyList->Apply(ThreadPrint);
    L1ReadyQueue->Apply(ThreadPrint);
    L2ReadyQueue->Apply(ThreadPrint);
    L3ReadyQueue->Apply(ThreadPrint);
}

// <TODO ?DONE>

// Function 1. Function definition of sorting rule of L1 ReadyQueue
void sortL1(){// unused
  
}
// Function 2. Function definition of sorting rule of L2 ReadyQueue
void sortL2(){//unused

    

}
// Function 3. Scheduler::UpdatePriority()
// Hint:
// 1. ListIterator can help.
// 2. Update WaitTime and priority in Aging situations
// 3. After aging, Thread may insert to different ReadyQueue

void 
Scheduler::UpdatePriority()
{

    ListIterator<Thread *> *it3 = new ListIterator<Thread *>(L3ReadyQueue);
    // ListIterator<Thread *> *it2 = new ListIterator<Thread *>(L2ReadyQueue);
    // ListIterator<Thread *> *it1 = new ListIterator<Thread *>(L1ReadyQueue); 
    //update waiting time
    for( ; !it3->IsDone(); it3->Next() ){
        it3->current->setWaitingTime(   it3->current->getWaitingTime()+100 );
        if(it3->current->getWaitingTime() %400 ==0){
            it3->current->setPriority(it3->current->getPriority()+10);
        }
    }
    it3->current = L2ReadyQueue->first;

    for( ; !it3->IsDone(); it3->Next() ){
        it3->current->setWaitingTime(   it3->current->getWaitingTime()+100 );
        if(it3->current->getWaitingTime() %400 ==0){
            it3->current->setPriority(it3->current->getPriority()+10);
        }
    }    

    it3->current = L1ReadyQueue->first;
    for( ; !it3->IsDone(); it3->Next() ){
        it3->current->setWaitingTime(   it3->current->getWaitingTime()+100 );
        if(it3->current->getWaitingTime() %400 ==0){
            it3->current->setPriority(it3->current->getPriority()+10>149 ? 149:it3->current->getPriority()+10);
        }
    }
    Thread * curr;
    it3->current = L3ReadyQueue ->first;

    for( ; !it3->IsDone(); it3->Next() ){
        if(it3->current->getPriority() >=50){
            curr = it3->current;
            it3->Next();
            L3ReadyQueue->Remove(curr);
            L2ReadyQueue->Insert(curr);
        }
    }

    //sortL2();
    it3->current = L2ReadyQueue->first;
    for( ; !it3->IsDone(); it3->Next() ){
        if(it3->current->getPriority() >=100){
            curr = it3->current;
            it3->Next();
            L2ReadyQueue->Remove(curr);
            L1ReadyQueue->Insert(curr);
        }
    
    }


    //sortL1(); //preemption
    if(L1ReadyQueue->Front()!=kernel->currentThread){
        kernel->interrupt->YieldOnReturn();
    }



}

// <TODO ?DONE>
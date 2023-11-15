// TestMultiThread.cpp : Defines the entry point for the console application.
//
//#define TESTMULTITHREAD_ON

#ifdef TESTMULTITHREAD_ON

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <queue>

#define MAX_THREAD 2

HANDLE hMutex;

/*
template<typename Data>
class concurrent_queue
{
private:
	std::queue<Data> the_queue;
	mutable boost::mutex the_mutex;
	boost::condition_variable the_condition_variable;
public:
	void push(Data const& data)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		the_queue.push(data);
		lock.unlock();
		the_condition_variable.notify_one();
	}

	bool empty() const
	{
		boost::mutex::scoped_lock lock(the_mutex);
		return the_queue.empty();
	}

	bool try_pop(Data& popped_value)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		if(the_queue.empty())
		{
			return false;
		}

		popped_value=the_queue.front();
		the_queue.pop();
		return true;
	}

	void wait_and_pop(Data& popped_value)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		while(the_queue.empty())
		{
			the_condition_variable.wait(lock);
		}

		popped_value=the_queue.front();
		the_queue.pop();
	}

};
*/

DWORD WINAPI t_fun(void* data)
{
	int* count = (int*) data;
	int tmp;

	while(1){

		WaitForSingleObject(hMutex, INFINITE);
		tmp = *count;
		tmp++;
		Sleep(500);
		*count = tmp;
		printf("%d %d \n", GetCurrentThreadId(), *count);
		ReleaseMutex(hMutex);
		
		if(*count > 10){
			break;
		}		
	}
	return 0;
}
class Queue {
private:
	CONDITION_VARIABLE m_waitForObj;
	CONDITION_VARIABLE m_waitForSlot;
	CRITICAL_SECTION m_cs; 

	std::deque< int > m_queue;
	int m_queueSize;
public:
	Queue(int queueSize)
		:m_queueSize(queueSize){
			InitializeConditionVariable(&m_waitForObj);
			InitializeConditionVariable(&m_waitForSlot);
			InitializeCriticalSection(&m_cs);
	}
	~Queue(){
	} 

	void Push(int v){
		EnterCriticalSection(&m_cs);
		while(m_queueSize == m_queue.size()){
			// no slot is available
			SleepConditionVariableCS(&m_waitForSlot, &m_cs, INFINITE);
		}
		m_queue.push_back(v);
		WakeConditionVariable(&m_waitForObj);
		LeaveCriticalSection(&m_cs);
	} 

	void Pull(int& v){
		EnterCriticalSection(&m_cs);
		while(m_queue.empty()){
			// no slot is available
			SleepConditionVariableCS(&m_waitForObj, &m_cs, INFINITE);
		}
		v = m_queue.front();
		m_queue.pop_front();
		WakeConditionVariable(&m_waitForSlot);
		LeaveCriticalSection(&m_cs);
	}
	int getSize(){return m_queue.size();}
}; 

Queue queue(100); 

DWORD WINAPI PushLoop(void* data)
{
	for(int i = 0; i < 1024 * 1024 * 10; i++){
		queue.Push(i);
		printf("push : size = %d, \n", queue.getSize());
		Sleep(200);
	}
	return 0;
} 

DWORD WINAPI PullLoop(void* data)
{
	for(int i = 0; i < 1024 * 1024 * 10; i++){
		int v;
		queue.Pull(v);
		printf("pull : size = %d, \n", queue.getSize());
		Sleep(500);
	}
	return 0;
}



int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hPushThread = CreateThread(NULL, 0, PushLoop, NULL, 0, NULL);
	HANDLE hPullThread = CreateThread(NULL, 0, PullLoop, NULL, 0, NULL);

	HANDLE hThreadArray[MAX_THREAD];
	hThreadArray[0] = hPushThread;
	hThreadArray[1] = hPullThread;

	WaitForMultipleObjects(MAX_THREAD, hThreadArray, TRUE, INFINITE);
	CloseHandle(hPushThread);
	CloseHandle(hPullThread);
	

	/*
	DWORD dwThreadArray[MAX_THREAD];
	HANDLE hThreadArray[MAX_THREAD];
	int i;
	int count = 0;
	
	hMutex = CreateMutex(NULL, FALSE, NULL);

	for(i=0;i<MAX_THREAD; i++){
		hThreadArray[i] = CreateThread(NULL, 0, t_fun, (void*)&count, 0, &dwThreadArray[i]);
	}

	WaitForMultipleObjects(MAX_THREAD, hThreadArray, TRUE, INFINITE);

	for (i=0; i<MAX_THREAD; i++){
		CloseHandle(hThreadArray[i]);
	}
	CloseHandle(hMutex);
	*/

	//boost::timer tim;
	//boost::thread pushThread(PushLoop);
	//boost::thread pullThread(PullLoop);
	//pushThread.join();
	//pullThread.join();
	//cout << tim.elapsed() << endl;

	
	return 0;
}

#endif
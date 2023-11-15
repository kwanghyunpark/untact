#define TEST4_ON

#ifdef TEST4_ON
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <process.h>

typedef struct{
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE rReady;
	CONDITION_VARIABLE mReady;
	bool writeable;
	int sharedData;
} buffer;

buffer theBuffer;
int prodDone = 0;
int consDone = 0;

const DWORD Timeout = 10;
int store(int item, buffer* pb)
{
	EnterCriticalSection(&pb->cs);
	printf("Enter critical section for storing\n");

	while(pb->writeable == false){
		printf("wait for storing \n");
		SleepConditionVariableCS(&pb->rReady, &pb->cs, INFINITE);
		printf("wake for storing \n");
	}
	
	pb->sharedData = item;
	//Sleep(1000);
	pb->writeable = false; //!pb->writeable;
	WakeConditionVariable(&pb->mReady);
	LeaveCriticalSection(&pb->cs);

	return 0;
}

int retrieve(buffer* pb)
{
	int data;
	EnterCriticalSection(&pb->cs);
	printf("Enter critical section for retrieving\n");
	while(pb->writeable == true){
		printf("wait for retrieving \n");
		SleepConditionVariableCS(&pb->mReady, &pb->cs, INFINITE);
		printf("wake for retrieving \n");
	}
	data = pb->sharedData;
	Sleep(10);
	pb->writeable = true ;//!pb->writeable;
	WakeConditionVariable(&pb->rReady);
	LeaveCriticalSection(&pb->cs);

	return data;
}

void delay(int secs)
{
	time_t beg = time(NULL), end = beg + secs;
	do {
		;
	}while(time(NULL) < end);
}

void* producer(void* n)
{
	int j = 1;
	while (j <= 1000) {
		store(j, &theBuffer);
		
		printf("Stored: %d [writeable: %d]\n", j, theBuffer.writeable);

		j++;
		//delay(rand() % 6);
		//Sleep(1000*(rand() % 6));
	}
	prodDone = 1;
	return n;
}
void* consumer(void* n) 
{
	int j = 0;
	int tot = 0;

	while(j < 1000)
	{
		j = retrieve(&theBuffer);
		tot += j;
		printf("Retrieved: %d [writeable: %d]\n", j, theBuffer.writeable);
		//delay(rand() % 6);
		//Sleep(1000*(rand() % 6));
	}
	printf("Retrieved total = %d; press Enter\n", tot);
	consDone = 1;
	return (n);
}

int main()
{
	DWORD prodThrdID, consThrdID;
	theBuffer.sharedData = 0;
	theBuffer.writeable = true;
	srand(time(NULL));

	InitializeCriticalSection(&theBuffer.cs);
	InitializeConditionVariable(&theBuffer.rReady);
	InitializeConditionVariable(&theBuffer.mReady);

	_beginthreadex(NULL, 0, (unsigned int(__stdcall*) (void*)) producer, 0, 0, (unsigned int*)&prodThrdID);
	_beginthreadex(NULL, 0, (unsigned int(__stdcall*) (void*)) consumer, 0, 0, (unsigned int*)&consThrdID);

	while (!prodDone || !consDone);

	getchar();

	return 0;
}

#endif
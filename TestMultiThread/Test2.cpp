//#define TEST2_ON

#ifdef TEST2_ON
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <process.h>

typedef struct{
	HANDLE mutex;
	HANDLE rReady;
	HANDLE mReady;
	int sharedData;
} buffer;

buffer theBuffer;
int prodDone = 0;
int consDone = 0;

int store(int item, buffer* pb)
{
	WaitForSingleObject(pb->rReady, INFINITE);
	WaitForSingleObject(pb->mutex, INFINITE);
	__try{
		pb->sharedData = item;
		SetEvent(pb->mReady);
	}
	__finally{
		ReleaseMutex(pb->mutex);
	}
	return 0;
}

int retrieve(buffer* pb)
{
	int data;
	WaitForSingleObject(pb->mReady, INFINITE);
	WaitForSingleObject(pb->mutex, INFINITE);
	__try{
		data = pb->sharedData;
		SetEvent(pb->rReady);
	}
	__finally{
		ReleaseMutex(pb->mutex);
	}
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
	while (j <= 10) {
		store(j, &theBuffer);
		printf("Stored: %d \n" , j);
		j++;
		delay(rand() % 10);
	}
	prodDone = 1;
	return n;
}
void* consumer(void* n) 
{
	int j = 0;
	int tot = 0;
	SetEvent(theBuffer.rReady);
	while(j < 10)
	{
		j = retrieve(&theBuffer);
		tot += j;
		printf("Retrieved: %d\n", j);
		delay(rand() % 10);
	}
	printf("Retrieved total = %d; press Enter\n", tot);
	consDone = 1;
	return (n);
}

int main()
{
	DWORD prodThrdID, consThrdID;
	theBuffer.sharedData = 0;
	srand(time(NULL));

	theBuffer.mutex  = CreateMutex(NULL, FALSE, NULL);
	theBuffer.rReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	theBuffer.mReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	_beginthreadex(NULL, 0, (unsigned int(__stdcall*) (void*)) producer, 0, 0, (unsigned int*)&prodThrdID);
	_beginthreadex(NULL, 0, (unsigned int(__stdcall*) (void*)) consumer, 0, 0, (unsigned int*)&consThrdID);
	
	while (!prodDone || !consDone);

	getchar();

	return 0;
}

#endif
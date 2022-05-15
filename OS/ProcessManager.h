#pragma once
using namespace std;
#include<list>
#include<queue>
#include<string>
#include<Windows.h>

static bool isClockRunning = true;

class BasicCommand
{
public:
	char command;//指令类型
	int time;//占用CPU或外设时间（以指令周期为单位）
	BasicCommand();
	BasicCommand(char command, int time);
	~BasicCommand();
private:

};

class FileCommand : public BasicCommand
{
public:
	string fileName;
	FileCommand();
	FileCommand(char command, int time, string fileName);
	~FileCommand();
private:

};

class MemoryCommand : public BasicCommand
{
public:
	int size;
	int memoryId;
	MemoryCommand();
	MemoryCommand(char command, int time, int size, int memoryId);
	~MemoryCommand();
private:

};


class TaskInfo
{
public:
	int pid;
	int waitingTime;//已经等待调度的时钟周期数
	list<BasicCommand*> commandList;//指令序列
	int timeDiff;//CPU占用时间与IO占用时间之差
	TaskInfo();
	TaskInfo(string processName);
	~TaskInfo();
	void addTime();
private:
	static int pidCount;//记录当前可分配的pid值
};


class MemoryRecord
{
public:
	int mark;//内存块标识符
	int addr;//起始位置
	int size;//大小
	MemoryRecord();
	MemoryRecord(int mark, int addr, int size);
	~MemoryRecord();
private:
};


class PCB
{
public:
	enum State {
		running,
		waiting,
		ready,
		swappedWaiting,
		swappedReady
	};
	int pid;
	int createTime;//进程已经创建的时钟周期数
	State state;
	list<BasicCommand*>::iterator PC;//当前正在执行的指令序号
	int runningTime;//当前指令已执行时间
	list<BasicCommand*> commandList;//指令序列
	list<MemoryRecord> memoryList;//申请内存序列

	PCB(int pid, list<BasicCommand*>& commandList, int timeDiff);
	~PCB();
	void addTime();
private:
	int timeDiff;//CPU占用时间与IO占用时间之差
};

class ProcessScheduler
{
	friend DWORD WINAPI clockFunc(LPVOID lpParam);
public:
	int submit(string fileName);
	void cancel(int pid);
	list<PCB*> processInfo();//返回所有PCB信息
	double cpuUtilizationRate();//返回CPU利用率
	ProcessScheduler();
	~ProcessScheduler();
private:
	const static int CLOCK_TERM = 500;//时钟周期长度，单位毫秒
	const static int CPU_TERM_MAX = 50;//最多记录CPU_TERM_MAX个时间片的CPU使用状态
	const static int LONG_TERM_THRESHOLD = 25;//长期调度等待时限
	const static int MAX_PROCESS = 20;//最多同时运行的进程数
	const static int RR_TERM = 2;//RR轮转时间片长度

	list<TaskInfo> backBuffer;//后备缓冲池
	list<PCB*> readyQueue;//就绪队列
	list<PCB*> waitingQueue;//阻塞队列
	list<PCB*> swappedReadyQueue;//就绪挂起队列
	list<PCB*> swappedWaitingQueue;//就绪阻塞队列
	PCB* runningProcess;//当前正在占用CPU的进程
	queue<bool> utilQueueCPU;//记录CPU使用状态
	int currentUtilCPU;//在固定周期内，CPU的占用时间片个数
	int longTermDifference;//CPU时间与IO时间之差
	HANDLE hClockThread;//时钟线程句柄
	int runningTime;//当前进程已占用CPU时间，用于短期调度
	int processCount;//当前进程数，用于长期调度

	bool longTermScheduling();//长期调度，当一个进程执行完毕后调用，或者道数不足时调用，返回值为真时有新的进程调度入内存
	void mediumTermScheduling();//中期调度，申请内存不足时调用，或内存释放时调用
	void shortTermScheduling();//短期调度，进程需切换时调用

	void requestForIO(PCB* requester);//申请IO操作
};

DWORD WINAPI clockFunc(LPVOID lpParam);//时钟周期处理函数
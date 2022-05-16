#pragma once
using namespace std;
#include<list>
#include<queue>
#include<string>
#include<Windows.h>

static bool isClockRunning = true;//����ϵͳ�˳�ǰ��false�����ڹر�ʱ���߳�

class BasicCommand
{
public:
	char command;//ָ������
	int time;//ռ��CPU������ʱ�䣨��ָ������Ϊ��λ��
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
	int waitingTime;//�Ѿ��ȴ����ȵ�ʱ��������
	list<BasicCommand*> commandList;//ָ������
	int timeDiff;//CPUռ��ʱ����IOռ��ʱ��֮��
	TaskInfo();
	TaskInfo(int pid);
	TaskInfo(string processName);
	~TaskInfo();
	void addTime();

	bool operator==(const TaskInfo& taskInfo) const;
private:
	static int pidCount;//��¼��ǰ�ɷ����pidֵ
};


class MemoryRecord
{
public:
	int mark;//�ڴ���ʶ��
	int addr;//��ʼλ��
	int size;//��С
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
	int createTime;//�����Ѿ�������ʱ��������
	State state;
	list<BasicCommand*>::iterator PC;//��ǰ����ִ�е�ָ�����
	int runningTime;//��ǰָ����ִ��ʱ��
	list<BasicCommand*> commandList;//ָ������
	list<MemoryRecord> memoryList;//�����ڴ�����

	PCB(int pid, list<BasicCommand*>& commandList, int timeDiff);
	~PCB();
	void addTime();
private:
	int timeDiff;//CPUռ��ʱ����IOռ��ʱ��֮��
};

class ProcessScheduler
{
	friend DWORD WINAPI clockFunc(LPVOID lpParam);
public:
	int submit(string fileName);
	void cancel(int pid);
	list<PCB*> processInfo();//��������PCB��Ϣ
	double cpuUtilizationRate();//����CPU������
	ProcessScheduler();
	~ProcessScheduler();
private:
	const static int CLOCK_TERM = 500;//ʱ�����ڳ��ȣ���λ����
	const static int CPU_TERM_MAX = 50;//����¼CPU_TERM_MAX��ʱ��Ƭ��CPUʹ��״̬
	const static int LONG_TERM_THRESHOLD = 25;//���ڵ��ȵȴ�ʱ��
	const static int MAX_PROCESS = 20;//���ͬʱ���еĽ�����
	const static int RR_TERM = 2;//RR��תʱ��Ƭ����

	list<TaskInfo> backBuffer;//�󱸻����
	list<PCB*> readyQueue;//��������
	list<PCB*> waitingQueue;//��������
	list<PCB*> swappedReadyQueue;//�����������
	list<PCB*> swappedWaitingQueue;//������������
	PCB* runningProcess;//��ǰ����ռ��CPU�Ľ���
	queue<bool> utilQueueCPU;//��¼CPUʹ��״̬
	int currentUtilCPU;//�ڹ̶������ڣ�CPU��ռ��ʱ��Ƭ����
	int longTermDifference;//CPUʱ����IOʱ��֮��
	HANDLE hClockThread;//ʱ���߳̾��
	int runningTime;//��ǰ������ռ��CPUʱ�䣬���ڶ��ڵ���
	int processCount;//��ǰ�����������ڳ��ڵ���
	int RR_currentExecuted;//RR�Ѿ�ִ�е�ʱ��Ƭ��

	void longTermScheduling();//���ڵ��ȣ���һ������ִ����Ϻ���ã����ߵ�������ʱ���ã�����ֵΪ��ʱ���µĽ��̵������ڴ�
	void mediumTermScheduling();//���ڵ��ȣ������ڴ治��ʱ���ã����ڴ��ͷ�ʱ����
	void shortTermScheduling();//���ڵ��ȣ��������л�ʱ����

	void requestForIO(PCB* requester);//����IO����
};

DWORD WINAPI clockFunc(LPVOID lpParam);//ʱ�����ڴ�����
#include "ProcessManager.h"
#include<fstream>
#include<iostream>

int TaskInfo::pidCount = 0;

BasicCommand::BasicCommand() {}

BasicCommand::BasicCommand(char command, int time) {
	this->command = command;
	this->time = time;
}

BasicCommand::~BasicCommand() {}

FileCommand::FileCommand() {}

FileCommand::FileCommand(char command, int time, string fileName) {
	this->command = command;
	this->time = time;
	this->fileName = fileName;
}

FileCommand::~FileCommand() {}

MemoryCommand::MemoryCommand() {}

MemoryCommand::MemoryCommand(char command, int time, int size, int memoryId) {
	this->command = command;
	this->time = time;
	this->size = size;
	this->memoryId = memoryId;
}

MemoryCommand::~MemoryCommand() {}

TaskInfo::TaskInfo() {}

TaskInfo::TaskInfo(string processName) {
	ifstream ifs(processName, ios::in);
	if (!ifs.is_open()) {
		cout << "文件打开失败" << endl;
		pid = -1;
		return;
	}
	pid = pidCount++;
	waitingTime = 0;
	int timeCPU = 0;
	int timeIO = 0;
	char buf[64];
	while (ifs >> buf) {
		int time;
		switch (buf[0]) {
		case 'C'://CPU指令
			time = atoi(&buf[2]);
			commandList.push_back(new BasicCommand('C', time));
			timeCPU += time;
			break;
		case 'P'://打印机指令
			time = atoi(&buf[2]);
			commandList.push_back(new BasicCommand('P', time));
			timeIO += time;
			break;
		default:
			break;
		}
	}
	timeDiff = timeCPU - timeIO;
}

TaskInfo::~TaskInfo() {}

void TaskInfo::addTime() {
	waitingTime++;
}
MemoryRecord::MemoryRecord() {}

MemoryRecord::MemoryRecord(int mark, int addr, int size) {
	this->mark = mark;
	this->addr = addr;
	this->size = size;
}

MemoryRecord::~MemoryRecord() {}

PCB::PCB(int pid, list<BasicCommand*>& commandList, int timeDiff) {
	this->pid = pid;
	createTime = 0;
	PC = commandList.begin();
	runningTime = 0;
	this->commandList = commandList;
	this->timeDiff = timeDiff;
}

PCB::~PCB() {
	for (list<BasicCommand*>::iterator it = commandList.begin(); it != commandList.end(); ++it) {
		delete (*it);
	}
}

void PCB::addTime() {
	createTime++;
}

int ProcessScheduler::submit(string fileName) {
	return 0;
}

void ProcessScheduler::cancel(int pid) {}

list<PCB*> ProcessScheduler::processInfo() {
	list<PCB*> result;//将所有PCB集中到result链表中并赋予正确的状态
	runningProcess->state = PCB::running;
	result.push_back(runningProcess);
	for (list<PCB*>::iterator it = readyQueue.begin(); it != readyQueue.end(); ++it) {
		(*it)->state = PCB::ready;
		result.push_back(*it);
	}
	for (list<PCB*>::iterator it = waitingQueue.begin(); it != waitingQueue.end(); ++it) {
		(*it)->state = PCB::waiting;
		result.push_back(*it);
	}
	for (list<PCB*>::iterator it = swappedReadyQueue.begin(); it != swappedReadyQueue.end(); ++it) {
		(*it)->state = PCB::swappedReady;
		result.push_back(*it);
	}
	for (list<PCB*>::iterator it = swappedWaitingQueue.begin(); it != swappedWaitingQueue.end(); ++it) {
		(*it)->state = PCB::swappedWaiting;
		result.push_back(*it);
	}
	return result;
}

double ProcessScheduler::cpuUtilizationRate() {
	int recordSize = utilQueueCPU.size();
	double rate = (double)currentUtilCPU / (double)recordSize;
	return rate;
}

ProcessScheduler::ProcessScheduler() {
	currentUtilCPU = 0;
	runningProcess = nullptr;
	processCount = 0;
	currentUtilCPU = 0;
	longTermDifference = 0;
	//时钟
	hClockThread = CreateThread(NULL, 0, clockFunc, this, 0, NULL);
}

ProcessScheduler::~ProcessScheduler() {
	//释放所有进程资源
	for (list<PCB*>::iterator it = readyQueue.begin(); it != readyQueue.end(); ++it) {
		delete (*it);
	}
	for (list<PCB*>::iterator it = waitingQueue.begin(); it != waitingQueue.end(); ++it) {
		delete (*it);
	}
	for (list<PCB*>::iterator it = swappedReadyQueue.begin(); it != swappedReadyQueue.end(); ++it) {
		delete (*it);
	}
	for (list<PCB*>::iterator it = swappedWaitingQueue.begin(); it != swappedWaitingQueue.end(); ++it) {
		delete (*it);
	}
	if (runningProcess != nullptr) {
		delete runningProcess;
	}
	isClockRunning = false;

	//时钟线程
	WaitForSingleObject(hClockThread, INFINITE);
	CloseHandle(hClockThread);
}

bool ProcessScheduler::longTermScheduling() {
	if (backBuffer.size() > 0) {//仍有程序等待成为进程
		TaskInfo choosingTask;
		int waitingTime = backBuffer.front().waitingTime;
		if (waitingTime > LONG_TERM_THRESHOLD) {//超过等待时限
			choosingTask = backBuffer.front();
			backBuffer.pop_front();
		} else {
			int minDiff = 10000;
			list<TaskInfo>::iterator pos = backBuffer.begin();
			for (list<TaskInfo>::iterator it = backBuffer.begin(); it != backBuffer.end(); ++it) {//选择最能均衡CPU占用时间和IO占用时间的进程进入内存
				if (abs(longTermDifference + it->timeDiff) < abs(minDiff)) {
					pos = it;
					minDiff = longTermDifference + it->timeDiff;
				}
			}
			choosingTask = *pos;
			backBuffer.erase(pos);
		}
		longTermDifference += choosingTask.timeDiff;//更新CPU占用时间与IO占用时间之差
		PCB* newPCB = new PCB(choosingTask.pid, choosingTask.commandList, choosingTask.timeDiff);//分配PCB，正式进入内存成为进程
		switch (choosingTask.commandList.front()->command) {
		case 'C'://第一个指令是CPU操作
			readyQueue.push_back(newPCB);//放入就绪队列
			break;
		case 'P'://第一个指令是IO操作
			requestForIO(newPCB);//申请IO
			break;
		default:
			break;
		}
		processCount++;
		return true;
	}
	return false;
}

void ProcessScheduler::mediumTermScheduling() {
	//TODO: 优先选择阻塞状态的进程进行挂起
}

void ProcessScheduler::shortTermScheduling() {
	//TODO: 根据RR算法，选择就绪队列中第一个进程占用CPU
}

void ProcessScheduler::requestForIO(PCB* requester) {
	waitingQueue.push_back(requester);
	//TODO: 申请IO
}

DWORD WINAPI clockFunc(LPVOID lpParam) {
	ProcessScheduler* ptr = (ProcessScheduler*)lpParam;
	//TODO: 调度

	Sleep(ProcessScheduler::CLOCK_TERM);
	while (isClockRunning){
		//进行时钟周期的处理
		//更改所有用于记录时间的变量
		for (list<TaskInfo>::iterator it = ptr->backBuffer.begin(); it != ptr->backBuffer.end(); ++it) {
			it->addTime();
		}
		for (list<PCB*>::iterator it = ptr->readyQueue.begin(); it != ptr->readyQueue.end(); ++it) {
			(*it)->addTime();
		}
		for (list<PCB*>::iterator it = ptr->waitingQueue.begin(); it != ptr->waitingQueue.end(); ++it) {
			(*it)->addTime();
		}
		for (list<PCB*>::iterator it = ptr->swappedReadyQueue.begin(); it != ptr->swappedReadyQueue.end(); ++it) {
			(*it)->addTime();
		}
		for (list<PCB*>::iterator it = ptr->swappedWaitingQueue.begin(); it != ptr->swappedWaitingQueue.end(); ++it) {
			(*it)->addTime();
		}
		if (ptr->runningProcess != nullptr) {
			ptr->runningProcess->addTime();
		}
		
		//记录及更新CPU占用状况
		if (ptr->utilQueueCPU.size() > ptr->CPU_TERM_MAX) {//如果记录个数超过CPU_TERM_MAX，弹出最早记录
			ptr->currentUtilCPU -= ptr->utilQueueCPU.front();
			ptr->utilQueueCPU.pop();
		}
		if (ptr->runningProcess != nullptr) {//CPU被占用
			ptr->utilQueueCPU.push(true);
			ptr->currentUtilCPU++;
		} else {
			ptr->utilQueueCPU.push(false);
		}

		if (ptr->runningProcess != nullptr) {//有需要执行的指令
			PCB* pcbPtr = ptr->runningProcess;
			switch ((*pcbPtr->PC)->command) {//具体指令的执行
				//TODO: 执行指令
			default:
				break;
			}
		}
		
		//TODO: 判断是否需要进行调度

		Sleep(ProcessScheduler::CLOCK_TERM);
	}
	return 0;
}

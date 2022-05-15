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
		cout << "�ļ���ʧ��" << endl;
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
		case 'C'://CPUָ��
			time = atoi(&buf[2]);
			commandList.push_back(new BasicCommand('C', time));
			timeCPU += time;
			break;
		case 'P'://��ӡ��ָ��
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
	list<PCB*> result;//������PCB���е�result�����в�������ȷ��״̬
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
	//ʱ��
	hClockThread = CreateThread(NULL, 0, clockFunc, this, 0, NULL);
}

ProcessScheduler::~ProcessScheduler() {
	//�ͷ����н�����Դ
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

	//ʱ���߳�
	WaitForSingleObject(hClockThread, INFINITE);
	CloseHandle(hClockThread);
}

bool ProcessScheduler::longTermScheduling() {
	if (backBuffer.size() > 0) {//���г���ȴ���Ϊ����
		TaskInfo choosingTask;
		int waitingTime = backBuffer.front().waitingTime;
		if (waitingTime > LONG_TERM_THRESHOLD) {//�����ȴ�ʱ��
			choosingTask = backBuffer.front();
			backBuffer.pop_front();
		} else {
			int minDiff = 10000;
			list<TaskInfo>::iterator pos = backBuffer.begin();
			for (list<TaskInfo>::iterator it = backBuffer.begin(); it != backBuffer.end(); ++it) {//ѡ�����ܾ���CPUռ��ʱ���IOռ��ʱ��Ľ��̽����ڴ�
				if (abs(longTermDifference + it->timeDiff) < abs(minDiff)) {
					pos = it;
					minDiff = longTermDifference + it->timeDiff;
				}
			}
			choosingTask = *pos;
			backBuffer.erase(pos);
		}
		longTermDifference += choosingTask.timeDiff;//����CPUռ��ʱ����IOռ��ʱ��֮��
		PCB* newPCB = new PCB(choosingTask.pid, choosingTask.commandList, choosingTask.timeDiff);//����PCB����ʽ�����ڴ��Ϊ����
		switch (choosingTask.commandList.front()->command) {
		case 'C'://��һ��ָ����CPU����
			readyQueue.push_back(newPCB);//�����������
			break;
		case 'P'://��һ��ָ����IO����
			requestForIO(newPCB);//����IO
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
	//TODO: ����ѡ������״̬�Ľ��̽��й���
}

void ProcessScheduler::shortTermScheduling() {
	//TODO: ����RR�㷨��ѡ����������е�һ������ռ��CPU
}

void ProcessScheduler::requestForIO(PCB* requester) {
	waitingQueue.push_back(requester);
	//TODO: ����IO
}

DWORD WINAPI clockFunc(LPVOID lpParam) {
	ProcessScheduler* ptr = (ProcessScheduler*)lpParam;
	//TODO: ����

	Sleep(ProcessScheduler::CLOCK_TERM);
	while (isClockRunning){
		//����ʱ�����ڵĴ���
		//�����������ڼ�¼ʱ��ı���
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
		
		//��¼������CPUռ��״��
		if (ptr->utilQueueCPU.size() > ptr->CPU_TERM_MAX) {//�����¼��������CPU_TERM_MAX�����������¼
			ptr->currentUtilCPU -= ptr->utilQueueCPU.front();
			ptr->utilQueueCPU.pop();
		}
		if (ptr->runningProcess != nullptr) {//CPU��ռ��
			ptr->utilQueueCPU.push(true);
			ptr->currentUtilCPU++;
		} else {
			ptr->utilQueueCPU.push(false);
		}

		if (ptr->runningProcess != nullptr) {//����Ҫִ�е�ָ��
			PCB* pcbPtr = ptr->runningProcess;
			switch ((*pcbPtr->PC)->command) {//����ָ���ִ��
				//TODO: ִ��ָ��
			default:
				break;
			}
		}
		
		//TODO: �ж��Ƿ���Ҫ���е���

		Sleep(ProcessScheduler::CLOCK_TERM);
	}
	return 0;
}

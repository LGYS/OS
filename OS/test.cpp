#include"ProcessManager.h"
#include<iostream>

void test1();

int main(void) {
	test1();

	return 0;
}

void test1() {//TaskInfo:pidµÄ·ÖÅä¡¢addTime()
	TaskInfo taskE("e.txt");
	TaskInfo task1("1.txt");
	TaskInfo task2("2.txt");
	TaskInfo* ptr = new TaskInfo("3.txt");
	cout << taskE.pid << endl;
	cout << task1.pid << endl;
	cout << task2.pid << endl;
	cout << ptr->pid << endl;
	cout << task1.waitingTime << endl;
	task1.addTime();
	cout << task1.waitingTime << endl;
	delete ptr;
}

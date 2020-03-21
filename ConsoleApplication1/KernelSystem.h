#pragma once
#include "System.h"
#include <map>
#include "KernelProcess.h"
#include <mutex>
#include <vector>


class Partition;
class Process;



class KernelSystem {



public:
	
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
		PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
		Partition* partition, System* mySystem=nullptr);
	~KernelSystem();
	Process* createProcess();
//	PhysicalAddress allocateSpace1(int size); 
	PhysicalAddress allocateSpace(int size);
	void deallocateSpace(PhysicalAddress addr, int size);
	
	int getReplacementFrame();
	void test();

	Time periodicJob() { return 0; }
	// Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);
	unsigned long* getFreeClusters(int num);
	void setFreeClusters(unsigned long* array, int size);

private:
	
	int fifoCNT = 0;
	std::mutex mtxSpace;
	std::mutex mtxClusters;
	std::mutex  mtxframes;
	Slot* space;
	Slot* first;
	bool* clusters;
	int ID = 1;
	int freeSpace;
	Descriptor** frames;
	std::map<ProcessId, KernelProcess*> processHash;	
	PhysicalAddress processVMSpace;
	PageNum processVMSpaceSize;
	PhysicalAddress pmtSpace;
	PageNum pmtSpaceSize;
	int freeClusters;
	Partition* myPartition;
	System *mySystem;
	std::vector<bool> clock;
	int clockHand = 0;
	
	friend class Process;
	friend class KernelProcess;
};

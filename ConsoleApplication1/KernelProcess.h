#pragma once
#include "vm_declarations.h"
#include "Process.h"

class System;
class KernelSystem;
class KernelProcess {
public:
	KernelProcess(ProcessId pid, Process* myProcess);
	~KernelProcess();
	
	bool isSecondTableEmpty(Descriptor** table);
	bool isThirdTableEmpty(Descriptor * table);
	void setSystem(KernelSystem* sys) { mySystem = sys; }
	void setPMT(Descriptor*** pmt) { this->pmt = pmt; }
	Descriptor*** getPMT() { return pmt; }
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessType flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessType flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
private:
	int numOfSeg = 0;
	int segmentID = 1;
	PageNum numOfPages;
	Descriptor*** pmt;
	KernelSystem* mySystem;
	ProcessId id;
	Process *myProcess;
	friend class System;
	friend class KernelSystem;
};
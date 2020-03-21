#include "KernelProcess.h"
#include "KernelSystem.h"
#include "part.h"
#include <iostream>

KernelProcess::KernelProcess(ProcessId pid, Process * myProcess)
{
	this->myProcess = myProcess;
	id = pid;
	numOfPages = 0;
}

KernelProcess::~KernelProcess()
{
	mySystem->processHash.erase(getProcessId());
	int segments = numOfSeg;
	unsigned long first;
	unsigned long	 second;
	int deletedSegments = 0;
	PageNum page = 0;
	while ((page < FIRST_TABLE_SIZE*SECOND_TABLE_SIZE*FIRST_TABLE_SIZE)&& (deletedSegments<segments))
	{
		first = page >> 9;
		second = (page>>4)  & 31;
		
		if (pmt[first] == nullptr) {
			page += THIRD_TABLE_SIZE*SECOND_TABLE_SIZE;
			continue;
		}
		if (pmt[first][second] == nullptr) {
			page = page + THIRD_TABLE_SIZE;
			continue;
		}
		for (int j=0; j < THIRD_TABLE_SIZE; j++) {
			if (pmt[first][second][j].flags&Sflag) {
				VirtualAddress adr = (page+j) << 10;
				deleteSegment(adr);
				if ((++deletedSegments == segments)||(pmt[first]==nullptr)||(pmt[first][second]==nullptr)) break;
			}
		}
		page+=THIRD_TABLE_SIZE;
	}
		pmt = nullptr;
	}

bool KernelProcess::isSecondTableEmpty(Descriptor ** table)
{
	for (int i = 0; i < SECOND_TABLE_SIZE; i++)if (table[i] != nullptr) return false;
	return true;
}

bool KernelProcess::isThirdTableEmpty(Descriptor * table)
{
	for (int i = 0; i < THIRD_TABLE_SIZE; i++)if (table[i].flags != 0)return false;
	return true;
	
}

ProcessId KernelProcess::getProcessId() const
{
	return id;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags)
{
	if (startAddress&WORD)return Status::ALIGNMENT;
	unsigned long* clusters = mySystem->getFreeClusters(segmentSize);
	if (clusters == nullptr)return Status::NO_CLUSTERS;
	PageNum page = startAddress >> 10;
	int checkedPages = 0;
	unsigned long first;
	unsigned long second;
	unsigned long third;
	while (checkedPages < segmentSize) {
		first = page >> 9;
		second = (page >> 4) & 31;
		third = page&15;
		page++;
		if (pmt[first] == nullptr) {
			checkedPages += SECOND_TABLE_SIZE*THIRD_TABLE_SIZE;
			continue;
		}
		if (pmt[first][second] == nullptr) {
			checkedPages += THIRD_TABLE_SIZE;
			continue;
		}
		if (pmt[first][second][third].flags != 0) {
			mySystem->setFreeClusters(clusters, segmentSize);
			delete[] clusters;
			return Status::OVERLAP;
		}
		checkedPages++;


	}
	int segment = segmentID++;
	unsigned int tableSize3 = THIRD_TABLE_SIZE*(sizeof(Descriptor) / sizeof(Slot));
	if (sizeof(Descriptor) % sizeof(Slot))tableSize3 += THIRD_TABLE_SIZE;
	int tableSize2 = SECOND_TABLE_SIZE*(sizeof(Descriptor*) / sizeof(Slot));
	if (sizeof(Descriptor*) % sizeof(Slot))tableSize2 += SECOND_TABLE_SIZE;
	page = startAddress >> 10;
	for (int i = 0; i < segmentSize; i++, page++) {
		first = page >> 9;
		second = (page >> 4) & 31;
		third = page&15;
		if (pmt[first] == nullptr) {
			Descriptor** pmt2 = reinterpret_cast<Descriptor**>(mySystem->allocateSpace(tableSize2));
			if (pmt2 == nullptr) {
				mySystem->setFreeClusters(clusters, segmentSize);
				delete[] clusters;
				if (i == 0)return Status::NO_SPACE;
				else {
					deleteSegment(startAddress);
					return Status::NO_SPACE;
				}
			}
			pmt[first] = pmt2;
			for (int j = 0; j < SECOND_TABLE_SIZE; j++)pmt2[j] = nullptr;

		}
		if (pmt[first][second] == nullptr) {
			Descriptor* pmt3 = reinterpret_cast<Descriptor*>(mySystem->allocateSpace(tableSize3));
			if (pmt3 == nullptr) {
				mySystem->setFreeClusters(clusters, segmentSize);
				delete[] clusters;
				if (i == 0)return Status::NO_SPACE;
				else {
					deleteSegment(startAddress);
					return Status::NO_SPACE;
				}
			}
			pmt[first][second] = pmt3;
			for (int j = 0; j < THIRD_TABLE_SIZE; j++)pmt3[j].flags = 0;

		}
		if (i == 0)		pmt[first][second][third].flags |= Sflag;

		pmt[first][second][third].flags |= Cflag;
		pmt[first][second][third].cluster = clusters[i];
		pmt[first][second][third].segment = segment;
		switch (flags) {
		case AccessType::EXECUTE:		pmt[first][second][third].flags |= Xflag;
			break;
		case AccessType::READ:pmt[first][second][third].flags |= Rflag;
			break;
		case AccessType::READ_WRITE:pmt[first][second][third].flags |= Wflag;
			pmt[first][second][third].flags |= Rflag;
			break;
		case AccessType::WRITE:pmt[first][second][third].flags |= Wflag;
			break;
		}
	}




	delete[] clusters;
	numOfPages += segmentSize;
	numOfSeg++;
	return Status::OK;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void * content)
{
	if (startAddress&WORD)return Status::ALIGNMENT;
	unsigned long* clusters = mySystem->getFreeClusters(segmentSize);
	if (clusters == nullptr)return Status::NO_CLUSTERS;
	PageNum page = startAddress >> 10;
	int checkedPages = 0;
	unsigned long first;
	unsigned long second;
	unsigned long third;
	while (checkedPages < segmentSize) {
		first = page >> 9;
		second = (page >> 4) & 31;
		third = page&15;
		page++;
		if (pmt[first] == nullptr) {
			checkedPages += SECOND_TABLE_SIZE*THIRD_TABLE_SIZE;
			continue;
		}
		if (pmt[first][second] == nullptr) {
			checkedPages += THIRD_TABLE_SIZE;
			continue;
		}
		if (pmt[first][second][third].flags != 0) {
			mySystem->setFreeClusters(clusters, segmentSize);
			delete[] clusters;
			return Status::OVERLAP;
		}
		checkedPages++;


	}
	int segment = segmentID++;
	int tableSize3 = THIRD_TABLE_SIZE*(sizeof(Descriptor) / sizeof(Slot));
	if (sizeof(Descriptor) % sizeof(Slot))tableSize3 += THIRD_TABLE_SIZE;
	int tableSize2 = SECOND_TABLE_SIZE*(sizeof(Descriptor*) / sizeof(Slot));
	if (sizeof(Descriptor*) % sizeof(Slot))tableSize2 += SECOND_TABLE_SIZE;
	page = startAddress >> 10;
	for (int i = 0; i < segmentSize; i++, page++) {
		first = page >> 9;
		second = (page >> 4) & 31;
		third = page&15;
		if (pmt[first] == nullptr) {
			Descriptor** pmt2 = reinterpret_cast<Descriptor**>(mySystem->allocateSpace(tableSize2));
			if (pmt2 == nullptr) {
				mySystem->setFreeClusters(clusters, segmentSize);
				delete[] clusters;
				if (i == 0)return Status::NO_SPACE;
				else {
					deleteSegment(startAddress);
					return Status::NO_SPACE;
				}
			}
			pmt[first] = pmt2;
			for (int j = 0; j < SECOND_TABLE_SIZE; j++)pmt2[j] = nullptr;

		}
		if (pmt[first][second] == nullptr) {
			Descriptor* pmt3 = reinterpret_cast<Descriptor*>(mySystem->allocateSpace(tableSize3));
			if (pmt3 == nullptr) {
				mySystem->setFreeClusters(clusters, segmentSize);
				delete[] clusters;
				if (i == 0)return Status::NO_SPACE;
				else {
					deleteSegment(startAddress);
					return Status::NO_SPACE;
				}
			}
			pmt[first][second] = pmt3;
			for (int j = 0; j < THIRD_TABLE_SIZE; j++)pmt3[j].flags = 0;

		}
		if (i == 0)		pmt[first][second][third].flags |= Sflag;

		pmt[first][second][third].flags |= Cflag;
		pmt[first][second][third].cluster = clusters[i];
		pmt[first][second][third].segment = segment;
		switch (flags) {
		case AccessType::EXECUTE:		pmt[first][second][third].flags |= Xflag;
			break;
		case AccessType::READ:pmt[first][second][third].flags |= Rflag;
			break;
		case AccessType::READ_WRITE:pmt[first][second][third].flags |= Wflag;
			pmt[first][second][third].flags |= Rflag;
			break;
		case AccessType::WRITE:pmt[first][second][third].flags |= Wflag;
			break;
		default:std::cout << "Prava pristupa nisu bila dobra" << std::endl;
		}
	}

	char* buffer = reinterpret_cast<char*>(content);
	for (int i = 0; i < segmentSize; i++,buffer+=PAGE_SIZE) 	mySystem->myPartition->writeCluster(clusters[i], buffer);
	
	delete[] clusters;
	numOfSeg++;
	numOfPages += segmentSize;
	return Status::OK;
	
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{



	if (pmt == nullptr)return Status::NULL_SIZE;
	if (startAddress&WORD)return Status::ALIGNMENT;
	PageNum  page = startAddress >> 10;
	unsigned long first = page >> 9;
	Descriptor** pmt2 = pmt[first];
	if (pmt2 == nullptr) return Status::NULL_SIZE;
	unsigned  long second = (page >> 4) & 31; //izdvajam prvih pet bitova
	Descriptor* pmt3 = pmt2[second];
	if (pmt3 == nullptr) return Status::NULL_SIZE;
	unsigned long third = page & 15; 
	if (!pmt3[third].flags&Sflag) return Status::NOT_FIRST;
	int segSize = 0;
	unsigned long segment = pmt3[third].segment;
	
	while (pmt[first][second][third].segment == segment) {
		segSize++;
		page++;
		first = page >> 9;
		second = (page >> 4) & 31; //izdvajam prvih pet bitova
		third = page & 15;
		if (pmt[first] == nullptr)break;
		if (pmt[first][second] == nullptr)break;
	}
	int tableSize3 = THIRD_TABLE_SIZE*(sizeof(Descriptor) / sizeof(Slot));//	
	if (sizeof(Descriptor) % sizeof(Slot))tableSize3 += THIRD_TABLE_SIZE;//	Odredjujem velicnu tabela za preslikavanje
	int tableSize2 = SECOND_TABLE_SIZE*(sizeof(Descriptor*) / sizeof(Slot));// jer ona moja metoda prima broj koji odredjuje 
	if (sizeof(Descriptor*) % sizeof(Slot))tableSize2 += SECOND_TABLE_SIZE;// velicinu u slotovima ne bajtovima
	page = startAddress >> 10;
	unsigned long * clusters = new unsigned long [segSize];
	for (int i = 0; i < segSize; i++) {
		first = page >> 9;
		second = (page >> 4) & 31;
		third = page & 15;
		page++;
		mySystem->mtxframes.lock();//ovde treba da dodje neki mutex
		if (pmt[first][second][third].flags&Vflag)mySystem->frames[pmt[first][second][third].frame] = nullptr;
		mySystem->mtxframes.unlock();//i ovde
		
		clusters[i] = pmt[first][second][third].cluster;
		pmt[first][second][third].flags = 0;
		if ((third == 15 || i == (segSize - 1))&&isThirdTableEmpty(pmt[first][second])) {
			mySystem->deallocateSpace(pmt[first][second], tableSize3);
			pmt[first][second] = nullptr;
			if (isSecondTableEmpty(pmt[first])) {
				mySystem->deallocateSpace(pmt[first], tableSize2);
				pmt[first] = nullptr;

			}
		}
	}

	mySystem->setFreeClusters(clusters, segSize);
	delete[] clusters;
	numOfPages -= segSize;
	numOfSeg--;
	return Status::OK;

}

Status KernelProcess::pageFault(VirtualAddress address)
{
	PageNum page = address >> 10;
	unsigned long first = page >> 9;
	unsigned long second = (page >> 4) & 31;
	unsigned long third = page & 15;
	if (pmt[first] == nullptr) return Status::ERROR;
	if (pmt[first][second] == nullptr) return Status::ERROR;
	if (!(pmt[first][second][third].flags & Cflag)) return Status::ERROR;
	int frame = -1;
	mySystem->mtxframes.lock();
	for (int i = 0; i < mySystem->processVMSpaceSize; i++)if (mySystem->frames[i] == nullptr) {
		frame = i;
		mySystem->frames[i] = pmt[first][second] + third;
		break;
		}
		mySystem->mtxframes.unlock();
	if (frame != -1) {
		pmt[first][second][third].flags |= Vflag;
		pmt[first][second][third].frame = frame;
		if (pmt[first][second][third].flags&Dflag)pmt[first][second][third].flags -= Dflag;
		char* buffer =static_cast<char*>( mySystem->processVMSpace) + PAGE_SIZE*frame;
		mySystem->myPartition->readCluster(pmt[first][second][third].cluster, buffer);
		return Status::OK;

	}
	else {
		mySystem->mtxframes.lock();
		frame = mySystem->getReplacementFrame();
		char* buffer = static_cast<char*>(mySystem->processVMSpace) + PAGE_SIZE*frame;
		Descriptor* desc = mySystem->frames[frame];
		if (desc->flags&Vflag)desc->flags -= Vflag;
		if (desc->flags&Dflag) mySystem->myPartition->writeCluster(desc->cluster, buffer);
		mySystem->frames[frame] = pmt[first][second] + third;
		pmt[first][second][third].flags |= Vflag;
		if (pmt[first][second][third].flags&Dflag)pmt[first][second][third].flags -= Dflag;
		pmt[first][second][third].frame = frame;

		mySystem->myPartition->readCluster(pmt[first][second][third].cluster, buffer);


		mySystem->mtxframes.unlock();
		return Status::OK;
	}



	
}

PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{



	unsigned first = (address&FRAME1) >> 19;
	Descriptor** pmt2 = pmt[first];
	if (pmt2 == nullptr) return nullptr;
	unsigned second = (address&FRAME2) >> 14;
	Descriptor* pmt3 = pmt2[second];
	if (pmt3 == nullptr) return nullptr;
	unsigned third = (address&FRAME3) >> 10;
	if (!(pmt3[third].flags&Cflag))return nullptr;
	if (!(pmt3[third].flags&Vflag))return nullptr;
	unsigned  long long frame = (pmt3[third].frame<<10+address&WORD);
	char* physicalAddr = static_cast<char*>(mySystem->processVMSpace) + frame;
	return physicalAddr;
	

}

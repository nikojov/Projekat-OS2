#include "KernelSystem.h"
#include "vm_declarations.h"
#include "part.h"
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition * partition, System* mySystem)
{
	//provera da li imamo dovoljno prostora da smestimo neophodne strukture
	if (PAGE_SIZE*pmtSpaceSize <= sizeof(bool)*partition->getNumOfClusters() + sizeof(Descriptor*)*processVMSpaceSize)
	{
		std::cout << "Nedovoljno prostora za neophodne strukture" << std::endl;
		std::exit(1);
	}
	
	
	this->processVMSpace = processVMSpace;
	this->processVMSpaceSize = processVMSpaceSize;
	this->pmtSpace = pmtSpace;
	this->pmtSpaceSize = pmtSpaceSize;
	this->mySystem = mySystem;
	myPartition = partition;
	clock =std::vector<bool>(processVMSpaceSize);
	for (auto i = clock.begin(); i != clock.end(); i++)*i = false;
	frames = static_cast<Descriptor**>(pmtSpace);
	
	clusters = reinterpret_cast<bool*>(frames + processVMSpaceSize);
	for (ClusterNo i = 0; i < myPartition->getNumOfClusters(); i++)clusters[i] = false;
	
	space=first = reinterpret_cast<Slot*>(clusters + myPartition->getNumOfClusters());

	first->free=freeSpace=(PAGE_SIZE*pmtSpaceSize-sizeof(bool)*myPartition->getNumOfClusters() - sizeof(Descriptor*)*processVMSpaceSize)/sizeof(Slot);
	first->next = 0;


	for (PageNum i = 0; i < processVMSpaceSize; i++)frames[i] = nullptr;


	int flagsInCluster = PAGE_SIZE / sizeof(int);// Mislim da mi ovo nije potrebno 
	
	//ClusterNo clusters = myPartition->getNumOfClusters();
	
	

//	partSem = new std::mutex[indexClusters];
	freeClusters = myPartition->getNumOfClusters();
}

KernelSystem::~KernelSystem()
{
	myPartition = nullptr;
	mySystem = nullptr;
	/*for (auto iter = processHash.begin(); iter != processHash.end();++iter) {
		delete iter->second;
		processHash.erase(iter);
	}*/
	//delete[] partSem;
}

Process * KernelSystem::createProcess()
{
	int size = ( FIRST_TABLE_SIZE * sizeof(Descriptor**) ) / sizeof(Slot);

	//if (sizeof(Descriptor**) % sizeof(Slot))	size+=FIRST_TABLE_SIZE; // koja je logika ove linije??
	if ((FIRST_TABLE_SIZE * sizeof(Descriptor**)) % sizeof(Slot)) size += sizeof(Slot);	//nova verzija

	Descriptor*** pmt = static_cast<Descriptor***>(allocateSpace(size));
	if (pmt == nullptr) return nullptr;
	for (int i = 0; i < FIRST_TABLE_SIZE; i++)pmt[i] = nullptr;



	Process* pro = new Process(ID++);
	pro->pProcess->setSystem(this);
	pro->pProcess->setPMT(pmt);
	processHash.insert(std::make_pair(pro->getProcessId(), pro->pProcess));
	return pro;







}


PhysicalAddress KernelSystem::allocateSpace(int size) {
	if (size < 1) return nullptr; // nevalidan parametar
	mtxSpace.lock();
	if (first == nullptr||freeSpace<size) {
		mtxSpace.unlock();
		return nullptr;
	}

	void* address = nullptr;
	Slot *curr = first, *help = nullptr;
	/*
	while (size > curr->free) {
		if (curr->next != 0) {
			help = curr;
			curr = space + curr->next;
		}
		else {
			curr = nullptr;
			break;
		}

	}
	if (curr == nullptr) {
		mtxSpace.unlock();
		return nullptr;
	}
	Nova verzija petlje*/
	while (curr->free < size &&  curr->next != 0) {
		help = curr;
		curr = space + curr->next;
	}
	if (curr->free < size) {
		mtxSpace.unlock();
		return nullptr;
	}
	 
	address = curr;
	freeSpace = freeSpace - size;
	if (size == curr->free) {
		if (help == nullptr) {
			if(curr->next==0)first = nullptr;
			else first = space + curr->next;
		}
		else {
			help->next = curr->next;
		}
		mtxSpace.unlock();
		return address;
	}

	if(size<curr->free) {

		if (help == nullptr) {
			help = curr + size;
			help->next = curr->next;
			help->free = curr->free - size;
			first = help;

		}
		else {

			Slot* help2 = curr + size;
			help2->free = curr->free - size;
			help2->next = curr->next;
			help->next = help2 - space;
		}
		mtxSpace.unlock();
		return address;

	}


}

void KernelSystem::deallocateSpace(PhysicalAddress addr, int size)
{
	if (size < 1 || addr==nullptr) return;
	mtxSpace.lock();
	Slot* empty = static_cast<Slot*>(addr);
	if (first == nullptr) {
		empty->free = size;
		empty->next = 0;
		first = empty;
		
		freeSpace += size;
		mtxSpace.unlock();
		return;
	}

	if (first > empty) {
		if ((empty + size) == first) {
			empty->next = first->next;
			empty->free = size + first->free;
			

		}
		else {
			empty->next = first - space;
			empty->free = size;
			}

		first = empty;
		
		freeSpace += size;
		mtxSpace.unlock();
		return;
	}

	if (first < empty) {
		Slot* help = first, *help2 = nullptr;
		while (help < empty && help->next != 0) {
			help2 = help;
			help = space + help->next;
		}
		if (help < empty) {
			if ((help + help->free) == empty) {
				help->free += size;
					}
			else {
				help->next = empty - space;
				empty->free = size;
				empty->next = 0;
				
			}

			
			freeSpace += size;
			mtxSpace.unlock();
			return;
		}
		else {

			if ((help2 + help2->free) == empty && (empty + size) == help) { //ako se nalazi izmedju dva slobodna parceta memorije

				help2->free = help2->free + size + help->free;
				help2->next = help->next;

				
				freeSpace += size;
				mtxSpace.unlock();
				return;
			}
			if ((help2 + help2->free) == empty) {//ako je samo odozgo slobodno parce memorije
				help2->next = help - space;//MISLIM DA SE OVDEE DESAVAAA GRESKAA!!! UMESTO EMPTY TREBA SPACE
				help2->free += size;

				
				freeSpace += size;
				mtxSpace.unlock();
				return;
			}
			if ((empty + size) == help) {//ako je samo odozdo slobodno parce memorije
				help2->next = empty - space;
				empty->free = size + help->free;
				empty->next = help->next;
				
				freeSpace += size;
				mtxSpace.unlock();
				return;
			}
			help2->next = empty - space;
			empty->free = size;
			empty->next = help - space;
			
			freeSpace += size;
			mtxSpace.unlock();
			return;//u slucaju da je s obe strane zauzeta memorija
		}


	}
	std::cout << "GRESKA U DEALOKACIJI" << std::endl;
	mtxSpace.unlock();
	return;

}

Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	Descriptor*** pmt1=	processHash.find(pid)->second->getPMT();
	unsigned long first = (address&FRAME1) >> 19;
	Descriptor** pmt2=pmt1[first];
	if (pmt2 == nullptr) return Status::PAGE_FAULT;
	unsigned long second = (address&FRAME2) >> 14;
	Descriptor* pmt3 = pmt2[second];
	if (pmt3 == nullptr) return Status::PAGE_FAULT;
	unsigned long third = (address&FRAME3) >> 10;
	if (!(pmt3[third].flags&Cflag))return Status::PAGE_FAULT;
	if (!(pmt3[third].flags&Vflag))return Status::PAGE_FAULT;
	switch (type) {
		

	case AccessType::EXECUTE:if (pmt3[third].flags & Xflag) { mtxframes.lock(); clock[address >> 10] = true;
		mtxframes.unlock();
		return Status::OK; }
							 else return Status::ACCESS;

	case AccessType::READ_WRITE:if (pmt3[third].flags & Rflag && pmt3[third].flags & Wflag) {
		mtxframes.lock(); clock[address >> 10] = true;
		pmt3[third].flags |= Dflag;
		mtxframes.unlock(); 
		return Status::OK; }
								else return Status::ACCESS;
	case AccessType::READ:if (pmt3[third].flags & Rflag) {
		mtxframes.lock(); clock[address >> 10] = true;
		mtxframes.unlock(); return Status::OK; }
						  else return Status::ACCESS;
	case AccessType::WRITE:if (pmt3[third].flags & Wflag) {
		mtxframes.lock(); clock[address >> 10] = true;
		pmt3[third].flags |= Dflag;

		mtxframes.unlock();
		return Status::OK;
	}
						   else return Status::ACCESS;
	default:std::cout << "Greska u parametru type funkcije access" << std::endl;
		exit(1);




	}
}
int KernelSystem::getReplacementFrame()
{
/* int x = fifoCNT; fifoCNT = (fifoCNT + 1) % processVMSpaceSize; return x;*/
	while (true) {
		if (clock[clockHand] == false) {
			int x = clockHand;
			clockHand = (clockHand + 1) % processVMSpaceSize;
			return x;
		}
		else {
			clock[clockHand] = false;
			clockHand = (clockHand + 1) % processVMSpaceSize;

		}

	}

}
void KernelSystem::test() {

	//adadasdsadasdasdas



}
unsigned long * KernelSystem::getFreeClusters(int num) //ko zpve ovu funkciju treba da zatvori niz koji ona vraca jer je alociran sa new
{
	if (num < 1) return nullptr;
	mtxClusters.lock();

	if (freeClusters < num) {
		mtxClusters.unlock();
		return nullptr;
	}
	freeClusters -= num;
	unsigned long * array = new unsigned long[num];
	int size = 0;
	for (ClusterNo i = 0; i < myPartition->getNumOfClusters(); i++) {

		if (!clusters[i]) {
			array[size++] = i; clusters[i] = true;
		}
		if (size == num)break;
	}
	if (size == num) {
		mtxClusters.unlock();
		return array;
	}
	else {

		mtxClusters.unlock();
		std::cout << "NISU PRONADJENI KLASTERI IAKO SU POSTOJALI" << std::endl;
		return nullptr;
	}
}

void KernelSystem::setFreeClusters(unsigned long * array, int size)// pretpostavlja se da je niz neopadajuce uredjen
{
	mtxClusters.lock();
	
	for (unsigned long i = 0; i < size; i++) {
		clusters[array[i]] = false;
	}
	freeClusters += size;
	mtxClusters.unlock();
}

#pragma once
#include <limits>


typedef unsigned long PageNum;
typedef unsigned long VirtualAddress;
typedef void* PhysicalAddress;
typedef unsigned long Time;
typedef unsigned long AccessRight;
enum Status { OK, PAGE_FAULT, TRAP, ACCESS,ALIGNMENT,NULL_SIZE,NOT_FIRST ,NO_CLUSTERS,NO_SPACE,OVERLAP,ERROR};
enum AccessType { READ, WRITE, READ_WRITE, EXECUTE };
typedef unsigned ProcessId;
#define PAGE_SIZE 1024 
const VirtualAddress   WORD = 0x0003ff;
const VirtualAddress FRAME1 = 0xf80000;// pmt tabela prvog nivoa ima 32 ulaza
const VirtualAddress FRAME2 = 0x07C000;// drugog nivoa 32
const VirtualAddress FRAME3 = 0x003C00;// i svaka tabela ima 16 deskriptora
const int END_OF_CLUSTER_INDEX = INT_MAX;
#define FIRST_TABLE_SIZE 32
#define SECOND_TABLE_SIZE 32
#define THIRD_TABLE_SIZE 16
const unsigned char Vflag = 1;
const unsigned char Dflag = 2;
const unsigned char Cflag = 4; // oznacava da li je stranica alocirana uopste
const unsigned char Rflag = 8;
const unsigned char Wflag = 16;
const unsigned char Xflag = 32;
const unsigned char Sflag = 64; //oznacava da je ova stranica pocetak segmenta
struct Descriptor  {
	unsigned char flags = 0;
	unsigned long frame = 0;
	unsigned long cluster;
	unsigned long segment;
};

/*
struct FreeSlots {
	int size;
	FreeSlots* next = nullptr;
};*/
struct Slot {
	unsigned long free;
	unsigned long next;
};
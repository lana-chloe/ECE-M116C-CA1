#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <tuple>
using namespace std;


// Opcodes
const bitset<7> OPCODE_R_TYPE(0x33); 	// R-type
const bitset<7> OPCODE_I_TYPE(0x13); 	// I-type
const bitset<7> OPCODE_LOAD(0x03);   	// Load
const bitset<7> OPCODE_STORE(0x23);  	// Store
const bitset<7> OPCODE_BRANCH(0x63);	// Branch
const bitset<7> OPCODE_LUI(0x37);    	// LUI
const bitset<7> OPCODE_J(0x6F);      	// JAL
const bitset<7> OPCODE_DEFAULT(0x00); 	// Default
// ALU Operations
const bitset<4> ALU_OP_ADD(0x2);     	// 0010: ADD
const bitset<4> ALU_OP_SUB(0x6);	 	// 0110: SUBTRACT
const bitset<4> ALU_OP_XOR(0x0);     	// 0000: XOR
const bitset<4> ALU_OP_OR(0x1);      	// 0001: OR
const bitset<4> ALU_OP_LUI(0x7);     	// 0111: LUI
const bitset<4> ALU_OP_SRAI(0x5);    	// 0101: SRAI
const bitset<4> ALU_OP_DEFAULT(0x8); 	// 1000: Default


class ControlUnit {
public:
	ControlUnit(bitset<7> opcode, bitset<3> funct3, bitset<7> funct7);
	ControlUnit() { // default constructor
		branch = 0;
		memRead = 0;
		memToReg = 0;
		aluOp = bitset<4>(0);
		memWrite = 0;
		aluSrc = 0;
		regWrite = 0;
		memSize = 0;
		jump = 0;
	}

	bitset<4> aluOpControl(bitset<3> funct3, bitset<7> funct7);
	bitset<1> memSizeControl(bitset<3> funct3);
	
	bitset<1> branch;
	bitset<1> memRead;
	bitset<1> memToReg;
	bitset<4> aluOp;
	bitset<1> memWrite;
	bitset<1> aluSrc;
	bitset<1> regWrite;
	bitset<1> memSize; // 0 for byte, 1 for word
	bitset<1> jump; // 0 for no jump, 1 for jump
	// add more later...
};


class Instruction {
public:
	Instruction(bitset<32> fetch);
	bitset<7> getOpcode() const;
    bitset<5> getRS1() const;
    bitset<5> getRS2() const;
    bitset<5> getRD() const;
	bitset<32> getImmediate() const;
	bitset<3> getFunct3() const;
	bitset<7> getFunct7() const;

	bitset<32> instr; 
};


class CPU {
public:
	CPU(char imem[4096]);
	unsigned long readPC();
	void incPC();
	void setPC(unsigned long newPC);
	bitset<32> readRegister(unsigned long regNum);
	void writeRegister(int regNum, bitset<32> value);

	bitset<32> instructionFetch();
	void instructionDecode(Instruction instr);
	void executeInstruction();
	void memory();
	void writeBack();

private:
	bitset<8> dmemory[4096]; 
	char imemory[4096]; 
	unsigned long PC;
	vector<bitset<32> > registers;

	ControlUnit control;
	bitset<32> rs1Value;
	bitset<32> rs2Value;
	bitset<5> rd;
	bitset<32> immValue;
	bitset<32> aluResult;
	bitset<32> dataMemValue;
};

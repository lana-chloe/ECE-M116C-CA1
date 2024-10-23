#include "CPU.h"
#include <tuple>

//////////////////////
// HELPER FUNCTIONS //
bitset<32> hexToBin(char hex[9]) {
    bitset<32> instruction;
    for (int i = 0; i < 8; i++) {
        uint8_t value;
        if (hex[i] >= '0' && hex[i] <= '9') {
            value = hex[i] - '0';
        } else if (hex[i] >= 'A' && hex[i] <= 'F') {
            value = hex[i] - 'A' + 10;
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            value = hex[i] - 'a' + 10;
        } else {
            cerr << "Invalid hex character: " << hex[i] << endl;
            exit(1);
        }
        instruction |= (value & 0xF) << ((7 - i) * 4);
    }
    return instruction;
}


///////////////////////
// INSTRUCTION CLASS //
Instruction::Instruction(bitset<32> fetch) {
	instr = fetch;
}
bitset<7> Instruction::getOpcode() const {
    return bitset<7>((instr.to_ulong() >> 0) & 0x7F); // Extract bits 6-0
}
bitset<5> Instruction::getRS1() const {
    return bitset<5>((instr.to_ulong() >> 15) & 0x1F); // Extract bits 19-15
}
bitset<5> Instruction::getRS2() const {
    return bitset<5>((instr.to_ulong() >> 20) & 0x1F); // Extract bits 24-20
}
bitset<5> Instruction::getRD() const {
    return bitset<5>((instr.to_ulong() >> 7) & 0x1F); // Extract bits 11-7
}
bitset<32> Instruction::getImmediate() const {
    bitset<7> opcode = getOpcode();
	if (opcode == OPCODE_R_TYPE) {
		return bitset<32>(0); // No immediate value
	} 
	else if (opcode == OPCODE_I_TYPE || opcode == OPCODE_LOAD) {
		// Do sign extension
		bitset<12> immValue = bitset<12>((instr.to_ulong() >> 20) & 0xFFF); // Extract bits 31-20
		bitset<32> extendedImmValue;
		if (immValue[11] == 1) { // Check the sign bit (bit 11)
        	extendedImmValue = bitset<32>(immValue.to_ulong() | 0xFFFFF000); // Fill upper 20 bits with 1s
		} else {
			extendedImmValue = bitset<32>(immValue.to_ulong()); // Fill upper 20 bits with 0s
		}
		return extendedImmValue;
	} 
	else if (opcode == OPCODE_LUI) {
		return bitset<32>((instr.to_ulong() >> 12) & 0xFFFFF); // Extract bits 31-12
	}
	/*
	else if (opcode == OPCODE_STORE) {
		return bitset<12>((instr.to_ulong() >> 7) & 0xFFF); // Extract bits 11-0
	} 
	else if (opcode == OPCODE_BRANCH) {
		return bitset<12>((instr.to_ulong() >> 7) & 0xFFF); // Extract bits 11-0
	} 
	*/
	else {
		cerr << "Invalid opcode: " << opcode << endl;
		exit(1);
	}
}
bitset<3> Instruction::getFunct3() const {
	return bitset<3>((instr.to_ulong() >> 12) & 0x7); // Extract bits 14-12
}
bitset<7> Instruction::getFunct7() const {
	return bitset<7>((instr.to_ulong() >> 25) & 0x7F); // Extract bits 31-25
}


///////////////
// CPU CLASS //
CPU::CPU(char imem[4096])
{
	// Data memory
	for (int i = 0; i < 4096; i++) {
		dmemory[i] = 0;
	}

	// Instruction memory
	for (int i = 0; i < 4096; i++) {
		imemory[i] = imem[i];
	}

	// Registers
	for (int i = 0; i < 32; i++) {
		registers.push_back(bitset<32>(0));
	}

	PC = 0;
	control = ControlUnit();
	rs1Value = bitset<32>(0);
	rs2Value = bitset<32>(0);
	rd = bitset<5>(0);
	immValue = bitset<32>(0);
	aluResult = bitset<32>(0);
	dataMemValue = bitset<32>(0);
}
unsigned long CPU::readPC() {
	return PC;
}
void CPU::incPC() {
	PC++;
}
void CPU::setPC(unsigned long newPC) {
	PC = newPC;
}
bitset<32> CPU::readRegister(unsigned long regNum) {
	return registers[regNum];
}
void CPU::writeRegister(int regNum, bitset<32> value) {
	registers[regNum] = value;
}
bitset<32> CPU::instructionFetch() {
	char hex[9];
	for (int i = 7; i >= 0; i-=2) {
        hex[i-1] = imemory[readPC()];
		hex[i] = imemory[readPC()+1];
		incPC();
		incPC();
    }
	hex[8] = '\0';
	
	bitset<32> instr = hexToBin(hex);

	//cout << hex << endl;
	//cout << instr << endl;
	//cout << "_______________________" << endl;

	return instr; 
}
void CPU::instructionDecode(Instruction instr) {
	control = ControlUnit(instr.getOpcode(), instr.getFunct3(), instr.getFunct7());
	rs1Value = readRegister(instr.getRS1().to_ulong());
	rs2Value = readRegister(instr.getRS2().to_ulong());
	rd = instr.getRD();
	immValue = instr.getImmediate();
}
void CPU::executeInstruction() {
	//ControlUnit control = ControlUnit(instr.getOpcode());
	//bitset<32> rs1Value = readRegister(instr.getRS1().to_ulong());
	//bitset<32> rs2Value = readRegister(instr.getRS2().to_ulong());
	//bitset<12> immValue = instr.getImmediate();

	//cout << "rs1Value: " << rs1Value << endl;
	//cout << "rs2Value: " << rs2Value << endl;
	//cout << "immValue: " << immValue << endl;

	// ALU source
	bitset<32> aluSrcValue;
	if (control.aluSrc == 0) {
		aluSrcValue = rs2Value;
	} else {
		aluSrcValue = immValue;
	}

	// ALU
	if (control.aluOp == ALU_OP_ADD) {
		aluResult = rs1Value.to_ulong() + aluSrcValue.to_ulong();
	} 
	else if (control.aluOp == ALU_OP_SUB) {
		aluResult = rs1Value.to_ulong() - aluSrcValue.to_ulong();
	} 
	else if (control.aluOp == ALU_OP_LUI) {
        aluResult = bitset<32>(immValue.to_ulong() << 12);
	} 
	else if (control.aluOp == ALU_OP_OR) {
		aluResult = rs1Value | aluSrcValue;
	} 
	else if (control.aluOp == ALU_OP_XOR) {
		aluResult = rs1Value ^ aluSrcValue;
	} 
	else if (control.aluOp == ALU_OP_SRAI) {
		aluResult = rs1Value.to_ulong() >> aluSrcValue.to_ulong();
	}
	else {
		cerr << "Invalid ALU operation: " << control.aluOp << endl;
		exit(1);
	}

	cout << "aluResult: " << aluResult.to_ulong() << endl;
}
void CPU::memory() {

}
void CPU::writeBack() {
	if (control.memToReg == 0) {
		if (control.regWrite == 1) {
			writeRegister(rd.to_ulong(), aluResult);
		}
	} else {
		if (control.regWrite == 1) {
			writeRegister(rd.to_ulong(), dataMemValue);
		}
	}
}

////////////////////
// CONTROL CLASS //
ControlUnit::ControlUnit(bitset<7> opcode, bitset<3> funct3, bitset<7> funct7) {
	if (opcode == OPCODE_R_TYPE) { // ADD, XOR
		regWrite = 1;
		aluSrc = 0;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;

		aluOp = aluOpControl(funct3, funct7);
	}
	else if (opcode == OPCODE_I_TYPE) { // ORI, SRAI
		regWrite = 1;
		aluSrc = 1;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;

		aluOp = aluOpControl(funct3, funct7);
	}
	else if (opcode == OPCODE_LUI) { // LUI
        regWrite = 1;
        aluSrc = 1;
        branch = 0;
        memRead = 0;
        memWrite = 0;
        memToReg = 0;

        aluOp = ALU_OP_LUI; 
    }
	else if (opcode == OPCODE_LOAD) { // Load
		regWrite = 1;
		aluSrc = 1;
		branch = 0;
		memRead = 1;
		memWrite = 0;
		memToReg = 1;

		aluOp = ALU_OP_ADD; 
	}
	else if (opcode == OPCODE_STORE) { // Store
		regWrite = 0;
		aluSrc = 1;
		branch = 0;
		memRead = 0;
		memWrite = 1;
		memToReg = 0;

		aluOp = ALU_OP_ADD; // add
	}
	else if (opcode == OPCODE_BRANCH) { // Branch
		regWrite = 0;
		aluSrc = 0;
		branch = 1;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;

		aluOp = ALU_OP_SUB;
	}
	else if (opcode == OPCODE_DEFAULT) { // Default
		regWrite = 0;
		aluSrc = 0;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;

		aluOp = ALU_OP_DEFAULT;
	}
	else {
		cerr << "Invalid opcode: " << opcode << endl;
		exit(1);
	}
	// add more later...
}
bitset<4> ControlUnit::aluOpControl(bitset<3> funct3, bitset<7> funct7) {
	if (funct3 == bitset<3>(0x6)) {				// ORI
		return ALU_OP_OR;
	}
	else if (funct3 == bitset<3>(0x5)) {		// SRAI
		return ALU_OP_SRAI;
	}

	if (funct7 == bitset<7>(0)) {			
		if (funct3 == bitset<3>(0x0)) {			// ADD
			return ALU_OP_ADD;
		} 
		else if (funct3 == bitset<3>(0x4)) {	// XOR
			return ALU_OP_XOR;
		}
		else {
			cerr << "Invalid funct3: " << funct3 << endl;
			exit(1);
		}
	}
	else {
		cerr << "Invalid funct7: " << funct7 << endl;
		exit(1);
	}
}
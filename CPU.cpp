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
int32_t bitsetToSignedInt(const bitset<32>& bs) {
    // Check the sign bit (bit 31)
    if (bs[31] == 1) {
        // If the sign bit is set, convert to signed integer by interpreting the bitset as a negative number
        return static_cast<int32_t>(bs.to_ulong() | 0xFFFFFFFF00000000);
    } else {
        // If the sign bit is not set, convert to signed integer directly
        return static_cast<int32_t>(bs.to_ulong());
    }
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
		// Sign extension
		bitset<12> immValue = bitset<12>((instr.to_ulong() >> 20) & 0xFFF); // Extract bits 31-20
		bitset<32> extendedImmValue;
		if (immValue[11] == 1) { // Check the sign bit 
        	extendedImmValue = bitset<32>(immValue.to_ulong() | 0xFFFFF000); // Fill upper 20 bits with 1s
		} else {
			extendedImmValue = bitset<32>(immValue.to_ulong()); // Fill upper 20 bits with 0s
		}
		//cout << "Immediate: " << immValue << endl;
		//cout << "Extended Immediate: " << extendedImmValue << endl;
		// print as signed integer
		//cout << "Extended Immediate (signed): " <<  bitsetToSignedInt(extendedImmValue) << endl;
		return extendedImmValue;
	} 
	else if (opcode == OPCODE_STORE) {
		// Extract bits 11-5 and 4-0
		bitset<12> immValue = bitset<12>(((instr.to_ulong() >> 7) & 0x1F) | ((instr.to_ulong() >> 25) & 0xFE0));
		bitset<32> extendedImmValue;
		// Sign extension
		if (immValue[11] == 1) { 
			extendedImmValue = bitset<32>(immValue.to_ulong() | 0xFFFFF000); 
		} else {
			extendedImmValue = bitset<32>(immValue.to_ulong());
		}
		return extendedImmValue;
	}
	else if (opcode == OPCODE_LUI) {
		return bitset<32>((instr.to_ulong() >> 12) & 0xFFFFF); // Extract bits 31-12
	}
	else if (opcode == OPCODE_BRANCH) {
		// Extract bits 11, 4-1, 10-5
		bitset<12> immValue = bitset<12>(((instr.to_ulong() >> 31) & 0x1) | ((instr.to_ulong() >> 7) & 0x1E) | ((instr.to_ulong() >> 25) & 0x3F) << 1);
		bitset<32> extendedImmValue;
		// Sign extension
		if (immValue[11] == 1) { 
			extendedImmValue = bitset<32>(immValue.to_ulong() | 0xFFFFF000); 
		} else {
			extendedImmValue = bitset<32>(immValue.to_ulong());
		}
		return extendedImmValue;
	}
	else if (opcode == OPCODE_J) {
		// Extract bits 20, 10-1, 11, 19-12
		bitset<21> immValue = bitset<21>(((instr.to_ulong() >> 31) & 0x1) << 20 | // Bit 20
										((instr.to_ulong() >> 21) & 0x3FF) << 1 | // Bits 10-1
										((instr.to_ulong() >> 20) & 0x1) << 11 | // Bit 11
										((instr.to_ulong() >> 12) & 0xFF) << 12); // Bits 19-12
		// Sign-extend the immediate value
		if (immValue[20] == 1) { // Check the sign bit (bit 20)
			return bitset<32>(immValue.to_ulong() | 0xFFE00000); // Fill upper 11 bits with 1s
		} else {
			return bitset<32>(immValue.to_ulong()); // Fill upper 11 bits with 0s
		}
	}
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
		dmemory[i] = bitset<8>(0);
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
		aluResult = rs1Value.to_ulong() + bitsetToSignedInt(aluSrcValue);
	} 
	else if (control.aluOp == ALU_OP_SUB) {
		aluResult = rs1Value.to_ulong() - bitsetToSignedInt(aluSrcValue);
	} 
	else if (control.aluOp == ALU_OP_LUI) {
        aluResult = bitset<32>(immValue.to_ulong() << 12);
	} 
	else if (control.aluOp == ALU_OP_OR) {
		//cout << "OR" << endl;
		//cout << rs1Value << endl;
		//cout << aluSrcValue << endl;
		aluResult = rs1Value | aluSrcValue;
	} 
	else if (control.aluOp == ALU_OP_XOR) {
		aluResult = rs1Value ^ aluSrcValue;
	} 
	else if (control.aluOp == ALU_OP_SRAI) {
		//cout << "SRAI" << endl;
		//cout << rs1Value.to_ulong() << " >> " << aluSrcValue.to_ulong() << endl;
		// Sign extend
		if (rs1Value[31] == 1) {
			aluResult = (rs1Value.to_ulong() >> aluSrcValue.to_ulong()) | (0xFFFFFFFF << (32 - aluSrcValue.to_ulong()));
		} else {
			aluResult = rs1Value.to_ulong() >> aluSrcValue.to_ulong();
		}
	}
	else if (control.aluOp == ALU_OP_DEFAULT) {
		return;
	}
	else {
		cerr << "Invalid ALU operation: " << control.aluOp << endl;
		exit(1);
	}

	//

	// Branch
	if (control.branch == 1) {
		//cout << "Branching" << endl;
		//cout << "rs1Value: " << rs1Value.to_ulong() << endl;
		//cout << "rs2Value: " << rs2Value.to_ulong() << endl;
		if (aluResult == 0) {
			//cout << "immValue: " << bitsetToSignedInt(immValue) << endl; 
			//cout << "Old PC: " << PC << endl;
			PC = ((PC - 8)/2 + bitsetToSignedInt(immValue)) * 2; // Adjust for the next instruction
			
			
			//PC + immValue.to_ulong() - 4; // Adjust for the next instruction
			//cout << "New PC: " << PC << endl;
		}
	}

	//cout << "aluResult: " << bitsetToSignedInt(immValue) << endl;
}
void CPU::memory() {
	// Branch
	if (control.branch == 1) {
		return;
	}

	unsigned long address = aluResult.to_ulong();
	if (control.memWrite == 1) { // Store
        if (control.memSize == 1) { // SW
			cout << "Store word" << endl;
            for (int i = 0; i < 4; ++i) {
				//cout << "Writing to address: " << address + i << " value: " << ((rs2Value.to_ulong() >> (i * 8)) & 0xFF) << endl;
                dmemory[address + i] = (rs2Value.to_ulong() >> (i * 8)) & 0xFF;
            }
        } else if (control.memSize == 0) { // SB
			//cout << "Store byte" << endl;
			//cout << "Writing to address: " << address << " value: " << (rs2Value.to_ulong() & 0xFF) << endl;
            dmemory[address] = rs2Value.to_ulong() & 0xFF;
        }
    }
	else if (control.memRead == 1) { // Load
		if (control.memSize == 1) { // LW
			//cout << "Load word" << endl;
			for (int i = 0; i < 4; ++i) {
				//cout << "Reading from address: " << address + i << " value: " << dmemory[address + i].to_ulong() << endl;
				dataMemValue |= (dmemory[address + i].to_ulong() & 0xFF) << (i * 8);
			}
		}
		else if (control.memSize == 0) { // LB
			//cout << "Load byte" << endl;
			//cout << "Reading from address: " << address << " value: " << dmemory[address].to_ulong() << endl;
			unsigned char byteValue = dmemory[address].to_ulong() & 0xFF;
			// Sign extension
			if (byteValue & 0x80) {
				dataMemValue = bitset<32>(byteValue | 0xFFFFFF00);
			} else {
				dataMemValue = bitset<32>(byteValue); 
			}
		}
	}
}
void CPU::writeBack() {
	// Branch and store
	if (control.memWrite == 1) {
		return;
	}

	if (control.memToReg == 0) {
		if (control.regWrite == 1) {
			if (control.jump == 1) {
				//cout << "Jumping" << endl;
				//cout << "immValue: " << bitsetToSignedInt(immValue) << endl;
				//cout << "Old PC: " << PC << endl;
				writeRegister(rd.to_ulong(), PC/2);

				PC = ((PC - 8)/2 + bitsetToSignedInt(immValue)) * 2;

			} else {
				writeRegister(rd.to_ulong(), aluResult);
			}
		}
	} 
	else { 
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
		memSize = 0;
		jump = 0;

		aluOp = aluOpControl(funct3, funct7);
	}
	else if (opcode == OPCODE_I_TYPE) { // ORI, SRAI
		regWrite = 1;
		aluSrc = 1;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;
		memSize = 0;
		jump = 0;

		aluOp = aluOpControl(funct3, funct7);
	}
	else if (opcode == OPCODE_LUI) { // LUI
        regWrite = 1;
        aluSrc = 1;
        branch = 0;
        memRead = 0;
        memWrite = 0;
        memToReg = 0;
		memSize = 0;
		jump = 0;

        aluOp = ALU_OP_LUI; 
    }
	else if (opcode == OPCODE_LOAD) { // Load
		regWrite = 1;
		aluSrc = 1;
		branch = 0;
		memRead = 1;
		memWrite = 0;
		memToReg = 1;
		jump = 0;

		memSize = memSizeControl(funct3);

		aluOp = ALU_OP_ADD; 
	}
	else if (opcode == OPCODE_STORE) { // Store
		regWrite = 0;
		aluSrc = 1;
		branch = 0;
		memRead = 0;
		memWrite = 1;
		memToReg = 0;
		jump = 0;

		memSize = memSizeControl(funct3);

		aluOp = ALU_OP_ADD; // add
	}
	else if (opcode == OPCODE_BRANCH) { // Branch
		regWrite = 0;
		aluSrc = 0;
		branch = 1;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;
		memSize = 0;
		jump = 0;

		aluOp = ALU_OP_SUB;
	}
	else if (opcode == OPCODE_J) { // JAL
		regWrite = 1;
		aluSrc = 0;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;
		memSize = 0;
		jump = 1;

		aluOp = ALU_OP_DEFAULT;
	}
	else if (opcode == OPCODE_DEFAULT) { // Default
		regWrite = 0;
		aluSrc = 0;
		branch = 0;
		memRead = 0;
		memWrite = 0;
		memToReg = 0;
		memSize = 0;
		jump = 0;

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
bitset<1> ControlUnit::memSizeControl(bitset<3> funct3) {
	if (funct3 == bitset<3>(0x0)) { // byte
		return bitset<1>(0);
	}
	else if (funct3 == bitset<3>(0x2)) { // word
		return bitset<1>(1);
	}
	else {
		cerr << "Invalid funct3 for data size (used for store/load): " << funct3 << endl;
		exit(1);
	}
}
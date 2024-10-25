#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <tuple>
using namespace std;

/*
Add all the required standard and developed libraries here
*/

/*
Put/Define any helper function/definitions you need here
*/
int main(int argc, char* argv[]) {
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

	/* Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an hex and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	*/

	char instMem[4096];


	if (argc < 2) {
		//cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream infile(argv[1]); //open the file
	if (!(infile.is_open() && infile.good())) {
		cout<<"error opening file\n";
		return 0; 
	}
	string line; 
	int i = 0;
	while (infile) {
			infile>>line;
			stringstream line2(line);
			char x; 
			line2>>x;
			instMem[i] = x; // be careful about hex
			i++;
			line2>>x;
			instMem[i] = x; // be careful about hex
			//cout<<instMem[i-1]<< instMem[i] <<endl; // prints instruction
			i++;
		}
	
	int maxPC= i-2; // THIS IS PROBABLY WRONG

	/* Instantiate your CPU object here.  CPU class is the main class in this project that defines different components of the processor.
	CPU class also has different functions for each stage (e.g., fetching an instruction, decoding, etc.).
	*/
	
	// call the approriate constructor here to initialize the processor... 
	CPU cpu = CPU(instMem); 
	// make sure to create a variable for PC and resets it to zero (e.g., unsigned int PC = 0); 
	cpu.setPC(0);
	
	bool done = true;
	while (done == true) { // processor's main loop. Each iteration is equal to one clock cycle.  

		Instruction instr = Instruction(cpu.instructionFetch()); 
	
		cpu.instructionDecode(instr);

		cpu.executeInstruction();

		cpu.memory();

		cpu.writeBack();

		if (cpu.readPC() >= maxPC)
			break;

	}
	int a0 = cpu.readRegister(10).to_ulong();
	int a1 = cpu.readRegister(11).to_ulong();  

	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	  cout << "(" << a0 << "," << a1 << ")" << endl;
	
	return 0;

}
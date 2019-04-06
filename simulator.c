/*******************************************************************************
 * Title:	HYPO Machine - MTOPS
 * Authors:     //TODO
 * Date:
 * Class :     	CSCI.465 Operating Systems Internals
 * Professor:	Suban Krishnamoorthy
 * Program Title: HYPO MACHINE SIMULATION
 * Program Description: Emulates a hypothetical processing machine called `HYPO`
 * In contrast to binary processors, HYPO is a decimal machine. This program
 * recreates the functionality of HYPO's hardware components. HYPO is comprised
 * of various components:
 *
 * MAR		: Memory Address Register
 * MBR		: Memory Buffer Register
 * RAM		: Random Access Memory
 * IR          	: Instruction Register
 * SP		: Stack Pointer
 * PC		: Program Counter
 * GPR		: General Purpose Register
 * ALU		: Arithmetic and Logic Unit
 * PSR		: Processor Status Register
 * Clock	: System clock wth time in microseconds
 *
 * A detailed description of each function can be found in this document.
 *
 *
 ******************************************************************************/

#include <stdio.h>

/*** VIRTUAL SYSTEM PARAMETERS ***/
#define SYSTEM_MEMORY_SIZE      10000
#define GPR_NUMBER              8
#define DEFAULT_PRIORITY	128
#define TIMESLICE		200


/*** VALUE CONSTANTS ***/
#define SCRIPT_INDICATOR_END	-1

// Simulator Execution Status
#define SIMULATOR_STATUS_HALTED	0
//#define SIMULATOR_STATUS_OK	1

// Machine PSR Modes
#define MACHINE_MODE_USER	0
#define MACHINE_MODE_OS		1


// Memory-Space Boundaries (non-inclusive)
#define MAX_USER_MEMORY         3999
#define MAX_HEAP_MEMORY         6999
#define MAX_OS_MEMORY         	9999

/*** ERROR CODES ***/
#define OK                      -1
#define ErrorFileOpen           -2
#define ErrorInvalidAddress     -3
#define ErrorInvalidPCValue     -4
#define ErrorNoEndOfProgram     -5
#define ErrorInvalidInstruction -6
#define ErrorInvalidOpcode      -7
#define ErrorInvalidMode        -8
#define ErrorImmediateMode      -9
#define ErrorRuntime            -10
#define ErrorStackOverflow      -11
#define ErrorStackUnderflow     -12
#define ErrorInvalidMemorySize  -13
#define ErrorNoFreeMemory       -14

/*** GLOBAL VARS ***/
long mem[SYSTEM_MEMORY_SIZE], gpr[GPR_NUMBER];
long mar, mbr, clock, ir, psr, pc, sp;
long OSFreeList, UserFreeList, RQ;
long EndOfList = -1; //indicates end of OSFreeList or UserFreeList


/*** FUNCTION PROTOTYPES ***/
void InitializeSystem();
int main();
int AbsoluteLoader(char* filename);
long CPU();
long SystemCall(long SystemCallID);
long FetchOperand(long OpMode, long OpReg, long *OpAddress, long *OpValue);
void DumpMemory(char* String, long StartAddress, long size);
long CreateProcess(char *filename, long priority);
long AllocateOSMemory(long RequestedSize);
long FreeOSMemory(long ptr, long size);
long AllocateUserMemory(long size);
long FreeUserMemory(long ptr, long size);

long MemAllocSystemCall();
long MemFreeSystemCall();


/*** FUNCTIONS ***/

/*******************************************************************************
 * Function: InitializeSystem
 *
 * Description: Resets all Global Vars (Hardware) to an initial value 0
 *
 * Input Parameters
 *      None
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      None
 ******************************************************************************/

void InitializeSystem()
{
	/* Initialize Memory Array and then GPR Register Array */
	for (int i = 0; i < SYSTEM_MEMORY_SIZE; i++)
		mem[i] = 0;
	for (int i = 0; i < GPR_NUMBER; i++)
		gpr[i] = 0;

	/* Assigns value `0` to Each Individual Hardware Variable */
	mar = mbr = clock = ir = psr = pc = sp = 0;
}

/*******************************************************************************
 * Function:Main
 *
 * Description:
 *
 * Input Parameters
 *      None
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      OK				-on successful execution
 *      ErrorFileOpen			-Cannot Open File
 *      ExecutionCompletionStatus	-Returns code returned by CPU
 ******************************************************************************/

int main()
{
	/* Local Variables */
	char filename[128];
	long ReturnValue;
	int ExecutionCompletionStatus;

	//Prompt User to load Machine Code Program
	printf("Enter Machine Code Program Filename >>");
	scanf("%127s", filename);


	// Ready System and Load File
	InitializeSystem();
	pc = AbsoluteLoader(filename);

	// Check for Negative Error Code
	if (pc < 0) {
		printf("FILE ERROR: %d\n", pc);
		return pc;
	}

	//Dump Memory, Execute, and Dump Memory
	DumpMemory("Program Loaded into System", 0, 99);
	ExecutionCompletionStatus = CPU();
	printf("Execution Status: %d\n", ExecutionCompletionStatus);
	DumpMemory("Program Execution Stopped System", 0, 99);

	return(ExecutionCompletionStatus);
}

/*******************************************************************************
 * Function: AbsoluteLoader

 * Description: Opens a file that contains HYPO-machine code (user program)
 * into the HYPO machine memory.
 * On a successful file load the function returns the value in the `End of
 * Program` line (Indicates PC Value).
 * On failure to load then the function displays the appropriate error message
 * and returns the appropriate error code.
 *
 *
 * Input Parameters
 *      filename			-Name of the Machine Code File
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      ErrorFileOpen			-Unable to open the file
 *      ErrorInvalidAddress		-Invalid address error
 *      ErrorNoEndOfProgram		-Missing end of program indicator
 *      ErrorInvalidPCValue		-Invalid PC value
 *      0 to Valid address range	-Successful Load, valid PC value
 ******************************************************************************/

int AbsoluteLoader(char* filename)
{
	// Load machine code file into HYPO memory
	FILE *fp;
	fp = fopen(filename, "r"); //open file in READ mode
	if (fp == NULL) {
		printf("ERROR: Unable to open file.\n");
		return ErrorFileOpen;
	}

	// Parse File
	int addr, word;
	while (fscanf(fp, "%d %d", &addr, &word) != EOF) {
		// If Address indicates EOP, Word is PC Value
		if (addr == SCRIPT_INDICATOR_END) {
			fclose(fp);
			if (word < SYSTEM_MEMORY_SIZE && word > 0)
				return word;                    //Success
			printf("ERROR: PC value Invalid\n");    //Error
			return ErrorInvalidPCValue;
		}else if (0 <= addr <= MAX_USER_MEMORY)
			mem[addr] = word;
		else{
			fclose(fp);
			printf("ERROR: Address Location in Invalid Range\n");
			return ErrorInvalidAddress;
		}
	}
	fclose(fp);
	printf("ERROR: No End of Program Indicator\n");         //Error
	return ErrorNoEndOfProgram;
}

/*******************************************************************************
 * Function: CPU
 *
 * Description: CPU Execution is divided into 2 phases.
 * Decode: Parses the composite instruction into constituent parts. Validation
 * is done on individual components and an error is returned should any
 * instruction component is invalid.
 * Execute: The validity of the instruction (whole and in context) is
 * performed. Error is returned on an invalid instruction. Given a valid
 * instruction, a call is made to FetchOperand() and the result of it's
 * execution defines the state of the machine status (PSR).
 *
 * Notes:
 *
 *
 * Input Parameters
 *      None
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      OK				-on successful execution
 *      psr				-Returns Error given by FetchOperand()
 *      ErrorInvalidAddress		-Payload address not valid
 *      ErrorRuntime			-Unbound address during runtime
 *      ErrorInvalidInstruction		-Instruction not valid
 *      ErrorInvalidOpcode		-Opcode not valid
 *      ErrorStackOverflow		-Attempted to allocate beyond stack
 *      ErrorStackUnderflow		-Attempted to allocate beneath stack
 ******************************************************************************/

long CPU()
{
	/* Local Variables */
	long opcode, op1mode, op1gpr, op2mode, op2gpr, op1addr, op1val,
	     op2addr, op2val, remainder, result, SystemCallID;
	long status = OK;
	long TimeLeft = TIMESLICE;


	// Run CPU until HALT state
	while (status = OK && TimeLeft > 0) {

		// Fetch Cycle
		if (0 <= pc <= MAX_USER_MEMORY) {
			mar = pc;
			pc++;
			mbr = mem[mar];
		}else  {
			printf("ERROR: Invalid Runtime Address. Line: %d "
			       "Address: %d\n", pc, mem[pc]);          // Error
			return(ErrorInvalidAddress);
		}

		ir = mbr;

		// Decode Cycle			-CAPTURE-
		opcode = ir / 10000;            //[65]4321
		remainder = ir % 10000;

		op1addr = remainder / 100;      //[43]21
		op1val = gpr[op1addr];

		op1mode = remainder / 1000;     //[4]321
		remainder = remainder % 1000;

		op1gpr = remainder / 100;       //[3]21
		remainder = remainder % 100;


		op2addr = remainder;            //[21]
		op2val = gpr[op2addr];
		op2mode = remainder / 10;       //[2]1
		remainder = remainder % 10;

		op2gpr = remainder;             //[1]


		// Decode Validation
		if (((0 <= op1mode <= 6) && (0 <= op2mode <= 6) &&
		     (0 <= op1gpr <= GPR_NUMBER) && (0 <= op2gpr <= GPR_NUMBER)) == 0) {
			printf("ERROR: Invalid Instruction on line %d\n", mar); // Error
			return ErrorInvalidInstruction;
		}


		// Execute Cycle

		switch (opcode) {
		case 0:                 //halt
			printf("Machine is Halting\n");
			return SIMULATOR_STATUS_HALTED;
			clock += 12;
			TimeLeft -= 12;
			break;
		case 1:                 //add
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			status = FetchOperand(op2mode, op2gpr, &op2addr, &op2val);
			if (status != OK) {
				return status;
			}

			//Add Operand Values
			result = op1val + op2val;

			// If op1mode is Register Mode
			if (op1mode == 1) {
				gpr[op1gpr] = result;
			}else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}else  {        //Store result in op1address
				mem[op1addr] = result;
			}
			clock += 3;
			TimeLeft -= 3;
			break;
		case 2:                 //subtract
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			status = FetchOperand(op2mode, op2gpr, &op2addr, &op2val);
			if (status != OK) {
				return status;
			}

			//Subtract Operand Values
			result = op1val - op2val;

			// If op1mode is Register Mode
			if (op1mode == 1) {
				gpr[op1gpr] = result;
			}else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}else  {        //Store result in op1address
				mem[op1addr] = result;
			}
			clock += 3;
			TimeLeft -= 3;
			break;
		case 3:                 //multiply
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			status = FetchOperand(op2mode, op2gpr, &op2addr, &op2val);
			if (status != OK) {
				return status;
			}

			//Multiply Operand Values
			result = op1val * op2val;

			// If op1mode is Register Mode
			if (op1mode == 1) {
				gpr[op1gpr] = result;
			}else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}else  {        //Store result in op1address
				mem[op1addr] = result;
			}
			clock += 6;
			TimeLeft -= 6;
			break;
		case 4:                 //divide
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			status = FetchOperand(op2mode, op2gpr, &op2addr, &op2val);
			if (status != OK) {
				return status;
			}

			//Divide Operand Values
			if (op2val != 0)		// Division by Zero Check
				result = op1val / op2val;
			else {
				printf("ERROR: Line %d Division by Zero\n", pc);
				return ErrorRuntime;
			}


			// If op1mode is Register Mode
			if (op1mode == 1) {
				gpr[op1gpr] = result;
			}else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}else  {        //Store result in op1address
				mem[op1addr] = result;
			}
			clock += 6;
			TimeLeft -= 6;
			break;
		case 5:                 //move (op1 <- op2)
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			status = FetchOperand(op2mode, op2gpr, &op2addr, &op2val);
			if (status != OK) {
				return status;
			}

			// If op1mode is Register Mode
			if (op1mode == 1) {
				gpr[op1gpr] = op2val;
			}else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}else  {        //Store result in op1address
				mem[op1addr] = op2val;
			}
			clock += 2;
			TimeLeft -= 2;
			break;
		case 6:                 //branch
			if (0 <= pc <= MAX_USER_MEMORY)
				pc = mem[pc];
			else{
				printf("ERROR: Invalid Branch Address at Runtime\n");
				return ErrorRuntime;
			}
			clock += 2;
			TimeLeft -= 2;
			break;
		case 7:                 //branch on minus
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			if (op1val < 0) {
				if (0 <= pc <= MAX_USER_MEMORY) {
					pc = mem[pc];
				}else  {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}else  {
				pc++;	//Skip Branch and advance
			}
			clock += 4;
			TimeLeft -= 4;
			break;
		case 8:                 //branch on plus
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			if (op1val > 0) {
				if (0 <= pc <= MAX_USER_MEMORY) {
					pc = mem[pc];
				}else  {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}else  {
				pc++;	//Skip Branch and advance
			}
			clock += 4;
			TimeLeft -= 4;
			break;
		case 9:                 //branch on zero
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			if (op1val == 0) {
				if (0 <= pc <= MAX_USER_MEMORY) {
					pc = mem[pc];
				}else  {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}else  {
				pc++;	//Skip Branch and advance
			}
			clock += 4;
			TimeLeft -= 4;
			break;
		case 10:                //push
			status = FetchOperand(op1mode, op1gpr, &op1addr, &op1val);
			if (status != OK) {                //Return ERROR value to Main
				return status;
			}

			if ((MAX_USER_MEMORY < sp < MAX_HEAP_MEMORY) != 0) {
				printf("ERROR: Stack Address Overflow\n");
				return ErrorStackOverflow;
			}

			// Push to Stack
			sp++;
			mem[sp] = op1val;
			clock += 2;
			TimeLeft -= 2;
			break;
		case 11:                //pop
			if ((MAX_USER_MEMORY < sp < MAX_HEAP_MEMORY) != 0) {
				printf("ERROR: Stack Address Underflow\n");
				return ErrorStackUnderflow;
			}

			// Pop Stack
			op1val = mem[sp];
			sp--;
			clock += 2;
			TimeLeft -= 2;
			break;
		case 12:                //system call
			if (MAX_USER_MEMORY < sp < MAX_HEAP_MEMORY) {
				printf("ERROR: Systemcall to Invalid Address\n");
				return ErrorRuntime;
			}
			long SystemCallID = mem[pc++];
			status = SystemCall(SystemCallID);
			clock += 12;
			TimeLeft -= 12;
			break;
		default:                //Invalid Opcode
			printf("ERROR: Invalid opcode on line %d\n", mar);         // Error
			return ErrorInvalidOpcode;
		}
	}
}


/*******************************************************************************
 * Function: SystemCall
 *
 * Description: Processes SystemCall Instructions
 *
 * Input Parameters
 *      OpValue
 *
 * Output Parameters
 *
 *
 * Function Return Value
 *      OK				-Successful Fetch
 *      ErrorInvalidAddress		-Oprand address not valid
 *      ErrorInvalidMode		-Mode not correct
 *      ErrorInvalidPCValue		-Direct Mode PC out of bounds
 ******************************************************************************/

//TODO: Implement Function
long SystemCall(long SystemCallID)
{
	psr = MACHINE_MODE_OS;		// Set system mode to OS mode
	printf("MACHINE STATUS SET >>> OS");

	long status = OK;

	switch (SystemCallID) {
	case 1:                 //process_create
		status = CreateProcess(filename, priority);
		break;
	case 2:                 //process_delete

		break;
	case 3:                 //process_inquiry

		break;
	case 4:                 //mem_alloc
	    //Dynamic memory allocation: Allocate user free memory system call
        MemAllocSystemCall();
		break;
	case 5:                 //mem_free
	    // Free dynamically allocated user memory system call
        MemFreeSystemCall();
		break;
	case 6:                 //msg_send

		break;
	case 7:                 //msg_recieve

		break;
	case 8:                 //io_getc

		break;
	case 9:                 //io_putc

		break;
	case 10:                //time_get

		break;
	case 11:                //time_set

		break;
	default:

		break;
	}
	psr = MACHINE_MODE_USER;		// Restore to User Mode
	return status;
}

/*******************************************************************************
 * Function: FetchOperand
 *
 * Description: Depending on the Addressing Mode, function checks if address
 * refered to by the instruction is in valid memory space and performs actions
 * on the instruction. The result is returned.
 * The result is intended to define the status of the machine (psr).
 *
 * Input Parameters
 *      OpMode				Operand Mode Value
 *      OpReg				Operand GPR Value
 *
 * Output Parameters
 *      OpAddress			Address of Operand
 *      OpValue				Value of Operand when GPR and mode are valid
 *
 * Function Return Value
 *      OK				-Successful Fetch
 *      ErrorInvalidAddress		-Oprand address not valid
 *      ErrorInvalidMode		-Mode not correct
 *      ErrorInvalidPCValue		-Direct Mode PC out of bounds
 ******************************************************************************/

long FetchOperand(
	long OpMode,
	long OpReg,
	long *OpAddress,
	long *OpValue)
{
	//Fetch value based on value based on the operand mode
	switch (OpMode) {
	case 1:         //Register Mode
		*OpAddress = -1;         //Set to Invalid Address
		*OpValue = gpr[OpReg];
		break;
	case 2:         //Register deferred mode
		*OpAddress = gpr[OpReg];         //Grab OPAddress from Register

		if (0 <= OpAddress <= MAX_USER_MEMORY) {
			*OpValue = mem[*OpAddress];         //Grab OpValue in Mem
		}else  {
			printf("ERROR: Invalid Fetch Operand Address\n");
			return ErrorInvalidAddress;
		}

		break;
	case 3:         //Autoincrement mode -> ADDR in GPR, OPVAL in MEM
		*OpAddress = gpr[OpReg];                //Op Address is in Register

		if (0 <= OpAddress <= MAX_USER_MEMORY) {
			*OpValue = mem[*OpAddress];         //Grab OpValue in Mem
		}else  {
			printf("ERROR: Invalid Fetch Operand Address\n");
			return ErrorInvalidAddress;
		}
		gpr[OpReg]++;

		break;
	case 4:         //Autodecrement mode
		--gpr[OpReg];
		*OpAddress = gpr[OpReg];
		if (0 <= OpAddress <= MAX_USER_MEMORY) {
			*OpValue = mem[*OpAddress];         //Grab OpValue in Mem
		}else  {
			printf("ERROR: Invalid Fetch Operand Address\n");
			return ErrorInvalidAddress;
		}

		break;
	case 5:         //Direct mode -> OP Address is mem[pc]

		if ((0 <= pc <= MAX_USER_MEMORY) == 0) {
			printf("ERROR: Invalid PC Address at Runtime\n");
			return ErrorRuntime;
		}
		*OpAddress = mem[pc++];
		if (0 <= *OpAddress <= MAX_USER_MEMORY) {
			*OpValue = mem[*OpAddress];
		}else  {
			printf("ERROR: Invalid Address\n");
			return ErrorInvalidAddress;
		}

		break;
	case 6:         //Immediate mode -> Opvalue in Instruction
		if ((0 <= mem[pc] <= MAX_USER_MEMORY) == 0) {
			printf("ERROR: Invalid PC Address at Runtime\n");
			return ErrorRuntime;
		}
		*OpAddress = -1;                        //Set to Invalid Address
		*OpValue = mem[pc++];

		break;
	default:        //Invalid mode
		printf("ERROR: Invalid mode at line %d\n", mbr);
		return ErrorInvalidMode;
	}
	return OK;
}

/*******************************************************************************
 * Function: DumpMemory
 *
 * Description: Displays the current state of all GeneralPurposeRegisters(GPR),
 * the StackPointer (SP), ProgramCounter(PC), ProcessorStatusRegister(PSR),
 * system Clock, and a dump of the system memory up-to a given memory address.
 *
 * Input Parameters
 *      String			String header displayed above Status Table
 *      StartAddress			Memory location from which to begin dump
 *      Size				Offset from StartAddress
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      None
 ******************************************************************************/

void DumpMemory(
	char* String,
	long StartAddress,
	long size)
{
	/* Print String Header */
	printf("%s\n", String);

	/* Returns Error */
	if (StartAddress + size > SYSTEM_MEMORY_SIZE || StartAddress < 0) {
		printf("ERROR: Invalid Dump-Memory Range\n");
		return;
	}

	/* Print Register Table Header + Status */
	for (int register_number = 0; register_number < GPR_NUMBER; register_number++)
		printf("\tG%d", register_number);
	printf("\tSP\tPC\n");

	for (int register_number = 0; register_number < GPR_NUMBER; register_number++)
		printf("\t%d", gpr[register_number]);
	printf("\t%d\t%d\n", sp, pc);

	/* Memory Table Header */
	printf("Address");
	for (int num = 0; num < 10; num++)
		printf("\t+%d", num);
	printf("\n");

	/* Operational Variables */
	long addr = (StartAddress / 10) * 10; //Rounds down to Nearest 10 (Integer Math)
	long endAddress = StartAddress + size;

	/* Dump Memory */
	while (addr < endAddress) {
		printf("%d\t", addr);
		for (int i = 0; i < 10; i++)
			printf("%d\t", mem[addr + i]);
		printf("\n");
		addr += 10;
	}
	printf("System-Clock >> %d\n", clock);
	printf("Processor Status Register (PSR) >> %d\n", psr);
}

/*******************************************************************************
 * Function: CreateProcess
 *
 * Description: //TODO
 *
 * Input Parameters
 *      String (pointer)		Name of the file associated with the process
 * 	Long				An interger value defining priority
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      None
 ******************************************************************************/
long CreateProcess (char* filename, long priority)
{
	// Allocate space for Process Control Block
	long *PCBptr = &EndOfList; //TODO: Wrong behavior,

	// Initialize PCB: Set nextPCBlink to end of list, default priority, Ready state, and PID
	InitializePCB(PCBptr); //TODO: FIX names once function implemented

	// Load the program
	if (AbsoluteLoader(filename) == OK)
		pcb->pc = pc; 		// Store PC value in the PCB of the process
	else
		return ErrorFileOpen;

					// Allocate stack space from user free list
	pcb->ptr = StackSize; 		// Set ptr = Allocate User Memory of size StackSize;
	if (ptr < 0)			// Check for error
	{  				// User memory allocation failed
		FreeOSMemory(PCBptr, SIZE);
		return(ErrorInvalidMemorySize);  		// return error code
	}

	// Store stack information in the PCB – SP, ptr, and size
	pcb->sp           = ptr + SIZE;		// empty stack is high address, full is low address
	pcb->StartAddress = ptr;
	pcb->StackSize    = SIZE;
	pcb->priority     = DEFAULT_PRIORITY;	// Set priority

	DumpMemory("PCB Created", ptr, SIZE);				// Dump PCB stack

	//TODO: print PCB

	// Insert PCB into Ready Queue according to the scheduling algorithm
	InsertIntoRQ(PCBptr);

	return(OK);
}

/*******************************************************************************
 * Function: AllocateOSMemory
 *
 * Description: Displays the current state of all GeneralPurposeRegisters(GPR),
 * the StackPointer (SP), ProgramCounter(PC), ProcessorStatusRegister(PSR),
 * system Clock, and a dump of the system memory up-to a given memory address.
 *
 * Input Parameters
 *      String			String header displayed above Status Table
 *      StartAddress			Memory location from which to begin dump
 *      Size				Offset from StartAddress
 *
 * Output Parameters
 *      None
 *
 * Function Return Value
 *      None
 ******************************************************************************/

long AllocateOSMemory (long RequestedSize)  // return value contains address or error
{
	// Allocate memory from OS free space, which is organized as link
	if(OSFreeList == EndOfList)
	{
		//TODO display no free OS memory error;
		return(ErrorNoFreeMemory);   // ErrorNoFreeMemory is constant set to < 0
	}
	if(RequestedSize < 0)
	{
		//TODO display invalid size error;
		return(ErrorInvalidMemorySize);  // ErrorInvalidMemorySize is constant < 0
	}
	if(RequestedSize == 1)
		RequestedSize = 2;  // Minimum allocated memory is 2 locations

	long CurrentPtr = OSFreeList;
	long PreviousPtr = EndOfList;
	while (CurrentPtr != EndOfList)
	{
		// Check each block in the link list until block with requested memory size is found
		if(mem[CurrentPtr + 1] == RequestedSize)
		{  // Found block with requested size.  Adjust pointers
			if(CurrentPtr == OSFreeList)  // first block
			{
				OSFreeList = mem[CurrentPtr];  // first entry is pointer to next block
				mem[CurrentPtr] = EndOfList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  // not first black
			{
				mem[PreviousPtr] = mem[CurrentPtr];  // point to next block
				mem[CurrentPtr] = EndOfList;  // reset next pointer in the allocated block
				return(CurrentPtr);    // return memory address
			}
		}
		else if(mem[CurrentPtr + 1] > RequestedSize)
		{  // Found block with size greater than requested size
			if(CurrentPtr == OSFreeList)  // first block
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];  // move next block ptr
				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr +1] - RequestedSize;
				OSFreeList = CurrentPtr + RequestedSize;  // address of reduced block
				mem[CurrentPtr] = EndOfList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  // not first black
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];  // move next block ptr
				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr +1] - RequestedSize;
				mem[PreviousPtr] = CurrentPtr + RequestedSize;  // address of reduced block
				mem[CurrentPtr] = EndOfList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
		}
		else  // small block
		{  // look at next block
			PreviousPtr = CurrentPtr;
			CurrentPtr = mem[CurrentPtr];
		}
	} // end of while CurrentPtr loop

	//TODO: display no free OS memory error;
	return(ErrorNoFreeMemory);   // ErrorNoFreeMemory is constant set to < 0
}


/*******************************************************************************
 * Function: FreeOSMemory
 *
 * Description: //TODO
 *
 * Input Parameters
 * //TODO
 *
 * Output Parameters
 * //TODO
 *
 * Function Return Value
 * //TODO
 ******************************************************************************/

long FreeOSMemory(long ptr, long size)
{
	if(ptr < 7000 || ptr > MAX_OS_MEMORY)
	{
		printf("ERROR: Invalid Adress");
		return(ErrorInvalidAddress);
	}

	if(size == 1)
	{
		size = 2; //minimum allocated size
	}

	else if(size < 1 ||(ptr+size) >= MAX_OS_MEMORY)
	{
		//invalid size
		printf("ERROR: Invalid size or Invalid Address");
		return(ErrorInvalidAddress);
	}

	mem[ptr] = OSFreeList;
	mem[ptr + 1] = size;
	OSFreeList = ptr;
}

/*******************************************************************************
 * Function: AllocateUserMemory
 *
 * Description: //TODO
 *
 * Input Parameters
 * //TODO
 *
 * Output Parameters
 * //TODO
 *
 * Function Return Value
 * //TODO
 ******************************************************************************/

long AllocateUserMemory(long RequestedSize) //return value contains address or ERROR
{
	// Allocate memory from OS free space, which is organized as link
	if(UserFreeList == EndOfList)
	{
		printf("ERROR: No free User memory\n");
		return(ErrorNoFreeMemory);
	}

	if(RequestedSize < 0)
	{
		printf("ERROR: Invalid Memory Size\n");
		return(ErrorInvalidMemorySize);
	}

	if(RequestedSize == 1)
	{
		RequestedSize = 2; //minimum allocated memory is 2 locations
	}

	long CurrentPtr = UserFreeList;
	long PreviousPtr = EndOfList;
	while(CurrentPtr != EndOfList)
	{
		if(mem[CurrentPtr + 1] == RequestedSize)
		{//found block with requested Size
			if(CurrentPtr == UserFreeList) // first block
			{
				UserFreeList = mem[CurrentPtr]; //first entry is pointer to next block
				mem[CurrentPtr] = EndOfList;  //reset next pointer in the allocated blcok
				return(CurrentPtr); //return memory address
			}
			else //not first block
			{
				mem[PreviousPtr] = mem[CurrentPtr]; //point to next block
				mem[CurrentPtr] = EndOfList; //reset next pointer in the allocated block
				return(CurrentPtr); //return memory address
			}
		}

		else if(mem[CurrentPtr+1] > RequestedSize)
		{
			//found block with size greater than RequestedSize
			if(CurrentPtr == UserFreeList) //first block
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];

				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr +1] - RequestedSize;

				UserFreeList = CurrentPtr + RequestedSize; //address of reduced block
				mem[CurrentPtr] = EndOfList; //reset next pointer in the allocated block
				return(CurrentPtr); //return memory address
			}
			else // not first block
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr]; //move the next block pointer

				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr + 1] - RequestedSize;

				mem[PreviousPtr] = CurrentPtr + RequestedSize; //address of reduced block
				mem[CurrentPtr] = EndOfList; //Reset next pointer in the allocated block
				return(CurrentPtr); //return memory address
			}
		}
		else //small block
		{
			//look at next block
			PreviousPtr = CurrentPtr;
			CurrentPtr = mem[CurrentPtr];

		}
	} //end of while CurrentPtr loop

	printf("ERROR: No Free User memory\n");
	return(ErrorNoFreeMemory);
}

/*******************************************************************************
 * Function: FreeUserMemory
 *
 * Description: //TODO
 *
 * Input Parameters
 * //TODO
 *
 * Output Parameters
 * //TODO
 *
 * Function Return Value
 * //TODO
 ******************************************************************************/

long FreeUserMemory(long ptr, long size)
{
	if(ptr < 0 || ptr > MAX_USER_MEMORY)
	{
		printf("ERROR: Invalid Adress");
		return(ErrorInvalidAddress);
	}

	if(size == 1)
	{
		size = 2; //minimum allocated size
	}

	else if(size < 1 ||(ptr+size) >= MAX_USER_MEMORY)
	{
		//invalid size
		printf("ERROR: Invalid size or Invalid Address");
		return(ErrorInvalidAddress);
	}

	mem[ptr] = UserFreeList;
	mem[ptr + 1] = size; //set the free block size in the given free block
	UserFreeList = ptr; // user free list points to given free block
}

/*******************************************************************************
 * Function: MemAllocSystemCall
 *
 * Description: //TODO
 *
 * Input Parameters
 * //TODO
 *
 * Output Parameters
 * //TODO
 *
 * Function Return Value
 * //TODO
 ******************************************************************************/

long MemAllocSystemCall()
{
	// Allocate memory from user free list
	// Return status from the function is either the address of allocated memory or an error code

	long Size = gpr[2];

	//check if size is out of range
	if(Size < 1 || Size > MAX_USER_MEMORY)
	{
		printf("Error: InvalidAddress");
		return(ErrorInvalidAddress);
	}

	if(Size == 1)
		Size = 2;

	gpr[1] = AllocateUserMemory(Size);

	if(gpr[1] < 0)
	{
		gpr[0] = gpr[1]; //set GPR0 to have the return status
	}
	else
	{
		gpr[0] = OK;
	}

	printf("MemAllocSystemCall - GPR0: %d\tGPR1: %d\tGPR2: %d", gpr[0], gpr[1], gpr[2]);

	return gpr[0];
}

/*******************************************************************************
 * Function: MemFreeSystemCall
 *
 * Description: //TODO
 *
 * Input Parameters
 * //TODO
 *
 * Output Parameters
 * //TODO
 *
 * Function Return Value
 * //TODO
 ******************************************************************************/

long MemFreeSystemCall()
{
	// Return dynamically allocated memory to the user free list
	// GPR1 has memory address and GPR2 has memory size to be released
	// Return status in GPR0

	long Size = gpr[2];

	//check if size is out of range
	if(Size < 1 || Size > MAX_USER_MEMORY)
	{
		printf("Error: InvalidAddress");
		return(ErrorInvalidAddress);
	}

	if(Size == 1)
		Size = 2;

	gpr[0] = FreeUserMemory(gpr[1], Size);

	printf("Mem_Free System Call - GPR0: %d\tGPR1: %d\tGPR2: %d", gpr[0], gpr[1], gpr[2]);

	return gpr[0];
}

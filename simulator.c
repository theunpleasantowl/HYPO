#include <stdio.h>

/*** VIRTUAL SYSTEM PARAMETERS ***/
#define SYSTEM_MEMORY_SIZE      10000
#define GPR_NUMBER              8
#define DEFAULT_PRIORITY	128
#define TIMESLICE		200
#define ReadyState 1
#define EndOfList -1


/*** VALUE CONSTANTS ***/
#define SCRIPT_INDICATOR_END	-1

// Simulator Execution Status
#define SIMULATOR_STATUS_HALTED	0
#define SIMULATOR_STATUS_OK	1

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
long OSFreeList, UserFreeList, RQ, WQ;
//long EndOfList = -1; //indicates end of OSFreeList or UserFreeList

long ProcessID = 1;

//PCB components
const int Ready = 1;
const int Running = 2;
const int Waiting = 3;
const int PCBsize = 22;
const int NextPtr = 0;
const int PCB_pid = 1;
const int PCB_State = 2;
const int PCB_Reason = 3;
const int PCB_Priority = 4;
const int PCB_StackSize = 5;
const int PCB_StackStartAddr = 6;
const int PCB_GPR0 = 11;
const int PCB_GPR1 = 12;
const int PCB_GPR2 = 13;
const int PCB_GPR3 = 14;
const int PCB_GPR4 = 15;
const int PCB_GPR5 = 16;
const int PCB_GPR6 = 17;
const int PCB_GPR7 = 18;
const int PCB_SP = 19;
const int PCB_PC = 20;
const int PCB_PSR = 21;

/*** FUNCTION PROTOTYPES ***/
void InitializeSystem();
int main();
int AbsoluteLoader(char* filename);
long CPU();
long SystemCall(long SystemCallID);
long FetchOperand(long OpMode, long OpReg, long *OpAddress, long *OpValue);
void DumpMemory(char* String, long StartAddress, long size);
long CreateProcess(char *filename, long priority);
void TerminateProcess(long PCBptr);
long AllocateOSMemory(long RequestedSize);
long FreeOSMemory(long ptr, long size);
long AllocateUserMemory(long size);
long FreeUserMemory(long ptr, long size);
long MemAllocSystemCall();
long MemFreeSystemCall();
void InitializePCB();
void PrintPCB(long PCBptr);
long PrintQueue(long Qptr);
long InsertIntoRQ(long PCBptr);
long InsertIntoWQ(long PCBptr);
long SelectProcessFromRQ();
void SaveContext(long PCBptr);
void Dispatcher(long PCBptr);
void TerminateProcess(long PCBptr);
void CheckAndProcessInterrupt();
void ISRrunProgramInterrupt();
void ISRinputCompletionInterrupt();
void ISRoutputCompletionInterrupt();
long io_getc(char R1, int *R0);
long io_putc(char R1, int *R0);

/*******************************************************************************
 * Function: PrintPCB
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

void PrintPCB(long PCBptr)
{
	/*
	Print the values of the following fields from PCB with a text before the value like below:
		PCB address = 6000, Next PCB Ptr = 5000, PID = 2, State = 2, PC = 200, SP = 4000,
		Priority = 127, Stack info: start address = 3990, size = 10
		GPRs = print 8 values of GPR 0 to GPR 7
	*/
}  // end of PrintPCB() function

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
		}
		else if (0 <= addr <= MAX_USER_MEMORY)
			mem[addr] = word;
		else {
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
		}
		else {
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
			}
			else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}
			else {        //Store result in op1address
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
			}
			else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}
			else {        //Store result in op1address
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
			}
			else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}
			else {        //Store result in op1address
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
			}
			else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}
			else {        //Store result in op1address
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
			}
			else if (op1mode == 6) {
				printf("ERROR: Line %d Destination cannot be immediate\n", pc);
				return ErrorImmediateMode;
			}
			else {        //Store result in op1address
				mem[op1addr] = op2val;
			}
			clock += 2;
			TimeLeft -= 2;
			break;
		case 6:                 //branch
			if (0 <= pc <= MAX_USER_MEMORY)
				pc = mem[pc];
			else {
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
				}
				else {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}
			else {
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
				}
				else {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}
			else {
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
				}
				else {
					printf("ERROR: Invalid Branch Address at Runtime\n");
					return ErrorRuntime;
				}
			}
			else {
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
		//status = CreateProcess(filename, priority);
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
		}
		else {
			printf("ERROR: Invalid Fetch Operand Address\n");
			return ErrorInvalidAddress;
		}

		break;
	case 3:         //Autoincrement mode -> ADDR in GPR, OPVAL in MEM
		*OpAddress = gpr[OpReg];                //Op Address is in Register

		if (0 <= OpAddress <= MAX_USER_MEMORY) {
			*OpValue = mem[*OpAddress];         //Grab OpValue in Mem
		}
		else {
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
		}
		else {
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
		}
		else {
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
long CreateProcess(char* filename, long priority)
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

	// Store stack information in the PCB . SP, ptr, and size
	pcb->sp = ptr + SIZE;		// empty stack is high address, full is low address
	pcb->StartAddress = ptr;
	pcb->StackSize = SIZE;
	pcb->priority = DEFAULT_PRIORITY;	// Set priority

	DumpMemory("PCB Created", ptr, SIZE);				// Dump PCB stack

	//TODO: print PCB

	// Insert PCB into Ready Queue according to the scheduling algorithm
	InsertIntoRQ(PCBptr);

	return(OK);
}
/*******************************************************************************
 * Function: TerminateProcess
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

void TerminateProcess(long PCBptr)
{
	// Return stack memory using stack start address and stack size in the given PCB

	// Return PCB memory using the PCBptr

	return;

} //End of TerminateProcess function

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

long AllocateOSMemory(long RequestedSize)  // return value contains address or error
{
	// Allocate memory from OS free space, which is organized as link
	if (OSFreeList == EndOfList)
	{
		//TODO display no free OS memory error;
		return(ErrorNoFreeMemory);   // ErrorNoFreeMemory is constant set to < 0
	}
	if (RequestedSize < 0)
	{
		//TODO display invalid size error;
		return(ErrorInvalidMemorySize);  // ErrorInvalidMemorySize is constant < 0
	}
	if (RequestedSize == 1)
		RequestedSize = 2;  // Minimum allocated memory is 2 locations

	long CurrentPtr = OSFreeList;
	long PreviousPtr = EndOfList;
	while (CurrentPtr != EndOfList)
	{
		// Check each block in the link list until block with requested memory size is found
		if (mem[CurrentPtr + 1] == RequestedSize)
		{  // Found block with requested size.  Adjust pointers
			if (CurrentPtr == OSFreeList)  // first block
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
		else if (mem[CurrentPtr + 1] > RequestedSize)
		{  // Found block with size greater than requested size
			if (CurrentPtr == OSFreeList)  // first block
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];  // move next block ptr
				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr + 1] - RequestedSize;
				OSFreeList = CurrentPtr + RequestedSize;  // address of reduced block
				mem[CurrentPtr] = EndOfList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  // not first black
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];  // move next block ptr
				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr + 1] - RequestedSize;
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
	if (ptr < 7000 || ptr > MAX_OS_MEMORY)
	{
		printf("ERROR: Invalid Adress");
		return(ErrorInvalidAddress);
	}

	if (size == 1)
	{
		size = 2; //minimum allocated size
	}

	else if (size < 1 || (ptr + size) >= MAX_OS_MEMORY)
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
	if (UserFreeList == EndOfList)
	{
		printf("ERROR: No free User memory\n");
		return(ErrorNoFreeMemory);
	}

	if (RequestedSize < 0)
	{
		printf("ERROR: Invalid Memory Size\n");
		return(ErrorInvalidMemorySize);
	}

	if (RequestedSize == 1)
	{
		RequestedSize = 2; //minimum allocated memory is 2 locations
	}

	long CurrentPtr = UserFreeList;
	long PreviousPtr = EndOfList;
	while (CurrentPtr != EndOfList)
	{
		if (mem[CurrentPtr + 1] == RequestedSize)
		{//found block with requested Size
			if (CurrentPtr == UserFreeList) // first block
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

		else if (mem[CurrentPtr + 1] > RequestedSize)
		{
			//found block with size greater than RequestedSize
			if (CurrentPtr == UserFreeList) //first block
			{
				mem[CurrentPtr + RequestedSize] = mem[CurrentPtr];

				mem[CurrentPtr + RequestedSize + 1] = mem[CurrentPtr + 1] - RequestedSize;

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
	if (ptr < 0 || ptr > MAX_USER_MEMORY)
	{
		printf("ERROR: Invalid Adress");
		return(ErrorInvalidAddress);
	}

	if (size == 1)
	{
		size = 2; //minimum allocated size
	}

	else if (size < 1 || (ptr + size) >= MAX_USER_MEMORY)
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
	if (Size < 1 || Size > MAX_USER_MEMORY)
	{
		printf("Error: InvalidAddress");
		return(ErrorInvalidAddress);
	}

	if (Size == 1)
		Size = 2;

	gpr[1] = AllocateUserMemory(Size);

	if (gpr[1] < 0)
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
	if (Size < 1 || Size > MAX_USER_MEMORY)
	{
		printf("Error: InvalidAddress");
		return(ErrorInvalidAddress);
	}

	if (Size == 1)
		Size = 2;

	gpr[0] = FreeUserMemory(gpr[1], Size);

	printf("Mem_Free System Call - GPR0: %d\tGPR1: %d\tGPR2: %d", gpr[0], gpr[1], gpr[2]);

	return gpr[0];
}

/*******************************************************************************
 * Function: InitializePCB
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

void InitializePCB(long PCBptr)
{
	//Set entire PCB area to 0 using PCBptr; 
	// Array initialization
	for (int i = 0; i < MAX_USER_MEMORY; i++)
	{
		mem[PCBptr + i] = 0;
	}
	// Allocate PID and set it in the PCB. PID zero is invalidcvoid
	mem[PCBptr + PCB_pid] = ProcessID++;  // ProcessID is global variable initialized to 1

	//Set state field in the PCB = ReadyState;
	mem[PCBptr + PCB_State] = ReadyState;

	//Set priority field in the PCB = Default Priority;  
	mem[PCBptr + PCB_Priority] = DEFAULT_PRIORITY;

	//Set next PCB pointer field in the PCB = EndOfList
	mem[PCBptr + NextPtr] = EndOfList;

	return;
}

/*******************************************************************************
 * Function: PrintQueue
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

long PrintQueue(long Qptr)
{
	long currentPCBPtr = Qptr;

	if (currentPCBPtr == EndOfList)
	{
		printf("End of List reached");
		return(OK);
	}

	while (currentPCBPtr != EndOfList)
	{
		//Print PCB passing currentPCBPtr
		//currentPCBPtr = nextPCBlink;
	}

	return(OK);
}

/*******************************************************************************
 * Function: SelectProcessFromRQ
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

long SelectProcessFromRQ()
{
	long PCBptr = RQ;

	if (RQ != EndOfList)
	{
		// Remove first PCB RQ
		// Set RQ = next PCB pointed by RQ
	}

	// Set next point to EOL in the PCB
	// Set Next PCBfield in the given PCB to End of List

	return(PCBptr);
} //end of SelectProcessFromRQ


/*******************************************************************************
 * Function: SaveContext
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

void SaveContext(long PCBptr)
{
	//Assume PCBptr is a valid pointer

	//Copy all CPU GPRs into PCB using PCBptr with or without using loop

	//Set SP field in the PCB=SP; //Save SP
	//Set PC field in the PCB=PC; //Save PC
}

/*******************************************************************************
 * Function: Dispatcher
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

void Dispatcher(long PCBptr)
{
	//PCBptr is assumed to be correct

	//copy CPU GPR register values from given PCB into the CPU registers 
	//This is opposite of save CPU context 

	//Restore SP and PC from given PCB
	//UserMode is 2, OSMode is 1
	psr = MACHINE_MODE_USER;

	return;
}

/*******************************************************************************
 * Function: InsertIntoRQ
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

long InsertIntoRQ(long PCBptr)
{
	// Insert PCB according to Priority Round Robin algorithm
	// Use priority in the PCB to find the correct place to insert
	long PreviousPtr = EndOfList;
	long CurrentPtr = EndOfList;

	//check for invalid PCB memory address
	if ((PCBptr < 0) || (PCBptr > MAX_USER_MEMORY))
	{
		printf("ERROR: Invalid Memory Address ");
		return(ErrorInvalidAddress);
	}

	//TODO: replace StateIndex, ready, NextPointerIndex
	mem[PCBptr + PCB_State] = Ready;   //set state to ready
	mem[PCBptr + NextPtr] = EndOfList; //set next pointer to end of list

	if (RQ == EndOfList) //RQ is empty
	{
		RQ = PCBptr;
		return(OK);
	}

	//walk thru RQ and find place to insert
	// PCB will be inserted at the end of its priority

	while (CurrentPtr != EndOfList)
	{
		if (mem[PCBptr + PCB_Priority] > mem[CurrentPtr + PCB_Priority])
		{
			if (PreviousPtr == EndOfList)
			{
				// Enter PCB in the front of the list as first entry
				mem[PCBptr + NextPtr] = RQ;
				RQ = PCBptr;
				return(OK);
			}
			//enter PCB in the middle of the list
			mem[PCBptr + NextPtr] = mem[PreviousPtr + NextPtr];
			mem[PreviousPtr + NextPtr] = PCBptr;
			return(OK);
		}
		else //PCB to inserted has lower or equal priority to the Current PCB in RQ
		{
			//go to next PCB in RQ
			PreviousPtr = CurrentPtr;
			CurrentPtr = mem[CurrentPtr + NextPtr];
		}
	} // end of while loop

	//insert PCB at the end of RQ
	mem[PreviousPtr + NextPtr] = PCBptr;
	return(OK);
}

/*** FUNCTIONS ***/
/*******************************************************************************
 * Function: InsertIntoWQ
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

long InsertIntoWQ(long PCBptr)
{
	//insert given PCB at the front of InsertIntoWQ

	//check for invalid PCB memory Address
	if ((PCBptr < 0) || (PCBptr > MAX_OS_MEMORY))
	{
		printf("ERROR: Invalid PCB address");
		return(ErrorInvalidAddress); //error code < 0
	}

	mem[PCBptr + PCB_State] = Waiting; //What
	mem[PCBptr + NextPtr] = WQ;

	WQ = PCBptr;

	return(OK);
} //end of InsertIntoWQ() function

/*******************************************************************************
 * Function: CheckAndProcessInterrupt
 *
 * Description: Read interrupt ID number. Based on the interrupt ID,
 * service the interrupt.
 *
 * Input Parameters: N/A
 *
 * Output Parameters: N/A
 *
 * Function Return Value: N/A
 *
 ******************************************************************************/

void CheckAndProcessInterrupt()
{

	int InterruptID;
	// Prompt and read interrupt ID
	printf("Possible interrupt IDs: \n0 - no interrupt
		\n1 - run program
		\n2 - shutdown system
		\n3 - input operation completion(io_getc)
		\n4 - output operation completion(io_putc)");

		printf("Input interrupt ID: ");
	scanf("%d", InterruptID);
	printf("Interrupt read: %d", InterruptID);

	// Process interrupt
	switch (InterruptID);
	{
	case 0: // no interrupt
		break;

	case 1: // run program
		ISRrunProgramInterrupt();
		break;

	case 2: // shutdown system
		ISRshutdownSystem();
		break;

	case 3: // input operation completion (io_getc)
		ISRinputCompletionInterrupt();
		break;

	case 4: // output operation completion (io_putc)
		ISRoutputCompletionInterrupt();
		break;

	default: // invalid interrupt ID
		printf("Invalid interrupt ID");
		break;
	}

	return;
}


/*******************************************************************************
* Function: ISRrunProgramInterrupt
*
* Description: Read filename and create process.
*
* Input Parameters: N/A
*
* Output Parameters
* 		1. N/A
*
* Function Return Value
* //
******************************************************************************/

void ISRrunProgramInterrupt()
{
	char filename[30];

	// Prompt and read filename
	printf("Input filename: ");
	fgets(filename, 30, stdin);

	// Call Create Process passing filename and Default Priority as arguments
	CreateProcess(filename, DEFAULT_PRIORITY);

	return;
}

/*******************************************************************************
* Function: Input Completion Interrupt
*
* Description: Read PID of the process completing the io_getc operation and
* 				read one character from the keyboard (input device). Store the
* 				character in the GPR in the PCB of the process.
*
* Input Parameters: N/A
*
* Output Parameters: N/A
*
* Function Return Value: N/A
******************************************************************************/

void ISRinputCompletionInterrupt()
{

	int ProcessID;
	long currentPCBptr = WQ;
	char PCBReplacementChar;

	// Prompt and read PID of the process completing input completion
	printf("Input PID of the process completing input completion: ");
	scanf("%d", ProcessID);

	// Search WQ to find the PCB having the given PID
	while (currentPCBptr != EndOfList) {
		if (mem[currentPCBptr] == ProcessID) {
			// Remove PCB from the WQ

			// Read one character from standard input device keyboard
				// (system call?)


			// Store the character in the GPR in the PCB, type cast char->long

			// Set the process state to Ready in the PCB

			// Insert PCB into RQ

			break;
		}
	}

	// If no match is found in WQ, then search RQ
	currentPCBptr = RQ;
	while (currentPCBptr != EndOfList) {
		if (mem[currentPCBptr] == ProcessID) {
			// Read one character from standard input device keyboard

			// Store the character in the GPR in the PCB	      

			break;
		}

	}

	// If no matching PCB is found in WQ, and RQ, print invalid PID as an error message.
	printf("Invalid Process ID");
}

/*******************************************************************************
* Function: Output Completion Interrupt
*
* Description: Read PID of the process completing the io_putc operation and
* 				display one character on the monitor (output device) from the GPR
* 				in the PCB of the process
*
* Input Parameters: N/A
*
* Output Parameters: N/A
*
* Function Return Value: N/A
******************************************************************************/

void ISRoutputCompletionInterrupt()
{

	int ProcessID;
	long currentPCBptr = WQ;

	// Prompt and read PID of the process completing input completion
	printf("Input PID of the process completing input completion: ");
	scanf("%d", ProcessID);

	// Search WQ to find the PCB having the given PID
	while (currentPCBptr != EndOfList) {
		if (mem[currentPCBptr] == ProcessID) {
			// Remove PCB from the WQ

			// Read one character from standard input device keyboard
				// (system call?)


			// Store the character in the GPR in the PCB, type cast char->long

			// Set the process state to Ready in the PCB

			// Insert PCB into RQ

			break;
		}
	}

	// If no match is found in WQ, then search RQ
	currentPCBptr = RQ;
	while (currentPCBptr != EndOfList) {
		if (mem[currentPCBptr] == ProcessID) {
			// Read one character from standard input device keyboard

			// Store the character in the GPR in the PCB	      

			break;
		}

	}

	// If no matching PCB is found in WQ, and RQ, print invalid PID as an error message.
	printf("Invalid Process ID");
}

/*******************************************************************************
* Function: IOGetC
*
* Description: Obtain one character from the user. Forces rescheduling
*
* Input Parameters: R1 = the character read
*
* Output Parameters
* 		1. R0 = return code, always OK.
*
* Function Return Value
* //
******************************************************************************/

long IOGetCSystemCall(char R1, int *R0)
{
	R1 = getchar();
	R0 = OK;
	return R1;
}

/*******************************************************************************
* Function: IOPutC
*
* Description: Specifies a character to be printed on the user terminal.
* 				Forces rescheduling
* Input Parameters: R1 = character to be displayed
*
* Output Parameters
* 		1. R0 = return code, always OK
*
* Function Return Value
* //
******************************************************************************/

long IOPutCSystemCall(char R1, int *R0)
{
	//printf("%d\n", R1);
	putchar(R1);
	R0 = OK;
	return R0;
}
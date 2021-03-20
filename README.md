# HYPO

This report describes the architecture of a hypothetical decimal machine called HYPO. This machine will be simulated by software. The simulated machine will be used to run the MTOPS real-time multitasking operating system. The MTOPS operating system is intended for microcomputers. MTOPS is designed and implemented as a project for the ‘Operating Systems’ course.

The HYPO is a decimal machine whereas all the existing computing systems are binary machine. I have designed a decimal machine for simplicity of programming and implementation. The architecture of the machine is given in Figure 1.
 






























	Abbreviations:
		MAR	: Memory Address Register
		MBR	: memory Buffer Register
		RAM	: Random Access Memory
IR 	: Instruction Register
SP	: Stack Pointer
PC	: Program Counter
GPR	: General Purpose Register
ALU	: Arithmetic and Logic Unit
PSR	: Processor Status Register
Clock	: System clock has time in microseconds
			: Bidirectional data transfer path
			: Unidirectional data transfer path

Figure 1. Architecture of HYPO Machine.

2.1 Memory
Memory is word oriented. Each word can hold a 6 digits integer value. The system can have up to 1,000,000 words of memory. However, for the simulation purpose it is limited to 10,000 words or less depending on the memory available on the host machine used for simulation. To accommodate a 6-digit word, we need 32 bits on the host computer. If C language is used for simulation, the memory will be declared as an array of type long since long in language C is 32 bits. 

The memory unit has two registers called MAR and MBR. MAR stands for Memory Address Register and contains the address of the memory location to be accessed. MBR stands for Memory Buffer Register and contains the value of the location read or to be written.

2.2 CPU
The CPU contains 8 general-purpose registers (GPRs) number 0 thru 7. The size of each register is the same as the size of a word in memory. These eight registers are available to user for programming purposes. 

The CPU has two other special purpose registers: (1) Stack Pointer (SP), and (2) Program Counter (PC).  The content of SP is a memory address in the stack.  The SP points to the top of the stack, that is, the top most (latest) value stored on the stack.  The PC contains the address of the instruction memory location (address) to be fetched (read).

Every process created by the MTOPS operating system, will be assigned a stack area in the main memory and keep the pointer to the stack in SP by the operating system. Therefore, SP is a system stack pointer (sometime referred also as hardware stack pointer). The stack size is a predefined size in the operating system. Programs have access to SP through push and pop instructions only.  Programs should not modify SP and PC directly.  The CPU also has an Instruction Register (IR). It is three words long (since the instructions in HYPO can be 1 to 3 words long). The instructor decoder electronic is attached to the IR register to identify (decode) the instruction that has been fetched during the fetch cycle of an instruction execution.  The instruction set is explained in the next section.

3. INSTRUCTION SET

The HYPO is a two address machine. It means an instruction has information of two operands, operand1 (op1) and operand2 (op2).  It supports a number of essential instructions. They are described below.

3.1 Instruction Format

There are three types of instruction formats. They are:

1.	Zero address instructions
2.	Single address instructions
3.	Two address instructions

An instruction can have up to two operand addresses. The instruction formats are given in Figure 2. The first word of the instruction is divided into three fields: (1) operation code (opcode), (2) operand 1, and (3) operand 2. The five or six digits in the first word are divided as follows:

1.	The first one or two digits from left contain the opcode.
2.	The next two digits contain the address of the first operand.
3.	The least two digits contain the address of the second operand.

The two digit operand address is further divided into two subfields. They are:
	
1.	The left digit of the address field stands for mode number.
2.	The right digit of the address field stands for one of the GPRs.

Zero address instructions are always one word long. One address instructions, depending on the addressing mode, could be one or two words long. Two address instructions, depending on the addressing mode, could be one, two, or three words long. Addressing modes are explained in the next section.

 

One-Word Instruction Format


Two-Word Instruction Format



Three-Word Instruction Format






Size of Fields:
Opcode : 1 or 2 decimal digits
	Mode    : 1 decimal digit
	GPR      : 1 decimal digit

Note:  The fields in the first word is always the same in all three formats shown above.

Figure 2. HYPO instruction formats
 

3.2 Instructions Sets
The HYPO machine is a two address machine. There are eight essential instructions. They are listed below.

Instruction
	Opcode	Execution Time	Meaning
Halt	0	12	Halt execution
Add	1	3	Op1 op1 + op2
Subtract	2	3	Op1 op1 – op2
Multiply	3	6	Op1 op1*op2
Divide	4	6	Op1 op1/op2
Move	5	2	Op1 op2
Branch	6	2	PC next word, that is, second word in the instruction
Op1 and Op2 are not used
Branch on minus	7	4	If Op1 < 0 then 
PC next word of the instruction.
Op2 is not used.
Branch on Plus	8	4	If Op1 > 0 then 
PC next word of the instruction.
Op2 is not used.
Branch on zero	9	4	If Op1 = 0 then 
PC next word of the instruction.
Op2 is not used.
Push	10	2	Op1 is stored (push) on top of stack using SP.
Op2 is not used.
Pop	11	2	Op1  top of stack (pop) using SP.
Op2 is not used.
System Call	12	12	Op1 has system call ID as immediate operand. That is, second word of the instruction has the system call identifier.  Op2 is not used.




Note:
•	Op1 is operand 1 (destination operand in most cases).
•	Op2 is operand 2 (source operand in most cases).
•	‘next word’ is the word in the instruction (second or third word depending on the instruction and mode).  It contains the branch address for ‘branch’, ‘branch on minus’, ‘branch on plus’, and ‘branch on zero’ instructions.
•	System call is a two word instruction.  The second word has the system call identifier.  Each system call has a set of parameters. They are passed along with each system call using GPRs as described in the MTOP operating system kernel specification.
•	Instruction execution time is in microseconds

The operands op1 and op2 values could be either in the general-purpose registers or in the memory depending on the addressing mode specified in the operand address.

3.3 System Calls
System call requests operating system services such as reading from the keyboard device (input), displaying to the monitor device (output), create process, delete process, suspend process, send message, receive message, etc. The type of service requested depends on the system call identifier specified in the instruction.   

Each system call has a set of parameters, which varies depending on the system call identifier. The list of system calls, their parameters, and the way to pass parameters for each system call are described in the MTOPS operating system design report [1].  System call is always two-word instruction.

4. ADDRESSING MODES

There are six different addressing modes in the HYPO machine. They are explained below.

Mode #	Meaning	Description
0	Invalid / not used	Mode digit is not used in the instruction
1	Register mode	The specified GPR contains the operand value.
2	Register deferred	Register contains the address of the operand.
Operand value is in the main memory.
3	Autoincrement	Register contains the address of the operand; 
Operand value is in the main memory.
register content is incremented by 1 after fetching the operand value
4	Autodecrement	Register content is decremented by 1; decremented value is the address of the operand.
Operand value is in the main memory.
5	Direct mode	Next word contains the address of the operand.
		Operand value is in the main memory.
GPR is not used.
Use PC to get the address.
6	Immediate mode	Next word contains the operand value.
Operand value is in the main memory as part of the instruction.
GPR is not used.
Address is in the PC.



5. EXECUTABLE OBJECT MODULE FORMAT

As a user, you first write the program in assembly language and store it in a file.  This program may or may not use library functions already written by system programmers and made available to the user.  For example, the higher level language C++, has C++ library which contains code for input (cin) and output (cout) operations and code for string operation functions like strlen, strcmp, etc.

Second, the assembly program is translated into machine language program called object module.  This is normally done by the assembler for that computer.   

Third, the object module should be linked with library object module(s) to create a complete executable machine language program.

For the HYPO machine, no assembler program is available.  Hence, you have to manually translate your assembly program into machine language program.

There are no library functions for the HYPO machine. Hence, there is no need for linking your machine language object module.  In other words, the object module you create by translating the assembly program is a complete executable program.  This executable machine language program is stored in a file.

In order to execute a program, the executable machine language program must be read from disk file and loaded into main memory.  Hence, the executable object module specifies the memory location where each instruction has to be stored.  

The format of the executable object module is shown below.  Each line in the object module contains two numbers. The number in the first field is the address of a memory location.  The number in the second field is one word of the machine language instruction or the value of a variable specified in the program.  The second number is stored at the memory location specified in the first field.






End of Program Indicator: 
Loading an executable machine language program from disk to main memory is performed by a loader.  This program is supplied as part of the operating system.  The loader must know where the program ends. It should also know the address of the first instruction to be executed so that it can store that address in the program counter, PC.  The end of program indicator and the initial PC value are specified in the last line in the machine language program as shown below:









Memory addresses are not negative numbers.  In other words, a negative number is not a valid address.  Hence, it is used to indicate end of the module (program). For example, one can use -1 as end of program indicator.  The loader will stop loading after reading the line with negative address.  The loader will return the second number to be stored in the PC before program execution starts.

6. ASSEMBLY LANGUAGE SPECIFICATION

The following are the list of executable assembly language instructions for the HYPO machine.

Opcode	Mnemonic	Operands	Description
    0		 Halt		None		Stop program execution
    1		 Add		Op1,Op2	Op1 = Op1 + Op2
    2		 Subtract	Op1,Op2	Op1 = Op1 – Op2
    3		 Multiply	Op1,Op2	Op1 = Op1 * Op2
    4		 Divide		Op1,Op2	Op1 = Op1 / Op2  (integer division)
    5		 Move		Op1,Op2	Op1 = Op2
    6		 Branch	Address	PC = Address
    7		 BrOnMinus	Op1,Address	if (Op1 < 0), PC = Address, else PC++
    8		 BrOnPlus	Op1,Address	if (Op1 > 0), PC = Address else PC++
    9		 BrOnZero	Op1,Address	if (Op1 = 0), PC = Address, else PC++
    10		Push		Op1		SP++ then Memory[SP] = Op1
    11		Pop		Op1		Op1 = Memory[SP], then SP--
    12		 SystemCall	Op1		Op1 is the System Call Identifier

Non-Executable Instruction:

The following non-executable instructions have no opcode.  These instructions are also called assembler directives because they are instructions to the assembler directing the assembly process.  The Long instruction is used to declare variables.  Only one data type is available, which is long.  There are no data types like short, char, and others that are available in a high-level language.  Each variable takes one HYPO memory location.  There is no executable machine language instruction for these assembler drirectives.

Symbol	Mnemonic	Operands	Description
Variable	Long		Value		Allocate one memory location for a 
long integer and initialize it to “Value”
Name		Function			Start of a function called “Name”
		End		PCvalue	Indicates end of function.  The Program
						Counter (PC) should be set to PCvalue.
						In other words, execution starts at the
instruction pointed by PCvalue.
		Origin		Address	Next instruction starts at “Address”

If the Origin statement is not specified in an assembly program, then it is assumed to be zero.

Method of Specifying Register Deferred, Autoincrement, and Autodecrement Modes
The following is the way to specify (1) register deferred, (2) autoincorement, and (3) autodecrement modes in the HYPO assembly language.
Register deferred: (general purpose register#)
Autoincrement: (general purpose register#))++
Autodecrment: --( general purpose register#))
Notice that ++ and -- are similar to the high-level language notation like C++ or Java.
The above modes are illustrated with the Move instruction, which moves 1 to the general purpose register #3, that is, R3:
Move (R3), 1
Move (R3)++,1
Move --(R3),1
You will need the autoincrement mode and autodecrement modes to store values in the array to be initialized for test programs for homework#2.
When you make the system call the operating system returns the start address of the dynamically allocated array space from the user heap area/space in R1. You can copy the content of R1 into R3 and use R3 to initialize the array.

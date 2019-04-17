#!/usr/bin/python
import csv
import os
import re
import sys

#   __                  __  __           _
#  / /  __ _____  ___  / /_/ /  ___ ___ (_)__ ___ _______ ____
# / _ \/ // / _ \/ _ \/ __/ _ \/ -_|_-</ /_ // -_)___/ -_) __/
#/_//_/\_, / .__/\___/\__/_//_/\__/___/_//__/\__/    \__/_/
#     /___/_/
#
# Program opens a file of HYPO ASM and converts it to HYPO Machine Code
#
# The ASM file is splt into 3 columns:
#   1. The Label Column
#   2. The Instruction Column
#   3. The Operand Column
#
# An Example of a properly formated ASM file is included with this program.

# === CONSTANTS ===
codearray = {}                                  # Output Stream
instructions = {
        'Halt':0,'Add':1,'Subtract':2,'Multiply':3,'Divide':4,'Move':5,'Branch':6,\
        'BranchOnMinus':7,'BranchOnPlus':8,'BranchOnZero':9,'Push':10,'Pop':11,\
        'SystemCall':12 }
labels = {}                                     # Label Index Locations
asmlist = []                                    # In-Memory copy of CSV file

# === CSV LOADING ===
if len(sys.argv) == 1:
    print ("Program Usage:")
    print ("./hypothesize.py $inputfile\n")
    filename = input("Enter Filename: ")
else:
    filename = sys.argv[1]
csv.register_dialect('myDialect',
delimiter = ',',
quoting=csv.QUOTE_ALL,
skipinitialspace=True)
with open(filename, 'r') as csv_file:
    reader = csv.reader(csv_file, dialect='myDialect')
    for row in reader:
        row[2] = row[2].split(",")              # Bifurcate Operand Column
        while len(row[2]) < 2:                  # Ensure Number of Operand
            row[2].append("")
        asmlist.append(row)                     # Allows use across functions
csv_file.close()

# === DEFINITIONS ===
def pushline(value):
    global index
    codearray[index] = value
    index += 1
def parse():
    global asmlist
    global index
    global labels
    index = 0
    for row in asmlist:
        code = ""                               # Line is [Index] [MC sequence]
        wordqueue = []                          # Multi-word statements
        # print(row[0],row[1],row[2])

    # Column 1: ASSIGN INDEX LOCATION TO LABELS
        if row[0] != "":                        # If we discover a label...
            labels[row[0]] = index              # Actualize it's location

    # Column 2: INSTRUCTION
        if row[1] == "Long":                    # Skip operating if "Long"
            pushline(row[2][0])
            continue
        elif row[1] in instructions:
            code += str(instructions[row[1]])
        elif row[1] == "End":
            index = -1
            pushline(labels[row[2][0]])
            break;
        else:
            print("ERROR: Invalid word:", row[1])
            break;

    # Column 3: Operands
        for op in row[2]:
            print("OP >>>>> ", op)
            if re.match(r'[R]\d{1}$', op):      # Match "R" and 1 Digit
                code += "1" + str(op[1])        # Register Mode & Register Num
            elif op in labels:
                code += "50"                    # Direct Mode and 0
                wordqueue.append(labels[op])
            elif re.match(r'^\d+$', op) \
                    and row[1] != "Long":       # Match Any Number
                code += "60"                    # Immediate Mode and 0
                wordqueue.append(op)
            elif op == "":
                code += "00"
            else:
                print('ERROR: Invalid Operand', op)
                break;

        # Push Code Line to List
        pushline(code)

        # Handle and Flush Word-Queue
        for value in wordqueue:
            pushline(value)

### === MAIN === ###
# BUILD LIST OF ALL LABELS
for column in asmlist:
    if column[0] != "":                 # If First Column not empty...
        labels[column[0]] = "???"       # add it to Labels dictionary

for key,val in labels.items():          # DEBUG: List Labels
    print(key,val)

print("Parsing...")
parse()

# Reparse the file until unknown label locations are known
while '???' in codearray.values():      # In Practice only a second pass is ever required
    parse()

for key,val in labels.items():
    print(key,val)

# [DEBUG STATEMENTS]
print("---------------")

for key, val in codearray.items():
    print(key,val)

for val in asmlist:
    print(val)

# === WRITE ===
filename = os.path.splitext(filename)[0] + ".txt"   // $filename.txt
file = open(filename, "w")
for key, val in codearray.items():
    line = ('%s %s\n' % (str(key), str(val)))
    file.writelines(line);
file.close()

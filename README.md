# RV32IC_Simulator
# Computer Organization and Assembly Language Programming: Project 1: RV32IC Simulator

## Authors:
- Kirolous Fouty (900212444)
- Omar Ibrahim (900192095)
- Raafat El Saeed (900191080)
- Yehia Ragab (900204888)
- Ziyad Amin (900201190)

### Program Guide (How to use)
```
0- In case you wish to edit the source code, after applying your changes, compile "rvsim.cpp" into "rvsim.exe" using a C++ compiler such as g++.

1- Run the program through the cmd line that is open in the same folder of the program
   while providing the binary file you would like to simulate,
   and, optionally, a data file you wish to include.
   E.g. enter “rvism.exe t0.bin t0-d.bin” in the proper directory in a command line window.

2- The program will start simulating the given binary files,
   and printing the corresponding assembly instructions.
```

### Simulator Design
- First, the program examines the given instructions to determine whether it is in a compressed format or an uncompressed format format.
- If the instruction is compressed, the program proceeds to decompress it using a specialized decompressor. The decompressor transforms the compressed instruction into its corresponding uncompressed instruction.If the instruction is not compressed, the program skips the decompression step.
- After the decompression step, the program passes the uncompressed instruction to a translator. The translator is responsible for producing the appropriate assembly code representation of the instruction.
- Once the translator has produced the assembly code representation of the instruction, the program executes the resulting code.

### Simulator Limitations
- The simulator does not support floating-point integers, limiting its usefulness for applications that require high-precision calculations.
- Multiplication instructions are not supported by the simulator, which may restrict its effectiveness for programs that rely heavily on multiplication operations.

### Challenges Faced
The team encountered several challenges while working on the program.

- Extracting immediate values from instruction code proved to be difficult, particularly for instructions that were not properly indexed from left to right as MSB to LSB. This led toadditional efforts and time being expended in order to extract the necessary information for the program.
- Debugging the program was a major challenge, as tracing the assembly code required manually writing the instruction codes on paper and translating the instructions that were not working properly while tracing. This process was time-consuming and required a high level of attention to detail, which made debugging a slow and arduous process.
- Re-writing and translating compressed instructions into their decompressed alternatives was a difficult task, particularly for instructions that did not have similar 32 and 16 bit formats. This required significant effort in order to ensure that the instructions were properly translated and that the program could run effectively.
- Lack of resources online regarding compressed instructions, which made it difficult to find solutions to certain issues that arose during development.
- The project idea and implementation being self-contained to the extent that made the good practice of branching somewhat inapplicable and more of an unconvenience than a benefit.
- As the program development was reaching its end, organization started to suffer due to the large amount of instructions the team had to keep up with. This caused a large amount of redundancies in the code, which took some time to be removed once the program was complete and passed its tests.

### Issues and Problems
- The program passed the all of the given test cases successfully except t4 and t5. The reasons are as follows.
- In the case of uncompressed test case t4, the stack pointer overwrites a part of the data section. This causes the program to output "programmis awesome" instead of "programming is awesome".
- In the case of uncompressed and compressed t4, the string "After concatenation: " is not stored in the data file nor created then stored at runtime. This causes the program to output an empty string instead of "After concatenation: ".
- In the case of uncompressed and compressed t5, the data file is empty. This causes the program to output a string of trash values, not outputing "Length of the string: ", and outputing zero.
- In the case of compressed t5, the program infinitely loops because the instruction "0x000000b6 0x00008082 C.JR ra" jumps to address 0x00000000, which is the start of the program.



#include <iostream>
#include <fstream>
#include "stdlib.h"
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <vector>
using namespace std;

unsigned int pc = 0;
unsigned char memory[(16 + 64) * 1024] = {0};
unsigned int reg[32] = {0};
string name[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
vector<unsigned int> symbol_table;
int instCount = 0;
bool isSWSP = false;
bool isAddi4spn = false;
bool isAddI16SP = false;
bool isNop = false;
string filename;

void emitError(const char *s)
{
    cout << s;
    exit(0);
}

void printPrefix(unsigned int instA, unsigned int instW)
{
    cout << "0x" << hex << std::setfill('0') << std::setw(8) << instA << "\t0x" << std::setw(8) << instW;
}

void printRegisterValues()
{
    cout << "\n\nRegister Values:\n";
    for (int i = 0; i < 32; i++)
        cout << name[i] << " = 0x" << hex << reg[i] << endl;

    cout << "pc = 0x" << hex << pc << endl;
}

void printMemoryValues()
{
    cout << "\n\n\n";
    for (int i = 0; i < (16 + 64) * 1024; i++)
    {
        if (memory[i] != 0)
            cout << "memory[" << i << "] = " << hex << (int)memory[i] << endl;
    }
    cout << "\n\n\n";
}

unsigned int decompress(unsigned int instWord)
{

    unsigned int rd, rs1_dash, rs2_dash, rd_dash, rs2, funct4, funct3, opcode;
    unsigned int CS_imm, CS_imm_v2, CL_imm, JAL_Imm, CI_imm;
    unsigned int instPC = pc - 2;

    opcode = (instWord & 0x00000003);
    rs2 = ((instWord >> 2) & 0x0000001F);
    rd = ((instWord >> 7) & 0x0000001F);
    funct4 = ((instWord >> 12) & 0x0000000F);
    funct3 = ((instWord >> 13) & 0x00000007);
    rs1_dash = ((instWord >> 7) & 0x00000007);
    rs2_dash = ((instWord >> 2) & 0x00000007);
    rd_dash = ((instWord >> 2) & 0x00000007);

    rd_dash = rd_dash + 0b1000;
    rs1_dash = rs1_dash + 0b1000;
    rs2_dash = rs2_dash + 0b1000;

    CI_imm = (instWord >> 2);
    CI_imm = (CI_imm & 0x001F);
    CI_imm = CI_imm + ((instWord >> 7) & 0x0020);

    if (CI_imm & 0x0010)
    {
        CI_imm = CI_imm | 0xFFFFFFF0;
    }

    CS_imm = (instWord >> 5) & 0x00000001;
    CS_imm = CS_imm << 3;
    CS_imm = CS_imm + ((instWord >> 10) & 0x00000007);
    CS_imm = CS_imm << 1;
    CS_imm = CS_imm + ((instWord >> 6) & 0x00000001);
    CS_imm = CS_imm << 2;

    CS_imm_v2 = (instWord >> 10) & 0x00000007;
    CS_imm_v2 = CS_imm_v2 << 2;
    CS_imm_v2 = CS_imm_v2 + ((instWord >> 5) & 0x00000003);

    CL_imm = (instWord >> 5) & 0x00000001;
    CL_imm = CL_imm << 3;
    CL_imm = CL_imm + ((instWord >> 10) & 0x00000007);
    CL_imm = CL_imm << 1;
    CL_imm = CL_imm + ((instWord >> 6) & 0x00000001);
    CL_imm = CL_imm << 2;

    // Extract J-Immediate
    JAL_Imm = ((instWord >> 12) & 0x0001);
    JAL_Imm = JAL_Imm << 1;
    JAL_Imm = JAL_Imm + ((instWord >> 8) & 0x0001);
    JAL_Imm = JAL_Imm << 2;
    JAL_Imm = JAL_Imm + ((instWord >> 9) & 0x0003);
    JAL_Imm = JAL_Imm << 1;
    JAL_Imm = JAL_Imm + ((instWord >> 6) & 0x0001);
    JAL_Imm = JAL_Imm << 1;
    JAL_Imm = JAL_Imm + ((instWord >> 7) & 0x0001);
    JAL_Imm = JAL_Imm << 1;
    JAL_Imm = JAL_Imm + ((instWord >> 2) & 0x0001);
    JAL_Imm = JAL_Imm << 1;
    JAL_Imm = JAL_Imm + ((instWord >> 11) & 0x0001);
    JAL_Imm = JAL_Imm << 3;
    JAL_Imm = JAL_Imm + ((instWord >> 3) & 0x0007);
    JAL_Imm = JAL_Imm << 1;

    // Extract CB Immeidate for C.BEQZ and C.BNEZ
    unsigned int CB_Imm;

    CB_Imm = instWord >> 12;
    CB_Imm = CB_Imm & 0x0001;                      // 00000008
    CB_Imm = CB_Imm << 2;                          // 00000800
    CB_Imm = CB_Imm + ((instWord >> 5) & 0x0003);  // 00000876
    CB_Imm = CB_Imm << 1;                          // 00008760
    CB_Imm = CB_Imm + ((instWord >> 2) & 0x0001);  // 00008765
    CB_Imm = CB_Imm << 2;                          // 00876500
    CB_Imm = CB_Imm + ((instWord >> 10) & 0x0003); // 00876543
    CB_Imm = CB_Imm << 2;                          // 87654300
    CB_Imm = CB_Imm + ((instWord >> 3) & 0x0003);  // 00876543
    CB_Imm = CB_Imm << 1;

    if (CB_Imm >> 8 == 1)
    {
        CB_Imm = CB_Imm | 0b1111000000000;
    }
    else
    {
        CB_Imm = CB_Imm | 0b0000000000000;
    }

    int MSF;
    printPrefix(instPC, instWord);
    unsigned int instWord_Decompressed = 0;

    if (opcode == 0x2)
    {
        // CR Format
        switch (funct4)
        {
        case 0x9:
        {
            if (rd != 0 && rs2 != 0)
            {
                // C.ADD --------> ADD
                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }
            else if (rd != 0 && rs2 == 0)
            {
                // C.JALR -------> JALR
                instWord_Decompressed = 0b000000000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + 0b00001;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b1100111;

                return instWord_Decompressed;
            }
            break;
        }

        case 0x8:
        {
            if (rd != 0 && rs2 == 0)
            {
                // C.JR ------> JALR
                instWord_Decompressed = 0b000000000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + 0b00000;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b1100111;

                return instWord_Decompressed;
            }
            else if (rd != 0 && rs2 != 0)
            {
                // C.MV ------> ADD
                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + 0b00000;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }
            break;
        }
        }

        switch (funct3)
        {
        case 0x0:
        {
            int CI_imm = instWord >> 2; // C.slli
            CI_imm = (CI_imm & 0x001F);
            CI_imm = CI_imm + ((instWord >> 7) & 0x0020);

            if (CI_imm >> 5)
                CI_imm = CI_imm | 0xFC0;

            else
                CI_imm = CI_imm | 0x0;

            if (rd != 0)
                instWord_Decompressed = 0x0;

            opcode = 0x13;
            funct3 = 0x1;
            rd = instWord >> 7;
            rd = (rd & 0x001F);
            instWord_Decompressed = CI_imm << 20;
            instWord_Decompressed = instWord_Decompressed + (rd << 15);
            instWord_Decompressed = instWord_Decompressed + (funct3 << 12);
            instWord_Decompressed = instWord_Decompressed + (rd << 7);
            instWord_Decompressed = instWord_Decompressed + opcode;
            return instWord_Decompressed;
            break;
        }

        case 0x2: // C.LWSP
        {
            unsigned int CILW_imm = instWord >> 2;
            CILW_imm = (CILW_imm & 0x0003);                    // 00076
            CILW_imm = CILW_imm << 1;                          // 00760
            CILW_imm = CILW_imm + ((instWord >> 11) & 0x0001); // 00765
            CILW_imm = CILW_imm << 2;                          // 87600
            CILW_imm = CILW_imm + ((instWord >> 4) & 0x0007);  // 87654
            CILW_imm = CILW_imm << 2;

            int I_rd = instWord >> 7;
            I_rd &= 0x001F;
            instWord_Decompressed = 0x0;
            if (I_rd != 0)
            {
                opcode = 0x03;
                funct3 = 0x2;
                instWord_Decompressed = CILW_imm << 20;
                instWord_Decompressed = instWord_Decompressed + (0x2 << 15);
                instWord_Decompressed = instWord_Decompressed + (funct3 << 12);
                instWord_Decompressed = instWord_Decompressed + (rd << 7);
                instWord_Decompressed = instWord_Decompressed + opcode;

                return instWord_Decompressed;
            }
            cout << "\nError?\n"; // debugging
            return instWord_Decompressed;
            break;
        }

        case 0x6:
        {
            // C.SWSP

            isSWSP = true;
            unsigned int opcode = 0;
            unsigned int rs2 = 0;
            unsigned int CSS_imm = 0;
            instWord_Decompressed = 0;

            opcode = 0b0100011;
            rs2 = ((instWord >> 2) & 0b11111);

            CSS_imm = ((instWord >> 8) & 0b1) << 7;
            CSS_imm |= ((instWord >> 7) & 0b1) << 6;
            CSS_imm |= ((instWord >> 9) & 0b1111) << 2;

            if (CSS_imm >> 7)
                CSS_imm |= 0xFFFFFF00;
            else
                CSS_imm &= 0x000000FF;

            funct3 = ((instWord >> 13) & 0b111);

            instWord_Decompressed = (opcode & 0b1111111);
            instWord_Decompressed |= ((CSS_imm & 0b11111) << 7);
            instWord_Decompressed |= ((0b010) << 12);
            instWord_Decompressed |= ((0b010) << 15);
            instWord_Decompressed |= (rs2 << 20);
            instWord_Decompressed |= ((CSS_imm & 0b111111100000) << 20);
            return instWord_Decompressed;

            break;
        }
        }
    }

    else if (opcode == 0x0)
    {

        switch (funct3)
        {

        case 0x0: // addi4spn
        {
            unsigned int CIW_Imm = 0;
            CIW_Imm = ((instWord >> 7) & 0b0000000000000111);

            CIW_Imm = CIW_Imm << 2;
            CIW_Imm = CIW_Imm + ((instWord >> 11) & 0b0000000000000011);

            CIW_Imm = CIW_Imm << 1;

            CIW_Imm = CIW_Imm + ((instWord >> 5) & 0b0000000000000001);

            CIW_Imm = CIW_Imm << 1;
            CIW_Imm = CIW_Imm + ((instWord >> 6) & 0b0000000000000001);
            CIW_Imm = CIW_Imm << 2;

            CIW_Imm = CIW_Imm | 0b000000000000;

            instWord_Decompressed = CIW_Imm;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + 0b00010;
            instWord_Decompressed = instWord_Decompressed << 3;
            instWord_Decompressed = instWord_Decompressed + 0b000;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rd_dash;
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b0010011;

            isAddi4spn = true;

            return instWord_Decompressed;
            break;
        }
        case 0x6:
        {
            // C.SW
            MSF = ((instWord >> 5) & 0x00000001);

            if (MSF == 1)
            {

                instWord_Decompressed = 0b1111111;
            }

            else if (MSF == 0)
            {
                instWord_Decompressed = 0b0000000;
            }

            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rs1_dash;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rs2_dash;
            instWord_Decompressed = instWord_Decompressed << 3;
            instWord_Decompressed = instWord_Decompressed + 0b010;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + CS_imm;
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b0100011;

            return instWord_Decompressed;
            break;
        }

        case 0x2:
        {
            // C.LW
            MSF = ((instWord >> 5) & 0x00000001);

            if (MSF == 1)
            {
                instWord_Decompressed = 0b1111111;
            }

            else if (MSF == 0)
            {
                instWord_Decompressed = 0b0000000;
            }
            instWord_Decompressed = instWord_Decompressed + CL_imm;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rs1_dash;
            instWord_Decompressed = instWord_Decompressed << 3;
            instWord_Decompressed = instWord_Decompressed + 0b010;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rd_dash;
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b0000011;

            return instWord_Decompressed;
            break;
        }
        }
    }

    else if (opcode == 0x1)
    {

        switch (funct3)
        {
        case 0x0:
        {

            if (rd == 0)
            {
                // C.NOP

                instWord_Decompressed = 0b00000000000000000000000000010011;

                isNop = true;
                return instWord_Decompressed;
            }

            else
            {
                // C.ADDI

                int I_rd = instWord >> 7;
                I_rd &= 0x001F;

                int CI_imm = instWord >> 2;
                CI_imm = (CI_imm & 0x001F);
                CI_imm = CI_imm + ((instWord >> 7) & 0x0020);
                if (CI_imm >> 5)
                {
                    CI_imm = CI_imm | 0xFC0;
                }
                else
                {
                    CI_imm = CI_imm | 0x0;
                }

                instWord_Decompressed = 0x0;
                opcode = 0x13;
                funct3 = 0x0;
                rd = instWord >> 7;
                rd = (rd & 0x001F);
                instWord_Decompressed = CI_imm << 20;
                instWord_Decompressed = instWord_Decompressed + (rd << 15);
                instWord_Decompressed = instWord_Decompressed + (funct3 << 12);
                instWord_Decompressed = instWord_Decompressed + (rd << 7);
                instWord_Decompressed = instWord_Decompressed + opcode;
                return instWord_Decompressed;
            }
            break;
        }

        case 0x4:
        {
            if (CS_imm_v2 == 0b01111)
            {
                // C.AND

                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2_dash;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b111;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }

            else if (CS_imm_v2 == 0b01110)
            {
                // C.OR

                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2_dash;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b110;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }

            else if (CS_imm_v2 == 0b01101)
            {
                // C.XOR

                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2_dash;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b100;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }

            else if (CS_imm_v2 == 0b01100)
            {
                // C.SUB

                instWord_Decompressed = 0b00100000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs2_dash;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110011;

                return instWord_Decompressed;
            }

            else if ((((instWord >> 10) & 0b11) == 0) && ((instWord >> 12) & 1) == 0) // C.SRLI
            {
                opcode = 0x13;
                funct3 = 0x5;
                rd = ((instWord >> 7) & 0b111);
                rd += 8;
                CS_imm_v2 = ((instWord >> 2) & 0b1111);

                instWord_Decompressed = (CS_imm_v2 << 20);
                instWord_Decompressed |= (rd << 15);
                instWord_Decompressed |= (funct3 << 12);
                instWord_Decompressed |= (rd << 7);
                instWord_Decompressed |= opcode;
                return instWord_Decompressed;
            }
            else if ((((instWord >> 10) & 0b11) == 0b10))
            {
                // C.ANDI
                CS_imm_v2 = ((instWord >> 12) & 0b1) << 5;
                CS_imm_v2 |= ((instWord >> 2) & 0b11111);

                instWord_Decompressed = 0b0000000;
                instWord_Decompressed = (instWord_Decompressed << 5);
                instWord_Decompressed = instWord_Decompressed + CS_imm_v2;
                instWord_Decompressed = (instWord_Decompressed << 5);
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = (instWord_Decompressed << 3);
                instWord_Decompressed = instWord_Decompressed + 0b111;
                instWord_Decompressed = (instWord_Decompressed << 5);
                instWord_Decompressed = instWord_Decompressed + rs1_dash;
                instWord_Decompressed = (instWord_Decompressed << 7);
                instWord_Decompressed = instWord_Decompressed + 0b0010011;
                return instWord_Decompressed;
            }
            else if (rd != 0) // C.SRAI
                instWord_Decompressed = 0x0;

            opcode = 0x13;
            funct3 = 0x5;
            rd = ((instWord >> 7) & 0x001F);
            instWord_Decompressed = CI_imm << 20;
            instWord_Decompressed = instWord_Decompressed + (rd << 15);
            instWord_Decompressed = instWord_Decompressed + (funct3 << 12);
            instWord_Decompressed = instWord_Decompressed + (rd << 7);
            instWord_Decompressed = instWord_Decompressed | 0x40000000;
            instWord_Decompressed = instWord_Decompressed + opcode;

            return instWord_Decompressed;

            break;
        }

        case 0x1:

        {
            // C.JAL ------> JAL

            if (JAL_Imm >> 11 == 1)
            {

                JAL_Imm = JAL_Imm | 0b111111111000000000000;
            }

            else
            {

                JAL_Imm = JAL_Imm | 0b000000000000000000000;
            }

            instWord_Decompressed = JAL_Imm >> 20;
            instWord_Decompressed = instWord_Decompressed << 10;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 1) & 0b000000000001111111111);
            instWord_Decompressed = instWord_Decompressed << 1;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 11) & 0b000000000000000000001);
            instWord_Decompressed = instWord_Decompressed << 8;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 12) & 0b000000000000011111111);
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + 0b00001;
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b1101111;

            return instWord_Decompressed;
            break;
        }

        case 0x2: // C.LI -------> ADDI
        {
            int I_rd = instWord >> 7;
            I_rd &= 0x001F;

            int CI_imm = instWord >> 2;
            CI_imm = (CI_imm & 0x001F);
            CI_imm = CI_imm + ((instWord >> 7) & 0x0020);
            if (CI_imm >> 5)
            {
                CI_imm = CI_imm | 0xFC0;
            }
            else
            {
                CI_imm = CI_imm | 0x0;
            }

            if (I_rd != 0)
            {
                instWord_Decompressed = 0x0;
                opcode = 0x13;
                funct3 = 0x0;
                instWord_Decompressed = CI_imm << 20;
                instWord_Decompressed = instWord_Decompressed + 0b00000;
                instWord_Decompressed = instWord_Decompressed + (0x00 << 12);
                instWord_Decompressed = instWord_Decompressed + (rd << 7);
                instWord_Decompressed = instWord_Decompressed + opcode;
            }

            return instWord_Decompressed;
            break;
        }

        case 0x3:
        {

            if (rd == 2)
            {
                // C.ADDI16SP

                unsigned int CI16_imm = ((instWord >> 12) & 0b0000000000000001);
                CI16_imm = CI16_imm << 2;
                CI16_imm = CI16_imm + ((instWord >> 3) & 0b0000000000000011);
                CI16_imm = CI16_imm << 1;
                CI16_imm = CI16_imm + ((instWord >> 5) & 0b0000000000000001);
                CI16_imm = CI16_imm << 1;
                CI16_imm = CI16_imm + ((instWord >> 2) & 0b0000000000000001);
                CI16_imm = CI16_imm << 1;
                CI16_imm = CI16_imm + ((instWord >> 6) & 0b0000000000000001);

                CI16_imm = CI16_imm << 4;

                if (CI16_imm >> 9 == 1)
                {
                    CI16_imm = (CI16_imm | 0b110000000000);
                }
                else
                {
                    CI16_imm = (CI16_imm | 0b000000000000);
                }

                instWord_Decompressed = CI16_imm;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 3;
                instWord_Decompressed = instWord_Decompressed + 0b000;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0010011;

                isAddI16SP = true;

                return instWord_Decompressed;
            }

            else
            {
                // C.LUI

                unsigned int CI_imm_lui;
                CI_imm_lui = ((instWord >> 12) & 0x1);
                CI_imm_lui = CI_imm_lui << 5;
                CI_imm_lui = CI_imm_lui + ((instWord >> 2) & 0b0000000000011111);
                CI_imm_lui = CI_imm_lui << 12;

                if (CI_imm_lui >> 17 == 1)
                {

                    CI_imm_lui = (CI_imm_lui | 0b11111111111111000000000000000000);
                }

                else
                {

                    CI_imm_lui = (CI_imm_lui | 0b00000000000000000000000000000000);
                }

                instWord_Decompressed = CI_imm_lui >> 12;
                instWord_Decompressed = instWord_Decompressed << 5;
                instWord_Decompressed = instWord_Decompressed + rd;
                instWord_Decompressed = instWord_Decompressed << 7;
                instWord_Decompressed = instWord_Decompressed + 0b0110111;

                return instWord_Decompressed;
            }

            break;
        }

        case 0x5:

        {
            // C.J -------> JAL

            if (JAL_Imm >> 11 == 1)
            {

                JAL_Imm = JAL_Imm | 0b111111111000000000000;
            }

            else
            {

                JAL_Imm = JAL_Imm | 0b000000000000000000000;
            }

            instWord_Decompressed = JAL_Imm >> 20;
            instWord_Decompressed = instWord_Decompressed << 10;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 1) & 0b000000000001111111111);
            instWord_Decompressed = instWord_Decompressed << 1;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 11) & 0b000000000000000000001);
            instWord_Decompressed = instWord_Decompressed << 8;
            instWord_Decompressed = instWord_Decompressed + ((JAL_Imm >> 12) & 0b000000000000011111111);
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + 0b00000;
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b1101111;

            return instWord_Decompressed;
            break;
        }

        case 0x6:
        {
            // C.BEQZ

            instWord_Decompressed = ((CB_Imm >> 12) & 0x1);
            instWord_Decompressed = instWord_Decompressed << 6;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 5) & 0b0000000111111);
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + 0b00000;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rs1_dash;
            instWord_Decompressed = instWord_Decompressed << 3;
            instWord_Decompressed = instWord_Decompressed + 0b000;
            instWord_Decompressed = instWord_Decompressed << 4;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 1) & 0b0000000001111);
            instWord_Decompressed = instWord_Decompressed << 1;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 11) & 0b0000000000001);
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b1100011;

            return instWord_Decompressed;
            break;
        }

        case 0x7:
        {
            // C.BNEZ

            instWord_Decompressed = ((CB_Imm >> 12) & 0x1);
            instWord_Decompressed = instWord_Decompressed << 6;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 5) & 0b0000000111111);
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + 0b00000;
            instWord_Decompressed = instWord_Decompressed << 5;
            instWord_Decompressed = instWord_Decompressed + rs1_dash;
            instWord_Decompressed = instWord_Decompressed << 3;
            instWord_Decompressed = instWord_Decompressed + 0b001;
            instWord_Decompressed = instWord_Decompressed << 4;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 1) & 0b0000000001111);
            instWord_Decompressed = instWord_Decompressed << 1;
            instWord_Decompressed = instWord_Decompressed + ((CB_Imm >> 11) & 0b0000000000001);
            instWord_Decompressed = instWord_Decompressed << 7;
            instWord_Decompressed = instWord_Decompressed + 0b1100011;

            return instWord_Decompressed;
            break;
        }
        }
    }
    return instWord_Decompressed;
}

void instDecExec(unsigned int instWord, bool isCompressed)
{

    unsigned int rd, rs1, rs2, funct3, funct7, opcode;
    // signed int I_imm, S_imm, B_imm, U_imm, J_imm;
    unsigned int I_imm, S_imm, B_imm, U_imm, J_imm;
    unsigned int address;

    unsigned int instPC;
    if (isCompressed == 0)
    {
        instPC = pc - 4;
    }

    else if (isCompressed == 1)
    {

        instPC = pc - 2;
    }

    opcode = instWord & 0x0000007F;

    rd = ((instWord >> 7) & 0x0000001F);
    funct3 = ((instWord >> 12) & 0x00000007);
    rs1 = ((instWord >> 15) & 0x0000001F);
    rs2 = ((instWord >> 20) & 0x0000001F);
    funct7 = ((instWord >> 25) & 0x0000007F);

    // reset
    I_imm = 0;
    S_imm = 0;
    B_imm = 0;
    U_imm = 0;
    J_imm = 0;

    // Extract I-Immediate
    I_imm = ((instWord >> 20) & 0x7FF) | (((instWord >> 31) ? 0xFFFFF800 : 0x0));

    // Extract S-Immediate
    S_imm = (instWord >> 25);
    S_imm <<= 5;
    S_imm |= ((instWord >> 7) & 0b11111);

    if (S_imm >> 11)
        S_imm |= 0xFFFFF000;

    // Extract B-Immediate
    B_imm = ((instWord >> 31) << 12);
    B_imm |= ((instWord & 0x80) << 4);
    B_imm |= ((instWord & 0x7E000000) >> 20);
    B_imm |= ((instWord & 0xF00) >> 7);
    if (B_imm >> 12)
        B_imm |= 0xFFFFF000;

    // Extract U-Immediate
    U_imm = (instWord >> 12);
    U_imm = (U_imm << 12);

    // Extract J-Immediate
    J_imm = ((instWord >> 31) << 20);
    J_imm |= (instWord & 0xFF000);
    J_imm |= ((instWord & 0x100000) >> 9);
    J_imm |= (((instWord & 0x7FE00000) >> 21) << 1);
    if (J_imm >> 20)
        J_imm |= 0xFFE00000;

    if (!isCompressed)
    {
        printPrefix(instPC, instWord);
    }

    if (opcode == 0x33)
    { // R Instructions

        switch (funct3)
        {
        case 0x0:
        {
            if (funct7 == 0x0)
            {
                // 1.ADD

                if (isCompressed == 0)
                {
                    cout << "\tADD\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                }
                else if (isCompressed == 1)
                {

                    if (rs1 == 0b00000)
                    {
                        cout << "\tC.MV\t" << name[rd] << ", " << name[rs2] << "\n";
                    }
                    else
                    {
                        cout << "\tC.ADD\t" << name[rd] << ", " << name[rs2] << "\n";
                    }
                }

                reg[rd] = reg[rs1] + reg[rs2];
            }
            else if (funct7 == 0x20)
            {
                // 2.SUB

                if (isCompressed == 0)
                {
                    cout << "\tSUB\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                }
                else if (isCompressed == 1)
                {
                    cout << "\tC.SUB\t" << name[rd] << ", " << name[rs2] << "\n";
                }

                // reg[rd] = reg[rs1] - reg[rs2]; // debugging: signed or unsigned?
                reg[rd] = reg[rs1] - reg[rs2];
            }
            break;
        }

        case 0x4:
        {
            if (funct7 == 0x00)
            {
                // 3.XOR

                if (isCompressed == 0)
                {
                    cout << "\tXOR\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                }

                else if (isCompressed == 1)
                {
                    cout << "\tC.XOR\t" << name[rd] << ", " << name[rs2] << "\n";
                }

                reg[rd] = reg[rs1] ^ reg[rs2];
            }
            break;
        }

        case 0x6:
        {
            if (funct7 == 0x00)
            {
                // 4.OR
                if (isCompressed == 0)
                {
                    cout << "\tOR\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                }
                if (isCompressed == 1)
                {
                    cout << "\tC.OR\t" << name[rd] << ", " << name[rs2] << "\n";
                }

                reg[rd] = reg[rs1] | reg[rs2];
            }
            break;
        }

        case 0x7:
        {
            if (funct7 == 0x00)
            {
                // 5.AND

                if (isCompressed == 0)
                {
                    cout << "\tAND\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                }
                else if (isCompressed == 1)
                {
                    cout << "\tC.AND\t" << name[rd] << ", " << name[rs2] << "\n";
                }

                reg[rd] = reg[rs1] & reg[rs2];
            }
            break;
        }

        case 0x1:
        {
            if (funct7 == 0x00)
            {
                // 6.SLL
                cout << "\tSLL\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                reg[rd] = reg[rs1] << reg[rs2];
            }
            break;
        }
        case 0x5:
        {
            if (funct7 == 0x00)
            {
                // 7.SRL
                cout << "\tSRL\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                reg[rd] = reg[rs1] >> reg[rs2];
            }
            else if (funct7 == 0x20)
            {
                // 8.SRA
                cout << "\tSRA\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                unsigned int temp = reg[rs2];
                unsigned int isNeg = reg[rs1] & 0x80000000;
                reg[rd] = reg[rs1] >> reg[rs2];

                if (isNeg)
                {
                    for (unsigned int i = 0; i < temp; i++)
                    {
                        reg[rd] = reg[rd] | (isNeg);
                        isNeg = isNeg >> 1;
                    }
                }
            }
            break;
        }

        case 0x2:
        {
            if (funct7 == 0x0)
            {
                // 9.SLT
                cout << "\tSLT\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                if ((int)(reg[rs1]) < (int)(reg[rs2]))
                {
                    reg[rd] = 1;
                }
                else
                {
                    reg[rd] = 0;
                }
            }
            break;
        }

        case 0x3:
        {
            if (funct7 == 0x0)
            {
                // 10.SLTU
                cout << "\tSLTU\t" << name[rd] << ", " << name[rs1] << ", " << name[rs2] << "\n";
                if (reg[rs1] < reg[rs2])
                {
                    reg[rd] = 1;
                }
                else
                {
                    reg[rd] = 0;
                }
            }
            break;
        }

        default:
            cout << "\tUnkown R Instruction \n";
        }
    }

    else if (opcode == 0x3B)
    {
        // R instructions
        cout << "\tUnkown R Instruction \n";
    }

    else if (opcode == 0x13)
    {
        // I instructions
        switch (funct3)
        {
        case 0:
        {
            // 11.ADDI

            if (isCompressed)
            {

                if (isAddi4spn == true)
                {
                    cout << "\tC.ADDI4SPN\t" << name[rd] << ", " << dec << (int)I_imm / 4 << "\n";
                    isAddi4spn = false;
                }

                else if (isAddI16SP == true)
                {
                    cout << "\tC.ADDI16SP\t" << name[rd] << ", " << dec << (int)I_imm / 16 << "\n";
                    isAddI16SP = false;
                }

                else if (isNop == true)
                {

                    cout << "\tC.Nop\t"
                         << "\n";
                    isNop = false;
                }
                else if (rs1 == 0)
                {

                    cout << "\tC.LI\t" << name[rd] << ", " << dec << (int)I_imm << "\n";
                }

                else
                {

                    cout << "\tC.ADDI\t" << name[rd] << ", " << dec << (int)I_imm << "\n";
                }
            }
            else
            {
                cout << "\tADDI\t" << name[rd] << ", " << name[rs1] << ", " << dec << (int)I_imm << "\n";
            }

            reg[rd] = (int)reg[rs1] + (int)I_imm;

            break;
        }
        case 0x1:
        {
            // 12.SLLI
            if (isCompressed == 0)
            {
                cout << "\tSLLI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            }

            else if (isCompressed == 1)
            {
                cout << "\tC.SLLI\t" << name[rd] << ", " << hex << "0x" << (int)I_imm << "\n";
            }
            I_imm = I_imm & 0b000000011111;
            reg[rd] = reg[rs1] << I_imm;

            break;
        }
        case 0x2:
        {
            // 13.SLTI
            cout << "\tSLTI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            if ((int)(reg[rs1]) < (int)I_imm)
            {
                reg[rd] = 1;
            }
            else
            {
                reg[rd] = 0;
            }

            break;
        }
        case 0x3:
        {
            // 14.SLTIU
            cout << "\tSLTIU\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            if (reg[rs1] < I_imm)
            {
                reg[rd] = 1;
            }
            else
            {
                reg[rd] = 0;
            }

            break;
        }

        case 0x4:
        {
            // 15.XORI
            cout << "\tXORI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            reg[rd] = reg[rs1] ^ I_imm;
            break;
        }
        case 0x5:
        {
            if (funct7 == 0x00)
            {
                // 16.SRLI
                if (isCompressed)
                {
                    cout << "\tC.SRLI\t" << name[rd] << ", " << hex << "0x" << (int)I_imm << "\n";
                }
                else
                {
                    cout << "\tSRLI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
                }
                I_imm = I_imm & 0b000000011111;
                reg[rd] = reg[rs1] >> I_imm;
            }
            else if (funct7 == 0x20)
            {
                // 17.SRAI
                I_imm = I_imm & 0b000000011111;
                cout << "\tSRAI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";

                unsigned int temp = rs2;
                unsigned int isNeg = reg[rs1] & 0x80000000;

                reg[rd] = reg[rs1] >> I_imm;

                for (unsigned int i = 0; i < temp; i++)
                {
                    reg[rd] = reg[rd] | (isNeg);
                    isNeg = isNeg >> 1;
                }
            }
            break;
        }

        case 0x6:
        {
            // 18.ORI
            cout << "\tORI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            reg[rd] = reg[rs1] | I_imm;
            break;
        }

        case 0x7:
        {
            // 19.ANDI

            if (isCompressed)
            {
                cout << "\tC.ANDI\t" << name[rd] << ", " << hex << "0x" << (int)I_imm << "\n";
            }
            else
            {
                cout << "\tANDI\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            }
            reg[rd] = reg[rs1] & I_imm;
            break;
        }

        default:
            cout << "\tUnkown I Instruction \n";
        }
    }

    else if (opcode == 0x03)
    {
        switch (funct3)
        {
        case 0x0:

        {
            // 20.lb
            cout << "\tLB\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";
            reg[rd] = memory[reg[rs1] + (int)I_imm];

            // sign extension
            if (reg[rd] & 0x00000080)
            {
                reg[rd] |= 0xFFFFFF00;
            }
            else
            {
                reg[rd] &= 0x000000FF;
            }

            break;
        }

        case 0x1:

        {
            // 21.lh
            cout << "\tLH\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";

            unsigned int temp;
            unsigned int data;
            temp = reg[rs1] + (int)I_imm;

            data = memory[temp];
            data |= (memory[temp + 1] << 8);

            reg[rd] = data;

            // sign extension
            if (reg[rd] & 0x00008000)
            {
                reg[rd] |= 0xFFFF0000;
            }
            else
            {
                reg[rd] &= 0x0000FFFF;
            }

            break;
        }
        case 0x2:
        {
            // 22.lw
            if (isCompressed)
            {

                if (rs1 == 2)
                {
                    cout << "\tC.LWSP\t" << name[rd] << ", " << dec << (int)I_imm << "\n";
                }

                else
                {
                    cout << "\tC.LW\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";
                }
            }
            else
            {
                cout << "\tLW\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";
            }

            unsigned int data;
            unsigned int temp;

            temp = reg[rs1] + (int)I_imm;

            data = memory[temp];
            data |= (memory[temp + 1] << 8);
            data |= (memory[temp + 2] << 16);
            data |= (memory[temp + 3] << 24);

            reg[rd] = data;

            break;
        }
        case 0x4:
        {
            // 23.lbu
            cout << "\tLBU\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";
            reg[rd] = memory[reg[rs1] + (int)I_imm];
            reg[rd] &= 0x000000FF;
            break;
        }
        case 0x5:
        {
            // 24.lhu
            cout << "\tLHU\t" << name[rd] << ", " << dec << (int)I_imm << "(" << name[rs1] << ")\n";

            unsigned int data;
            unsigned int temp;
            temp = reg[rs1] + (int)I_imm;

            data = memory[temp];
            data |= (memory[temp + 1] << 8);
            data &= 0x0000FFFF;

            reg[rd] = data;

            break;
        }
        }
    }

    else if (opcode == 0x73)
    {
        // 25.ECALL
        cout << "\tECALL\n";
        if (reg[17] == 1) // if a7==1 print a0 integer
        {
            cout << dec << (int)reg[10] << endl;
        }
        else if (reg[17] == 4)
        {
            // if a7==4 print a0 string
            int i = 0;

            while (memory[reg[10] + i] != 0)
            {
                cout << (char)(memory[reg[10] + i]);
                i++;
            }
            cout << endl;
        }
        else if (reg[17] == 10)
        {
            exit(0);
        }
    }

    else if (opcode == 0x0F)
    {
        // I instructions
        cout << "\tUnkown I Instruction \n";
    }

    else if (opcode == 0x23)
    {
        // S instructions
        switch (funct3)
        {

        case 0x0:
        {
            // 26.SB
            cout << "\tSB\t" << name[rs2] << ", " << (int)S_imm << "(" << name[rs1] << ")\n";

            if ((S_imm >> 11) == 1)
            {
                S_imm |= 0b11111111111111111111100000000000;
            }

            memory[reg[rs1] + (int)S_imm] = (reg[rs2] & 0x000000FF);

            break;
        }

        case 0x1:
        {
            // 27.SH
            cout << "\tSH\t" << name[rs2] << ", " << (int)S_imm << "(" << name[rs1] << ")\n";

            if ((S_imm >> 11) == 1)
            {
                S_imm |= 0b11111111111111111111100000000000;
            }

            unsigned int temp = reg[rs1] + (int)S_imm;
            unsigned int data = reg[rs2];

            memory[temp] = (data & 0x000000FF);
            memory[temp + 1] = ((data >> 8) & 0x000000FF); // debugging: review

            break;
        }

        case 0x2:
        {
            // 28.SW
            if (isCompressed == 0)
            {

                cout << "\tSW\t" << name[rs2] << ", " << dec << (int)S_imm << "(" << name[rs1] << ")\n";
            }

            else if (isCompressed == 1)
            {

                if (rs1 == 2)
                {
                    cout << "\tC.SWSP\t" << name[rs2] << ", " << dec << (int)S_imm << "\n";
                }
            }
            if ((S_imm >> 11) == 1)
            {
                S_imm |= 0b11111111111111111111100000000000;
            }

            unsigned int temp = reg[rs1] + int(S_imm);
            unsigned int data = reg[rs2];

            memory[temp] = data & 0xFF;
            memory[temp + 1] = (data >> 8) & 0xFF;
            memory[temp + 2] = (data >> 16) & 0xFF;
            memory[temp + 3] = (data >> 24) & 0xFF;

            break;
        }

        default:
            cout << "\tUnknown S Instruction\n";
        }
    }

    else if (opcode == 0x63)
    {
        // B instructions
        switch (funct3)
        {
        case 0x0:
        {
            // 29.BEQ
            if (isCompressed == 0)
            {
                cout << "\tBEQ\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            }

            else if (isCompressed == 1)
            {

                if (rs2 == 0)
                {
                    cout << "\tC.BEQZ\t" << name[rs1] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
                }
            }

            if (reg[rs1] == reg[rs2])
            {

                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff;  // debugging: review
            }

            break;
        }

        case 0x1:
        {
            // 30.BNE

            if (isCompressed == 0)
            {
                cout << "\tBNE\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            }
            else if (isCompressed == 1)
            {

                if (rs2 == 0)
                {
                    cout << "\tC.BNEZ\t" << name[rs1] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
                }
            }

            if (reg[rs1] != reg[rs2])
            {
                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff; // debugging: review
            }

            break;
        }
        case 0x4:
        {
            // 31.BLT
            cout << "\tBLT\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            if ((int)reg[rs1] < (int)reg[rs2])
            {
                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff; // debugging: review
            }
            break;
        }
        case 0x5:
        {
            // 32.BGE
            cout << "\tBGE\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            if ((int)reg[rs1] >= (int)reg[rs2])
            {
                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff; // debugging: review
            }
            break;
        }
        case 0x6:
        {
            // 33.BLTU
            cout << "\tBLTU\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            if (reg[rs1] < reg[rs2])
            {
                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff; // debugging: review
            }
            break;
        }
        case 0x7:
        {
            // 34.BGEU
            cout << "\tBGEU\t" << name[rs1] << ", " << name[rs2] << ", " << hex << "0x" << instPC + (int)B_imm << "\n";
            if (reg[rs1] >= reg[rs2])
            {
                pc = instPC + (int)B_imm;
                // pc = pc & 0x00001fff; // debugging: review
            }
            break;
        }
        default:
            cout << "\tUnknown B Instruction\n";
        }
    }

    else if (opcode == 0x37)
    {
        // 35.LUI
        cout << "\tLUI\t" << name[rd] << ", " << hex << "0x" << ((int)U_imm >> 12) << "\n";
        reg[rd] = (int)U_imm; // debugging: review
    }

    else if (opcode == 0x17)
    {
        // 26.AUIPC
        cout << "\tAUIPC\t" << name[rd] << ", 0x" << hex << ((int)U_imm >> 12) << "\n";

        // debugging: In case of uncompressed test case "t4", use this if block to change "sp" to a lower value to be further than the data section,
        // becuase t4.bin's stack pointer overwrites the data section
        // if (instCount == 0 && ((filename == "t4.bin")))
        //     reg[rd] = 0x1000;
        // else
        reg[rd] = instPC + (int)U_imm;
    }

    else if (opcode == 0x6F)
    {
        // 37.JAL

        if (isCompressed == 0)
        {
            cout << "\tJAL\t" << name[rd] << ", 0x" << hex << instPC + (int)J_imm << "\n";
        }

        else if (isCompressed == 1)
        {

            if (rd == 0)
            {
                cout << "\tC.J\t"
                     << "0x" << hex << instPC + (int)J_imm << "\n";
            }

            else if (rd == 1)
            {

                cout << "\tC.JAL\t"
                     << "0x" << hex << instPC + (int)J_imm << "\n";
            }
        }

        if (isCompressed == 0)
        {
            reg[rd] = instPC + 4;
        }

        else if (isCompressed == 1)
        {
            reg[rd] = instPC + 2;
        }

        pc = instPC + (int)J_imm;
        // pc = pc & 0b00000000000111111111111111111111; // debugging: review
    }
    else if (opcode == 0x67)
    {
        switch (funct3)
        {
        case 0x0:
            // 38.JALR

            if (isCompressed == 0)
            {
                cout << "\tJALR\t" << name[rd] << ", " << name[rs1] << ", " << hex << "0x" << (int)I_imm << "\n";
            }
            else if (isCompressed == 1)
            {
                if (rd == 0b00000)
                {
                    cout << "\tC.JR\t" << name[rs1] << "\n";
                }
                else if (rd == 0b00001)
                {
                    cout << "\tC.JALR\t" << name[rs1] << "\n";
                }
            }

            if (isCompressed == 0)
            {
                reg[rd] = instPC + 4;
            }

            else if (isCompressed == 1)
            {
                reg[rd] = instPC + 2;
            }

            pc = reg[rs1] + (int)I_imm;
            // pc = pc & 0b00000000000000000000111111111111; // debugging: review

            break;

        default:
            cout << "\tUnkown I Instruction \n";
        }
    }
    else
    {
        cout << "\tUnkown Instruction Type \n";
    }
}

int main(int argc, char *argv[])
{
    unsigned int instWord = 0;
    ifstream inFile;
    ifstream dataFile;
    ofstream outFile;

    // debugging: use the lines below to hardcode which files to open
    // argc = 3;
    // argv[0] = "rvsim.exe";
    // argv[1] = "t4new.bin";
    // argv[2] = "t4-dnew.bin";
    // argv[1] = "t3.bin";
    // argv[2] = "t3-d.bin";

    if (argc < 1)
        emitError("use: rvcdiss <machine_code_file_name>\n");

    inFile.open(argv[1], ios::in | ios::binary | ios::ate);
    filename = argv[1];

    if (argc == 3)
        dataFile.open(argv[2], ios::in | ios::binary | ios::ate); // data section

    if (inFile.is_open())
    {
        int fsize = inFile.tellg();
        inFile.seekg(0, inFile.beg);
        if (!inFile.read((char *)memory, fsize)) // text file
            emitError("Cannot read from text file\n");
    }

    if (dataFile.is_open())
    {
        int fsize = dataFile.tellg();
        dataFile.seekg(0, dataFile.beg);
        if (!dataFile.read((char *)(memory + 0x00010000), fsize)) // data section
            emitError("Cannot read from data file\n");
    }

    if (inFile.is_open())
    {
        while (true)
        {
            reg[0] = 0; // zero is const
            instWord = (unsigned char)memory[pc] |
                       (((unsigned char)memory[pc + 1]) << 8) |
                       (((unsigned char)memory[pc + 2]) << 16) |
                       (((unsigned char)memory[pc + 3]) << 24);

            pc += 4;
            if ((instWord & 0x00000003) != 0x3) // if 16-bit instruction
            {
                pc -= 4;

                instWord = (unsigned char)memory[pc];
                instWord |= (((unsigned char)memory[pc + 1]) << 8);

                if (instWord == 0) // safety to prevent infinite loops
                    break;

                pc += 2;
                instWord = decompress(instWord);

                instDecExec(instWord, 1);
            }
            else
            {
                instDecExec(instWord, 0);
            }

            if (instWord == 0)
            {
                cout << "\nInstruction word = 0x0\nExit file\n";
                break;
            }

            // if (pc > 65536)
            // { // safety to prevent infinite loops
            //     cout << "\nEnd of text file\npc > 65536\n";
            //     break;
            // }
        }
    }
}
#include <stdio.h>
#include <stdlib.h>

using Byte = unsigned char;
using Word = unsigned short;
using u32 = unsigned int;

struct Mem
{
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialise()
    {
        for (u32 i = 0; i < MAX_MEM; i++)
        {
            Data[i] = 0;
        }
    }
    //read 1 byte
    Byte operator[](u32 Address) const
    {
        return Data[Address];
    }

    //write 1 byte
    Byte &operator[](u32 Address)
    {
        //assert here Address is <MAX_MEM
        return Data[Address];
    }

    //Write a word or 2 bytes
    void WriteWord(Word Value, u32 Address, u32 &Cycles)
    {
        Data[Address] = Value & 0xff;
        Data[Address + 1] = (Value >> 8);
        Cycles -= 2;
    }
};

struct CPU
{
    Word PC; //Program coutnter
    Word SP; // Stack Pointer

    Byte A, X, Y; // Accumulator,Index X,Index Y

    Byte C : 1; //Status Flag
    Byte Z : 1;
    Byte I : 1;
    Byte D : 1;
    Byte B : 1;
    Byte V : 1;
    Byte N : 1;

    /*Flag bits:
        Processor Status Register
        7  6  5  4  3  2  1  0
        N  V     B  D  I  Z  C

        C=Carry Flag
        Z=Zero Flag
        I=IRQ Disable Flag
        D=Decimal Mode Flag
        B=Break Command Flag
        V=OverFlow Flag
        N=Negative Flag*/

    void Reset(Mem &memory)
    {
        PC = 0xFFFC;
        SP = 0x0100;
        D = A = X = Y = C = Z = I = 0;
        memory.Initialise();
    }

    Byte FetchByte(u32 &Cycles, Mem &memory)
    {
        Byte Data = memory[PC];
        Cycles--;
        PC++;
        return Data;
    }

    Word FetchWord(u32 &Cycles, Mem &memory)
    {
        //6502 is little endian
        //first byte is the least significant byte
        Word Data = memory[PC];
        PC++;
        Cycles--;

        //second byte
        Data |= (memory[PC] << 8);
        PC++;
        Cycles--;

        //If You want to handle endianess you have to swap bytes here
        //if(PLATFORM_BIG_ENDIAN)
        //SwapBytesInWord(Data)

        return Data;
    }

    Byte ReadByte(u32 &Cycles, Byte Address, Mem &memory)
    {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }

    //opcodes
    static constexpr Byte INS_LDA_IM = 0xA9;  //Instruction Load Accumulator (Immediate)
    static constexpr Byte INS_LDA_ZP = 0xA5;  //Instruction Load Accumulator (Zero Page)
    static constexpr Byte INS_LDA_ZPX = 0xB5; //Instruction Load Accumulator (Zero Page X)
    static constexpr Byte INS_JSR = 0x20;     //Instruction Jump_To_Subroutine

    void LDASetStatus()
    {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    void Execute(u32 Cycles, Mem &memory)
    {
        while (Cycles > 0)
        {
            Byte Ins = FetchByte(Cycles, memory); //Instruction
            switch (Ins)
            {
            case INS_LDA_IM:
            {
                Byte Value = FetchByte(Cycles, memory);
                A = Value;
                LDASetStatus();
            }
            break;

            case INS_LDA_ZP:
            {
                Byte ZeroPageAddress = FetchByte(Cycles, memory);
                A = ReadByte(Cycles, ZeroPageAddress, memory);
                LDASetStatus();
            }
            break;

            case INS_LDA_ZPX:
            {
                Byte ZeroPageAddress = FetchByte(Cycles, memory);
                ZeroPageAddress += X;
                Cycles--;
                A = ReadByte(Cycles, ZeroPageAddress, memory);
                LDASetStatus();
            }
            break;

            case INS_JSR:
            {
                Word SubAdr = FetchWord(Cycles, memory);
                memory.WriteWord(PC - 1, SP, Cycles);
                PC = SubAdr;
                Cycles--;
            }
            break;

            default:
            {
                printf("Instruction not handled %d", Ins);
            }
            break;
            }
        }
    }
};

int main()
{
    Mem mem;
    CPU cpu;

    cpu.Reset(mem);
    //start-inline a little program (Immediate Load Accumulator 2 cycles)
    mem[0xFFFC] = CPU::INS_LDA_IM;
    mem[0xFFFD] = 0x42;
    //end-inline a little program
    cpu.Execute(2, mem);

    cpu.Reset(mem);
    //start-inline a little program (Zero Page Load Accumulator 3 cycles)
    mem[0xFFFC] = CPU::INS_LDA_ZP;
    mem[0xFFFD] = 0x42;
    mem[0x0042] = 0x84;
    //end-inline a little program
    cpu.Execute(3, mem);

    cpu.Reset(mem);
    //start-inline a little program (Jump to Subroutine)
    mem[0xFFFC] = CPU::INS_JSR;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0x42;
    mem[0x4242] = CPU::INS_LDA_IM;
    mem[0x4243] = 0x84;
    //end-inline a little program
    cpu.Execute(9, mem);

    return 0;
}
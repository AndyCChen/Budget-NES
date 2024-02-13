#include <stdio.h>

#include "../includes/cpu.h"
#include "../includes/cpu_ram.h"
#include "../includes/log.h"
#include "../includes/bus.h"
#include "../includes/util.h"

static CPU_6502 cpu;

static uint8_t cpu_fetch();
static void cpu_decode(uint8_t opcode);
static uint8_t cpu_execute(uint8_t opcode);
static void set_instruction_operand(address_modes_t address_mode, uint8_t opcode, uint8_t *extra_cycle);

static void stack_push(uint8_t value);
static uint8_t stack_pop(void);

// current decoded opcode to be executed
static opcode_t* decoded_opcode = NULL;

/**
 * Stores the operand of the instrucion depending on it's adressing mode.
 * Is unused by certain addressing modes such as implied and accumulator modes
 * because the operations are well... implied.
*/
static uint16_t instruction_operand;

// ------------------------------- instruction functions ----------------------------------------------------

/**
 * most instruction functions will return 0 but branch instructions 
 * for example return 1 to represent 1 extra cycle when a branch is taken.
 * Function signature commented with * are undocumented opcodes, most games
 * do not make use of these instructions.
*/

// load instructions

static uint8_t LAS(void); // *
static uint8_t LAX(void); // *
static uint8_t LDA(void);
static uint8_t LDX(void);
static uint8_t LDY(void);
static uint8_t SAX(void); // *
static uint8_t SHA(void); // *
static uint8_t SHX(void); // *
static uint8_t SHY(void); // *
static uint8_t STA(void);
static uint8_t STX(void);
static uint8_t STY(void);

// transfer instructions

static uint8_t SHS(void); // *
static uint8_t TAX(void);
static uint8_t TAY(void);
static uint8_t TSX(void);
static uint8_t TXA(void);
static uint8_t TXS(void);
static uint8_t TYA(void);

// stack instructions

static uint8_t PHA(void);
static uint8_t PHP(void);  
static uint8_t PLA(void);
static uint8_t PLP(void);

// shift instructions

static uint8_t ASL(void);
static uint8_t LSR(void);
static uint8_t ROL(void);
static uint8_t ROR(void);

// logic instructions

static uint8_t AND(void);
static uint8_t BIT(void);
static uint8_t EOR(void);
static uint8_t ORA(void);

// arithmetic instructions

static uint8_t ADC(void);
static uint8_t ANC(void);
static uint8_t ARR(void);
static uint8_t ASR(void);
static uint8_t CMP(void);
static uint8_t CPX(void);
static uint8_t CPY(void);
static uint8_t DCP(void);
static uint8_t ISC(void);
static uint8_t RLA(void);
static uint8_t RRA(void);
static uint8_t SBC(void);
static uint8_t SBX(void);
static uint8_t SLO(void);
static uint8_t SRE(void);
static uint8_t XAA(void);

// increment instructions

static uint8_t DEC(void);
static uint8_t DEX(void);
static uint8_t DEY(void);
static uint8_t INC(void);
static uint8_t INX(void);
static uint8_t INY(void);

// control

static uint8_t BRK(void);
static uint8_t JMP(void);
static uint8_t JSR(void);
static uint8_t RTI(void);
static uint8_t RTS(void);

// branch instructions

static uint8_t BCC(void);
static uint8_t BCS(void);
static uint8_t BEQ(void);
static uint8_t BMI(void);
static uint8_t BNE(void);
static uint8_t BPL(void);
static uint8_t BVC(void);
static uint8_t BVS(void);

// flags instructions

static uint8_t CLC(void);
static uint8_t CLD(void);
static uint8_t CLI(void);
static uint8_t CLV(void);
static uint8_t SEC(void);
static uint8_t SED(void);
static uint8_t SEI(void);

// kil

static uint8_t JAM(void){return 0;}

// nop instructions

static uint8_t NOP(void){return 0;}

// temp function to handle illegal opcodes for now
static uint8_t TMP(void)
{
   return 0;
} 

// addressing modes
// https://www.pagetable.com/c64ref/6502/
// a16, Y   Y indexed absolute            YAB
// a16, X   X indexed absolute            XAB
// (a8, X)  X index zero page indirect    XZI
// (a8), Y  Zero Page indirect Y indexed  YZI
// a8, X    X indexed zero page           XZP
// a8, Y    Y indexed zero page           YZP 
// a8       zero page                     ZPG
// #d8      immediate                     IMM
// a16      absolute                      ABS
// (a16)    absolute indirect             ABI
// r8       relative                      REL

static opcode_t opcode_lookup_table[256] = 
{
   /* 0x00 - 0x 0F */
   {"BRK", &BRK, IMP, 7}, {"ORA", &ORA, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"ORA", &ORA, ZPG, 3},
   {"ASL", &ASL, ZPG, 5}, {"???", &TMP, IMP, 0}, {"PHP", &PHP, IMP, 3},
   {"ORA", &ORA, IMM, 2}, {"ASL", &ASL, ACC, 2}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"ORA", &ORA, ABS, 4}, {"ASL", &ASL, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0x10 - 0x1F */
   {"BPL", &BPL, REL, 2}, {"ORA", &ORA, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"ORA", &ORA, XZP, 3},
   {"ASL", &ASL, XZP, 6}, {"???", &TMP, IMP, 0}, {"CLC", &CLC, IMP, 2},
   {"ORA", &ORA, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"ORA", &ORA, XAB, 4}, {"ASL", &ASL, XAB, 7},
   {"???", &TMP, IMP, 0},

   /* 0x20 - 0x2F */
   {"JSR", &JSR, ABS, 6}, {"AND", &AND, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"BIT", &BIT, ZPG, 3}, {"AND", &AND, ZPG, 3},
   {"ROL", &ROL, ZPG, 5}, {"???", &TMP, IMP, 0}, {"PLP", &PLP, IMP, 4},
   {"AND", &AND, IMM, 2}, {"ROL", &ROL, ACC, 2}, {"???", &TMP, IMP, 0},
   {"BIT", &BIT, ABS, 4}, {"AND", &AND, ABS, 4}, {"ROL", &ROL, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0x30 - 0x3F */
   {"BMI", &BMI, REL, 2}, {"AND", &AND, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"AND", &AND, XZP, 4},
   {"ROL", &ROL, XZI, 6}, {"???", &TMP, IMP, 0}, {"SEC", &SEC, IMP, 2},
   {"AND", &AND, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"AND", &AND, XAB, 4}, {"ROL", &ROL, XAB, 7},
   {"???", &TMP, IMP, 0},

   /* 0x40 - 0x4F */
   {"RTI", &RTI, IMP, 6}, {"EOR", &EOR, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"EOR", &EOR, ZPG, 3},
   {"LSR", &LSR, ZPG, 5}, {"???", &TMP, IMP, 0}, {"PHA", &PHA, IMP, 3}, 
   {"EOR", &EOR, IMM, 2}, {"LSR", &LSR, ACC, 2}, {"???", &TMP, IMP, 0}, 
   {"JMP", &JMP, ABS, 3}, {"EOR", &EOR, ABS, 4}, {"LSR", &LSR, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0x50 - 0x5F */
   {"BVC", &BVC, REL, 2}, {"EOR", &EOR, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"EOR", &EOR, XZP, 4},
   {"LSR", &LSR, XZP, 6}, {"???", &TMP, IMP, 0}, {"CLI", &CLI, IMP, 2},
   {"EOR", &EOR, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"EOR", &EOR, XAB, 4}, {"LSR", &LSR, XAB, 7},
   {"???", &TMP, IMP, 0},

   /* 0x60 - 0x6F */
   {"RTS", &RTS, IMP, 6}, {"ADC", &ADC, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"ADC", &ADC, ZPG, 3},
   {"ROR", &ROR, ZPG, 5}, {"???", &TMP, IMP, 0}, {"PLA", &PLA, IMP, 4},
   {"ADC", &ADC, IMM, 2}, {"ROR", &ROR, ACC, 2}, {"???", &TMP, IMP, 0},
   {"JMP", &JMP, ABI, 5}, {"ADC", &ADC, ABS, 4}, {"ROR", &ROR, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0x70 - 7F */
   {"BVS", &BVS, REL, 2}, {"ADC", &ADC, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"ADC", &ADC, XZP, 4},
   {"ROR", &ROR, XZP, 6}, {"???", &TMP, IMP, 0}, {"SEI", &SEI, IMP, 2},
   {"ADC", &ADC, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"ADC", &ADC, XAB, 4}, {"ROR", &ROR, XAB, 7},
   {"???", &TMP, IMP, 0},

   /* 0x80 - 0x8F */
   {"???", &TMP, IMP, 0}, {"STA", &STA, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"STY", &STY, ZPG, 3}, {"STA", &STA, ZPG, 3},
   {"STX", &STX, ZPG, 3}, {"???", &TMP, IMP, 0}, {"DEY", &DEY, IMP, 2},
   {"???", &TMP, IMP, 0}, {"TXA", &TXA, IMP, 2}, {"???", &TMP, IMP, 0},
   {"STY", &STY, ABS, 4}, {"STA", &STA, ABS, 4}, {"STX", &STX, ABS, 4},
   {"???", &TMP, IMP, 0},

   /* 0x90 - 0x9F */
   {"BCC", &BCC, REL, 2}, {"STA", &STA, YZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"STY", &STY, XZP, 4}, {"STA", &STA, XZP, 4},
   {"STX", &STX, XZP, 4}, {"???", &TMP, IMP, 0}, {"TYA", &TYA, IMP, 2},
   {"STA", &STA, YAB, 5}, {"TXS", &TXS, IMP, 2}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"STA", &STA, XAB, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0},

   /* 0xA0 - 0xAF */
   {"LDY", &LDY, IMM, 2}, {"LDA", &LDA, XZI, 6}, {"LDX", &LDX, IMM, 2},
   {"???", &TMP, IMP, 0}, {"LDY", &LDY, ZPG, 3}, {"LDA", &LDA, ZPG, 3},
   {"LDX", &LDX, ZPG, 3}, {"???", &TMP, IMP, 0}, {"TAY", &TAY, IMP, 2},
   {"LDA", &LDA, IMM, 2}, {"TAX", &TAX, IMP, 2}, {"???", &TMP, IMP, 0},
   {"LDY", &LDY, ABS, 4}, {"LDA", &LDA, ABS, 4}, {"LDX", &LDX, ABS, 4},
   {"???", &TMP, IMP, 0},

   /* 0xB0 - 0xBF */
   {"BCS", &BCS, REL, 2}, {"LDA", &LDA, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"LDY", &LDY, XZP, 4}, {"LDA", &LDA, XZP, 4},
   {"LDX", &LDX, YZP, 4}, {"???", &TMP, IMP, 0}, {"CLV", &CLV, IMP, 2},
   {"LDA", &LDA, YAB, 4}, {"TSX", &TSX, IMP, 2}, {"???", &TMP, IMP, 0},
   {"LDY", &LDY, XAB, 4}, {"LDA", &LDA, XAB, 4}, {"LDX", &LDX, YAB, 4},
   {"???", &TMP, IMP, 0},

   /* 0xC0 - 0xCF */
   {"CPY", &CPY, IMM, 2}, {"CMP", &CMP, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"CPY", &CPY, ZPG, 3}, {"CMP", &CMP, ZPG, 3},
   {"DEC", &DEC, ZPG, 5}, {"???", &TMP, IMP, 0}, {"INY", &INY, IMP, 2},
   {"CMP", &CMP, IMM, 2}, {"DEX", &DEX, IMP, 2}, {"???", &TMP, IMP, 0},
   {"CPY", &CPY, ABS, 4}, {"CMP", &CMP, ABS, 4}, {"DEC", &DEC, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0xD0 - 0xDF */
   {"BNE", &BNE, REL, 2}, {"CMP", &CMP, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"CMP", &CMP, XZP, 4},
   {"DEC", &DEC, XZP, 6}, {"???", &TMP, IMP, 0}, {"CLD", &CLD, IMP, 2},
   {"CMP", &CMP, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"CMP", &CMP, XAB, 4}, {"DEC", &DEC, XAB, 7},
   {"???", &TMP, IMP, 0},

   /* 0xE0 - 0xEF */
   {"CPX", &CPX, IMM, 2}, {"SBC", &SBC, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"CPX", &CPX, ZPG, 3}, {"SBC", &SBC, ZPG, 3},
   {"INC", &INC, ZPG, 5}, {"???", &TMP, IMP, 0}, {"INX", &INX, IMP, 2},
   {"SBC", &SBC, IMM, 2}, {"NOP", &NOP, IMP, 2}, {"???", &TMP, IMP, 0},
   {"CPX", &CPX, ABS, 4}, {"SBC", &SBC, ABS, 4}, {"INC", &INC, ABS, 6},
   {"???", &TMP, IMP, 0},

   /* 0xF0 - 0xFF */
   {"BEQ", &BEQ, REL, 2}, {"SBC", &SBC, YZI, 5}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"SBC", &SBC, XZP, 4},
   {"INC", &INC, XZP, 6}, {"???", &TMP, IMP, 0}, {"SED", &SED, IMP, 2},
   {"SBC", &SBC, YAB, 4}, {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"SBC", &SBC, XAB, 4}, {"INC", &INC, XAB, 7},
   {"???", &TMP, IMP, 0}
};

/**
 * Read more on different addressing modes here -> https://www.pagetable.com/c64ref/6502/?tab=3
 * Forms the instruction operand depending on the provided address mode.
 * @param address_mode the address mode of the current opcode
 * @param opcode opcode to execute is passed in for logging purposes
 * @param extra_cycles set to 1 if page is crossed to represent 1 extra cycle,
 * else is set to zero
*/
static void set_instruction_operand(address_modes_t address_mode, uint8_t opcode, uint8_t *extra_cycle)
{
   switch (address_mode)
   {
      case IMP:
      {
         *extra_cycle = 0;

         nestest_log("%02X %6s%4s %28s", opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case ACC:
      { 
         *extra_cycle = 0;

         nestest_log("%02X %6s%4s A %26s", opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case IMM:
      {
         *extra_cycle = 0;

         instruction_operand = cpu_fetch();

         nestest_log("%02X %02X %3s%4s #$%02X %23s", opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case ABS:
      {
         *extra_cycle = 0;

         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         instruction_operand = ( hi << 8 ) | lo;

         nestest_log("%02X %02X %2X %4s $%04X %22s", opcode, lo, hi, decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case XAB:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         instruction_operand = abs_address + cpu.X;

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken. 
         */
         *extra_cycle = ( (instruction_operand & 0xFF00) != hi << 8 ) ? 1 : 0;

         nestest_log("%02X %02X %02X %4s $%04X, X @ %04X = %02X %8s", opcode, lo, hi, decoded_opcode->mnemonic, abs_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case YAB:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         instruction_operand = abs_address + cpu.Y;

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken. 
         */
         *extra_cycle = ( (instruction_operand & 0xFF00) != hi << 8 ) ? 1 : 0;

         nestest_log("%02X %02X %02X %4s $%04X, Y @ %04X = %02X %8s", opcode, lo, hi, decoded_opcode->mnemonic, abs_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case ABI:
      {
         *extra_cycle = 0;

         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         instruction_operand = bus_read_u16(abs_address);

         nestest_log("%02X %02X %2X %4s ($%04X) = %04X %13s", opcode, lo, hi, decoded_opcode->mnemonic, abs_address, instruction_operand, "");
         break;
      }
      case ZPG:
      {
         *extra_cycle = 0;

         instruction_operand = cpu_fetch();

         nestest_log("%02X %02X %3s%4s $%02X = %02X %19s", opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case XZP:
      {
         *extra_cycle = 0;

         uint8_t zpg_address = cpu_fetch();

         instruction_operand =  ( zpg_address + cpu.X ) & 0x00FF;

         nestest_log("%02X %02X %3s%4s %02X, X @ %02X = %02X %12s", opcode, zpg_address, "", decoded_opcode->mnemonic, zpg_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case YZP:
      {
         *extra_cycle = 0;

         uint8_t zpg_address = cpu_fetch();

         instruction_operand = ( zpg_address + cpu.Y ) & 0x00FF;

         nestest_log("%02X %02X %3s%4s %02X, Y @ %02X = %02X %12s", opcode, zpg_address, "", decoded_opcode->mnemonic, zpg_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case XZI:
      {
         *extra_cycle = 0;
         
         uint8_t zpg_base_address  = cpu_fetch();
         uint8_t zpg_address = ( zpg_base_address + cpu.X ) & 0x00FF; // add X index offsest to base address to form zpg address 

         instruction_operand = bus_read_u16(zpg_address);

         nestest_log("%02X %02X %3s%4s ($%02X),X @ %02X = %04X = %02X %3s", opcode, zpg_base_address, "", decoded_opcode->mnemonic, zpg_base_address, zpg_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case YZI:
      {
         uint8_t zpg_address = cpu_fetch();
         uint16_t base_address = bus_read_u16(zpg_address);

         instruction_operand = base_address + cpu.Y;

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken. 
         */
         *extra_cycle = ( (instruction_operand & 0xFF00) != (base_address & 0xFF00) ) ? 1 : 0;

         nestest_log("%02X %02X %3s%4s ($%02X),Y = %04X @ %04X = %02X  ", opcode, zpg_address, "", decoded_opcode->mnemonic, zpg_address, base_address, instruction_operand, bus_read(instruction_operand));
         break;
      }
      case REL: // todo need page cross
      {
         uint8_t offset_byte = cpu_fetch();
         
         /**
          * Offset_byte is in 2's complement so we check bit 7 to see if it is negative.
         */
         if (offset_byte & 0x80)
         {
            /**
             * Since the signed offset_byte is 8 bits and we are adding into a 16 bit value,
             * we need to treat the offset as a 16 bit value. The offset byte is negative value
             * in 2's complement, so the added extra 8 bits are all set to 1, hence the 0xFF00 bitmask.
            */
            instruction_operand = (cpu.pc) + ( offset_byte | 0xFF00 );
         }
         else
         {
            instruction_operand = (cpu.pc) + offset_byte;
         }

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken.
         */
         *extra_cycle = ( (instruction_operand & 0xFF00) != (cpu.pc & 0xFF00) ) ? 1 : 0;

         nestest_log("%02X %02X %3s%4s $%04X %22s", opcode, offset_byte, "", decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
   }

   nestest_log("A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu.ac, cpu.X, cpu.Y, cpu.status_flags, cpu.sp);
}

/**
 * Wrapper function around bus_read() that fetches a byte at current location of program counter address.
 * Increments program counter after fetching.
 * @returns the fetched byte
*/
static uint8_t cpu_fetch()
{
   // fetch
   uint8_t fetched_byte = bus_read(cpu.pc);
   ++cpu.pc;

   return fetched_byte;
}

/**
 * Decodes an opcode by using the opcode to index into the opcode lookup table
 * The decoded opcode will be executed on the next call to instruction_execute.
 * Call this function before calling cpu_execute.
 * @param opcode the opcode to decode
*/
static void cpu_decode(uint8_t opcode)
{
   decoded_opcode = &opcode_lookup_table[opcode];
}

/**
 * execute the instruction that was previously decoded from a call to cpu_decode().
 * Only call this function after cpu_decode() has been called.
 * @param opcode the opcode to execute is passed for logging purposes
 * @returns the number of cycles the executed instruction takes
*/
static uint8_t cpu_execute(uint8_t opcode)
{
   uint8_t extra_cycles = 0;
   set_instruction_operand(decoded_opcode->mode, opcode, &extra_cycles);

   extra_cycles += decoded_opcode->opcode_function(); // execute the current instruction

   return decoded_opcode->cycles + extra_cycles;
}

// load instructions

static uint8_t LAS(void){return 0;}
static uint8_t LAX(void){return 0;}

/**
 * load accumulator with value from memory
 * sets zero flag if accumulator is set to zero, otherwise zero flag is reset
 * sets negative flag if bit 7  of accumulator is 1, otherwises negative flag is reset
*/
static uint8_t LDA(void)
{
   /**
    * Load operand directly into accumulator when in immediate mode.
    * Else treat operand as a effective memory address.
   */ 
   if (decoded_opcode->mode == IMM)
   {
      cpu.ac = instruction_operand;
   }
   else
   {
      cpu.ac = bus_read(instruction_operand);
   }

   // set zero flag
   if (cpu.ac == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   // set negative flag
   if (cpu.ac & 0x80)
   {
      set_bit(cpu.status_flags,7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   return 0;
}

/**
 * Load X register with value from memory
 * Set zero bit if loaded value is zero.
 * Set negative bit if loaded value has bit 7 set to 1.
*/
static uint8_t LDX(void)
{
   /**
    * Load operand directly into X register when in immediate mode.
    * Else treat operand as a effective memory address.
   */ 
   if (decoded_opcode->mode == IMM)
   {
      cpu.X = instruction_operand;
   }
   else 
   {
      cpu.X = bus_read(instruction_operand);
   }
   
   // set zero flag
   if (cpu.X == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   // set negative flag
   if (cpu.X & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   return 0;
}

/**
 * Load value from memory into Y register.
 * Set negative flag if bit 7 of loaded value is 1, else reset.
 * Set zero flag if loaded value is 0, else reset.
*/
static uint8_t LDY(void)
{
   // use operand directly when in immediate mode
   if ( decoded_opcode->mode == IMM )
   {
      cpu.Y = instruction_operand;
   }
   else // else use operand as effective memory address
   {
      cpu.Y = bus_read(instruction_operand);
   }

   // set/reset negative flag
   if ( cpu.Y & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if ( cpu.Y == 0 )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

static uint8_t SAX(void){return 0;}
static uint8_t SHA(void){return 0;}
static uint8_t SHX(void){return 0;}
static uint8_t SHY(void){return 0;}

/**
 * transfer accumulator value into memory
*/
static uint8_t STA(void)
{
   bus_write(instruction_operand, cpu.ac);
   return 0;
}

/**
 * transfers X register value into memory location
*/
static uint8_t STX(void)
{
   bus_write(instruction_operand, cpu.X);
   return 0;
}

/**
 * transfers Y register value into memory location
*/
static uint8_t STY(void)
{
   bus_write(instruction_operand, cpu.Y);
   return 0;
}

// transfer instructions

static uint8_t SHS(void){return 0;}

/**
 * Transfer accumulator to X register.
 * Negative flag is set if bit 7 in loaded value in X register is 1, else reset.
 * Zero flag is set if loaded value in X register is zero, else reset.
*/
static uint8_t TAX(void)
{
   cpu.X = cpu.ac;

   // set/reset negative flag
   if (cpu.X & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.X == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Transfer accumulator to Y register.
 * Negative flag is set if bit 7 in loaded value in Y register is 1, else reset.
 * Zero flag is set if loaded value in Y register is zero, else reset.
*/
static uint8_t TAY(void)
{
   cpu.Y = cpu.ac;

   // set/reset negative flag
   if (cpu.Y & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.Y == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Transfer stack pointer to X register.
 * Negative flag is set if bit 7 in loaded value in X register is 1, else reset.
 * Zero flag is set if loaded value in X register is zero, else reset.
*/
static uint8_t TSX(void)
{
   cpu.X = cpu.sp;

   // set/reset negative flag
   if (cpu.X & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.X == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }
   return 0;
}

/**
 * Transfer X register to accumulator.
 * Negative flag is set if bit 7 in loaded value in accumulator is 1, else reset.
 * Zero flag is set if loaded value in accumulator is zero, else reset.
*/
static uint8_t TXA(void)
{
   cpu.ac = cpu.X;

   // set/reset negative flag
   if (cpu.ac & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.ac == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Transfer X register to stack pointer.
*/
static uint8_t TXS(void)
{
   cpu.sp = cpu.X;
   return 0;
}

/**
 * Transfer Y register to accumulator.
 * Negative flag is set if bit 7 in loaded value in accumulator is 1, else reset.
 * Zero flag is set if loaded value in accumulator is zero, else reset.
*/
static uint8_t TYA(void)
{
   cpu.ac = cpu.Y;

   // set/reset negative flag
   if (cpu.ac & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.ac == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

// stack instructions

/**
 * Push accumulator onto the stack.
*/
static uint8_t PHA(void)
{
   stack_push(cpu.ac);
   return 0;
}

/**
 * Pushes the status_flags register onto the stack.
 * Software instructions PHP and BRK push the status flag with 
 * bit 4 (break flag) set as 1.
 * Hardware interupts push the break flag set as 0.
*/
static uint8_t PHP(void)
{
   stack_push(cpu.status_flags | 0x10);
   return 0;
}

/**
 * Pop a value from the stack and loads it into the accumulator.
 * Sets negative flag if bit 7 of accumulator is 1, else reset flag
 * Sets zero flag if if accumulator is 0, else reset
*/
static uint8_t PLA(void)
{
   cpu.ac = stack_pop();

   // set or clear negative flag
   if ( cpu.ac & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set or clear zero flag
   if ( cpu.ac == 0 )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Pop status flag register from stack.
 * Break flag is reset as this flag only exists
 * when the status register is pushed onto the stack.
 * Whet the status register is poped, the break flag is
 * "discarded" by being reset to 0.
 * The 5th unused bit is alway set to 1.
*/
static uint8_t PLP(void)
{
   cpu.status_flags = stack_pop();
   clear_bit(cpu.status_flags, 4);
   set_bit(cpu.status_flags, 5);

   return 0;
}

// shift instructions

/**
 * Shift either the accumulator or value in memory 1 bit to the left.
 * Bit 0 after shifting is always set to 0, while bit 7 prior to the shift is stored
 * in the carry flag.
 * Set negative flag if bit 7 after the shift is set, else reset.
 * Set zero flag if the result after shifting is 0, else reset.
*/
static uint8_t ASL(void)
{
   uint8_t shifted_value;

   if (decoded_opcode->mode == ACC)
   {
      cpu.status_flags |= cpu.ac >> 7; // move bit 7 into carry flag
      shifted_value = cpu.ac << 1;

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = bus_read(instruction_operand);

      cpu.status_flags |= value_to_shift >> 7; // move bit 7 into carry flag
      shifted_value = value_to_shift << 1;

      bus_write(instruction_operand, shifted_value);
   }

   // set/reset negative flag
   if (shifted_value & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (shifted_value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Shift either the accumulator or value in memory 1 bit to the right.
 * Bit 7 that is shifted in is always zero, while bit 0 which is shifted out 
 * is stored in the carry flag.
 * Negative flag is alway reset to 0.
 * Set zero flag if result of the shit is 0, else reset.
*/
static uint8_t LSR(void)
{
   uint8_t shifted_value;

   if (decoded_opcode->mode == ACC)
   {
      cpu.status_flags |= cpu.ac & 0x01; // store bit 0 in carry flag
      shifted_value = cpu.ac >> 1;

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = bus_read(instruction_operand);

      cpu.status_flags |= value_to_shift & 0x01; // store bit 0 in carry flag
      shifted_value = value_to_shift >> 1;

      bus_write(instruction_operand, shifted_value);
   }

   // reset negative flag
   clear_bit(cpu.status_flags, 7);

   // set/reset zero flag
   if (shifted_value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Rotate either the accumulator or value in memory left 1 bit.
 * The left most bit that is shifted out is stored in the carry flag
 * and in bit 0.
 * Set negative flag to bit 7 of the shifted result.
 * Set zero flag is shifted value is zero, else reset.
*/
static uint8_t ROL(void)
{
   uint8_t shifted_value, carry_bit;

   if (decoded_opcode->mode == ACC)
   {
      carry_bit = cpu.ac & 0x80;   // store left most bit prior to shift
      shifted_value = cpu.ac << 1; // left shift 1 bit
      shifted_value |= carry_bit;  // store carry_bit into bit 0 of shifted value

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = bus_read(instruction_operand);

      carry_bit = value_to_shift & 0x80;   // store left most bit prior to shift
      shifted_value = value_to_shift << 1; // left shift 1 bit
      shifted_value |= carry_bit;          // store carry_bit into bit 0 of shifted value

      bus_write(instruction_operand, shifted_value);
   }

   cpu.status_flags |= carry_bit; // store carry bit into carry flag

   // set/reset negative flag
   if (shifted_value & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (shifted_value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Rotate either the accumulator or value in memory right 1 bit.
 * The right most bit that is shifted out is stored in the carry flag
 * and in bit 7.
 * Set negative flag to bit 7 of the shifted result.
 * Set zero flag is shifted value is zero, else reset.
*/
static uint8_t ROR(void)
{
   uint8_t shifted_value, carry_bit;

   if (decoded_opcode->mode == ACC)
   {
      carry_bit = cpu.ac & 0x01;        // store right most bit prior to shift
      shifted_value = cpu.ac >> 1;      // right shift 1 bit
      shifted_value |= carry_bit << 7;  // store carry_bit into bit 7 of shifted value

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = bus_read(instruction_operand);

      carry_bit = value_to_shift & 0x01;   // store right most bit prior to shift
      shifted_value = value_to_shift >> 1; // right shift 1 bit
      shifted_value |= carry_bit << 7;     // store carry_bit into bit 7 of shifted value

      bus_write(instruction_operand, shifted_value);
   }

   cpu.status_flags |= carry_bit; // store carry bit into carry flag

   // set/reset negative flag
   if (shifted_value & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (shifted_value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

// logic instructions

/**
 * Performs bitwise AND between accumulator and value in memory,
 * result is store back into the accumulator.
 * Sets the zero flag if result of accumulator is zero, else reset.
 * Sets the negative flag if bit 7 of result of accumulator is set, else reset.
*/
static uint8_t AND(void)
{
   // use operand directly in immediate mode
   if (decoded_opcode->mode == IMM)
   {
      cpu.ac = cpu.ac & instruction_operand;
   }
   // else treat operand as a effective memory address
   else
   {
      cpu.ac = cpu.ac & bus_read(instruction_operand);
   }

   // set or clear negative flag
   if ( cpu.ac & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set or clear zero flag
   if ( cpu.ac == 0 )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Performs bitwise AND between value in memory and value of accumulator.
 * The result is used to set or clear status flags but is not stored.
 * Bit 7 of the addressed memory is transfered into the negative flag.
 * Bit 6 of the addressed memory is transfered into the overflow flag.
 * zero flag is set if result of bitwise AND between accumulator and memory is zero
*/
static uint8_t BIT(void)
{
   uint8_t value = bus_read(instruction_operand);

   // clear bits before transfer
   clear_bit(cpu.status_flags, 7);
   clear_bit(cpu.status_flags, 6);

   cpu.status_flags |= value & ( 1 << 7 ); // transfer 7th bit into negative flag
   cpu.status_flags |= value & ( 1 << 6 ); // transfer 6th bit into overflow flag

   if ( (cpu.ac & value) == 0 )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Perform bitwise XOR with accumulator and value in memory,
 * store the result back into the accumulator.
 * Set negative flag if bit 7 is set in result, else reset.
 * Set zero flag if result value is zero, else reset.
*/
static uint8_t EOR(void)
{
   uint8_t operand;

   if (decoded_opcode->mode == IMM)
   {
      operand = instruction_operand;
   }
   else
   {
      operand = bus_read(instruction_operand);
   }

   cpu.ac = cpu.ac ^ operand;

   // set/reset negative flag
   if (cpu.ac & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.ac == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Perform bitwise OR with accumulator and value in memory storing
 * the result in the accumulator.
 * Set negative flag if bit 7 in result is set, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t ORA(void)
{
   uint8_t value;

   if (decoded_opcode->mode == IMM)
   {
      value = instruction_operand;
   }
   else
   {
      value =  bus_read(instruction_operand);
   }

   cpu.ac = cpu.ac | value;

   // set/reset negative flag
   if (cpu.ac & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.ac == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

// arithmetic instructions

static uint8_t ADC(void){return 0;}
static uint8_t ANC(void){return 0;}
static uint8_t ARR(void){return 0;}
static uint8_t ASR(void){return 0;}

/**
 * Subtracts value in memory from value in accumulator, the result is not stored.
 * Zero flag is set if value in memory is equal to value in accumulator, else reset.
 * Negative flag is set if bit 7 in the final result of the subtraction is a 1, else reset.
 * Carry flag set if value in memory is less than or equal to value in accumulator, else reset when greater.
*/
static uint8_t CMP(void)
{
   uint8_t result, value;

   // use operand directly when in immediate mode
   if ( decoded_opcode->mode == IMM )
   {
      value = instruction_operand;
   }
   else // use operand as effective memory address
   {
      value = bus_read(instruction_operand);
   }

   result = cpu.ac - value;

   // set/reset zero flag
   if ( value == cpu.ac)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   // set/reset negative flag
   if ( result & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset carry flag
   if ( value <= cpu.ac )
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }

   return 0;
}

/**
 * Subtract value in memory from the X register,
 * but does not store the result.
 * Set carry flag when absolute value of X is >= than value in memory.
 * Set negative flag if result has bit 7 set, else reset.
 * Set zero flag if value in memory is equal to X, else reset.
*/
static uint8_t CPX(void)
{
   uint8_t value, result;

   if (decoded_opcode->mode == IMM)
   {
      value = instruction_operand;
   }
   else
   {
      value = bus_read(instruction_operand);
   }

   result = cpu.X - value;

   // set/reset carry flag
   if ( cpu.X >= value )
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }

   // set/reset negative flag
   if ( result & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if ( cpu.X == result )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

static uint8_t CPY(void){return 0;}
static uint8_t DCP(void){return 0;}
static uint8_t ISC(void){return 0;}
static uint8_t RLA(void){return 0;}
static uint8_t RRA(void){return 0;}
static uint8_t SBC(void){return 0;}
static uint8_t SBX(void){return 0;}
static uint8_t SLO(void){return 0;}
static uint8_t SRE(void){return 0;}
static uint8_t XAA(void){return 0;}

// increment instructions

/**
 * Subtract 1 from value in memory in 2's complement
 * and store result back into memory.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t DEC(void)
{
   uint8_t value = bus_read(instruction_operand);
   bus_write(instruction_operand, --value);

   // set/reset negative flag
   if (value & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Decrement value in X register by 1 and store result in X.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t DEX(void)
{
   --cpu.X;

   // set/reset negative flag
   if (cpu.X & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.X == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Decrement value in Y register by 1 and store result in Y.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t DEY(void)
{
   --cpu.Y;

   // set/reset negative flag
   if (cpu.Y & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.Y == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Increment value in memory by one and store result back into memory.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset. 
*/
static uint8_t INC(void)
{
   uint8_t value = bus_read(instruction_operand);
   bus_write(instruction_operand, ++value);

   // set/reset negative flag
   if (value & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (value == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Increment value in X register by 1 and store result in X.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t INX(void)
{
   ++cpu.X;

   // set/reset negative flag
   if (cpu.X & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.X == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

/**
 * Increment value in Y register by 1 and store result in Y.
 * Set negative flag if bit 7 of result is on, else reset.
 * Set zero flag if result is zero, else reset.
*/
static uint8_t INY(void)
{
   ++cpu.Y;

   // set/reset negative flag
   if (cpu.Y & 0x80)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
   if (cpu.Y == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   return 0;
}

// control

/**
 * Program counter of the second byte and the status flags are pushed
 * onto the stack with the bit 4 (break flag) set as 1.
 * The cpu then transfers control to the interrupt vecter at
 * address 0xFFFE.
*/
static uint8_t BRK(void)
{
   /**
    * next byte is fetch and pc increment with the result discarded, 
    * but we can just increment pc directly without calling cpu_fetch()
   */
   ++cpu.pc;

   stack_push( ( cpu.pc & 0xFF00 ) >> 8 );
   stack_push( cpu.pc & 0x00FF );

   set_bit(cpu.status_flags, 4); // break flag pushed as 1
   stack_push(cpu.status_flags);

   cpu.pc = bus_read_u16(INTERRUPT_VECTOR);

   return 0;
}

/**
 * loads program counter with new jump value
*/
static uint8_t JMP(void)
{
   cpu.pc = instruction_operand;
   return 0;
}

/**
 * Jumps to a subroutine.
 * Save return address onto the stack
*/
static uint8_t JSR(void)
{
   /**
    * When the third byte is fetched the program counter is not incremented by the 6502 cpu.
    * This is because the pc is going to be loaded with a new address to jump to a subroutine anyways
    * so the incrementation is skipped.
    * So we decrement to program counter to undo the increment from calling cpu_fetch() when setting
    * the instruction operand.
    * Yes this is weird.
   */
   --cpu.pc;

   uint8_t hi = ( cpu.pc & 0xFF00 ) >> 8;
   stack_push(hi);

   uint8_t lo = cpu.pc & 0x00FF;
   stack_push(lo);

   cpu.pc = instruction_operand;

   return 0;
}

/**
 * Returns from a interrupt.
 * Pops the status flag register and the program counter from the stack
 * and loads them back into respective cpu registers.
*/
static uint8_t RTI(void)
{
   cpu.status_flags = stack_pop();
   uint8_t lo = stack_pop();
   uint8_t hi = stack_pop();

   set_bit(cpu.status_flags, 5);   // unused bit 5 is always set
   clear_bit(cpu.status_flags, 4); // break flag is always reset when popped from stack

   cpu.pc = ( hi << 8) | lo;

   return 0;
}

/**
 * return from subroutine
*/
static uint8_t RTS(void)
{
   uint8_t lo = stack_pop();
   
   uint8_t hi = stack_pop();
   
   cpu.pc = ( hi << 8 ) | lo;
   ++cpu.pc;

   return 0;
}

// branch instructions

/**
 * branch if carry flag is cleared
*/
static uint8_t BCC(void)
{
   if ( !(cpu.status_flags & 0x01) )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if carry flag is set
*/ 
static uint8_t BCS(void)
{
   if ( cpu.status_flags & 0x01 )
   {
      cpu.pc = instruction_operand;

      return 1;
   }

   return 0;
}

/**
 * branch if zero flag is set
*/
static uint8_t BEQ(void)
{
   if ( cpu.status_flags & 0x02 )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if negative flag is set
*/
static uint8_t BMI(void)
{
   if ( cpu.status_flags & 0x80 )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if zero flag is not set
*/
static uint8_t BNE(void)
{
   if ( !(cpu.status_flags & 0x02) )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if negative flag is cleared
*/
static uint8_t BPL(void)
{
   if ( !(cpu.status_flags & 0x80) )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if overflow flag is cleared
*/
static uint8_t BVC(void)
{
   if ( !(cpu.status_flags & 0x40) )
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

/**
 * branch if overflow flag is set
*/
static uint8_t BVS(void)
{
   if ( cpu.status_flags & 0x40)
   {
      cpu.pc = instruction_operand;
      return 1;
   }

   return 0;
}

// flags instructions

/**
 * clear the carry flag
*/
static uint8_t CLC(void)
{
   clear_bit(cpu.status_flags, 0);
   return 0;
}

/**
 * clears the decimal mode flag
*/
static uint8_t CLD(void)
{
   clear_bit(cpu.status_flags, 3);
   return 0;
}

/**
 * Clears the interupt flag.
*/
static uint8_t CLI(void)
{
   clear_bit(cpu.status_flags, 2);
   return 0;
}

/**
 * Clears the overflow flag.
*/
static uint8_t CLV(void)
{
   clear_bit(cpu.status_flags, 6);
   return 0;
}

/**
 * sets the carry flag to 1
*/
static uint8_t SEC(void)
{
   set_bit(cpu.status_flags, 0);
   return 0;
}

/**
 * Sets the decimal flag to 1.
 * Causes subsequent ADC and SBC instructions to operate
 * in decimal arithmetic mode
*/
static uint8_t SED(void)
{
   set_bit(cpu.status_flags, 3);
   return 0;
}

/**
 * sets the interrupt disable flag to 1
*/
static uint8_t SEI(void)
{
   set_bit(cpu.status_flags, 2);
   return 0;
}

/**
 * Emulate the execution of one 6502 cpu instruction
 * and counts the number of cycles that the execution would have taken.
*/
void cpu_emulate_instruction()
{
   nestest_log("%04X  ", cpu.pc); // log current pc value before fetching

   uint8_t opcode = cpu_fetch();
   cpu_decode(opcode);
   uint8_t cycles = cpu_execute(opcode);

   // todo count cycles
}

/**
 * resets cpu register states
*/
void cpu_reset(void)
{
   cpu.ac = 0;
   cpu.X = 0;
   cpu.Y = 0;
   cpu.sp = 0xFD;
   cpu.status_flags = 0x24;
   cpu.pc = 0xC000;
}

/**
 * push a value onto the stack and decrement stack pointer
 * @param value the value to push onto the stack
 */ 
static void stack_push(uint8_t value)
{
   bus_write(CPU_STACK_ADDRESS + cpu.sp, value);
   --cpu.sp;
}

/**
 * increment stack pointer and pops value from stack
 * @returns the popped stack value
*/ 
static uint8_t stack_pop(void)
{
   ++cpu.sp;
   return bus_read(CPU_STACK_ADDRESS + cpu.sp);
}
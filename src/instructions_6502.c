#include <stdio.h>

#include "../includes/instructions_6502.h"
#include "../includes/cpu.h"
#include "../includes/cpu_ram.h"
#include "../includes/log.h"
#include "../includes/bus.h"
#include "../includes/util.h"

/* instruction functions */

/**
 * most instruction functions will return 0 but branch instructions 
 * for example return 1 to represent 1 extra cycle when a branch is taken
*/

// load instructions

static uint8_t LAS(void);
static uint8_t LAX(void);
static uint8_t LDA(void);
static uint8_t LDX(void);
static uint8_t LDY(void);
static uint8_t SAX(void);
static uint8_t SHA(void);
static uint8_t SHX(void);
static uint8_t SHY(void);
static uint8_t STA(void);
static uint8_t STX(void);
static uint8_t STY(void);

// transfer instructions

static uint8_t SHS(void);
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

static uint8_t TMP(void){return 0;} // temp function to handle illegal opcode for now

static uint8_t get_addressing_mode(address_modes_t address_mode, uint8_t opcode, uint8_t *extra_cycle);

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

static opcode_t* decoded_opcode = NULL; // current decoded opcode to be executed

/**
 * Stores the operand of the instrucion depending on it's adressing mode.
 * Is unused by certain addressing modes such as implied and accumulator modes
 * because the operations are well... implied.
*/
static uint16_t instruction_operand;


/**
 * Read more on different addressing modes here -> https://www.pagetable.com/c64ref/6502/?tab=3
 * Forms the instruction operand depending on the provided address mode.
 * @param address_mode the address mode of the current opcode
 * @param opcode opcode to execute is passed in for logging purposes
 * @param extra_cycles uint8_t pointer set to 1 if page is crossed to represent 1 extra cycle
 * else is set to zero
 * @returns the number of bytes the instruction occupies, 
 * can be used to determine how much to increment the program counter by
 * to point to the next instruction.
*/
static uint8_t get_addressing_mode(address_modes_t address_mode, uint8_t opcode, uint8_t *extra_cycle)
{
   uint8_t num_of_bytes = 1;

   switch (address_mode)
   {
      case IMP: // implied adressing modes are 1 byte
      {
         *extra_cycle = 0;

         nestest_log("%04X  %02X %6s %s %28s", cpu.pc, opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case ACC: // acccumulator addresing modes are 1 byte
      { 
         *extra_cycle = 0;

         nestest_log("%04X  %02X %7s %s %28s", cpu.pc, opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case IMM:
      {
         num_of_bytes = 2;
         *extra_cycle = 0;

         instruction_operand = bus_read(cpu.pc + 1);

         nestest_log("%04X  %02X %02X %3s %s #$%02X %23s", cpu.pc, opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case ABS:
      {
         num_of_bytes = 3;
         *extra_cycle = 0;

         instruction_operand = bus_read_u16(cpu.pc + 1);
         uint8_t lo = instruction_operand & 0x00FF;
         uint8_t hi = (instruction_operand & 0xFF00) >> 8;

         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, lo, hi, decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case XAB:
      {
         num_of_bytes = 3;

         uint16_t abs_address = bus_read_u16(cpu.pc + 1);
         uint8_t lo = abs_address & 0x00FF;
         uint8_t hi = (abs_address & 0xFF00) >> 8;

         instruction_operand = abs_address + cpu.X; // add absolute address with offset value in X index register

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken. 
         */
         *extra_cycle = ( ( instruction_operand & 0xFF00 ) >> 8 != hi ) ? 1 : 0;

         nestest_log("%04X  %02X %02X %02X %s $%04X, X @ %04X = %02X %8s", cpu.pc, opcode, lo, hi, decoded_opcode->mnemonic, abs_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case YAB:
      {
         num_of_bytes = 3;

         uint16_t abs_address = bus_read_u16(cpu.pc + 1);
         uint8_t lo = abs_address & 0x00FF;
         uint8_t hi = (abs_address & 0xFF00) >> 8;

         instruction_operand = abs_address + cpu.Y; // add absolute address with offset value in Y index register

         /**
          * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
          * This happens because the offset is added to the low byte first which can result in a carry out.
          * The carry out bit then needs to be added back into the high byte which results in a
          * extra cycle being taken. 
         */
         *extra_cycle = ( ( instruction_operand & 0xFF00 ) >> 8 != hi ) ? 1 : 0;

         nestest_log("%04X  %02X %02X %02X %s $%04X, Y @ %04X = %02X %8s", cpu.pc, opcode, lo, hi, decoded_opcode->mnemonic, abs_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case ABI:
      {
         num_of_bytes = 3;
         *extra_cycle = 0;

         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2), decoded_opcode->mnemonic, bus_read_u16(cpu.pc + 1), "");
         break;
      }
      case ZPG:
      {
         num_of_bytes = 2;
         *extra_cycle = 0;

         instruction_operand = bus_read(cpu.pc + 1);

         nestest_log("%04X  %02X %02X %3s %s $%02X = %02X %19s", cpu.pc, opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case XZP:
      {
         num_of_bytes = 2;
         *extra_cycle = 0;

         uint8_t zpg_address = bus_read(cpu.pc + 1);
         instruction_operand = zpg_address + cpu.X;

         nestest_log("%04X  %02X %02X %3s %s %02X, X @ %02X = %02X %13s", cpu.pc, opcode, zpg_address, "", decoded_opcode->mnemonic, zpg_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case YZP:
      {
         num_of_bytes = 2;
         *extra_cycle = 0;

         uint8_t zpg_address = bus_read(cpu.pc + 1);
         instruction_operand = zpg_address + cpu.Y;

         nestest_log("%04X  %02X %02X %3s %s %02X, Y @ %02X = %02X %13s", cpu.pc, opcode, zpg_address, "", decoded_opcode->mnemonic, zpg_address, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case XZI:
      {
         num_of_bytes = 2;
         *extra_cycle = 0;

         nestest_log("%04X  %02X %02X %3s %s ", cpu.pc, opcode, bus_read(cpu.pc + 1), "", decoded_opcode->mnemonic);
         break;
      }
      case YZI:
      {
         num_of_bytes = 2;

         nestest_log("%04X  %02X %02X %3s %s ", cpu.pc, opcode, bus_read(cpu.pc + 1), "", decoded_opcode->mnemonic);
         break;
      }
      case REL:
      {
         num_of_bytes = 2;

         uint8_t offset_byte = bus_read(cpu.pc + 1);
         
         if (offset_byte & 0x80) // check if offset byte is a signed negative number
         {
            instruction_operand = (cpu.pc + num_of_bytes) - ( ~offset_byte + 1);
         }
         else
         {
            instruction_operand = (cpu.pc + num_of_bytes) + offset_byte;
         }

         nestest_log("%04X  %02X %02X %3s %s $%04X %22s", cpu.pc, opcode, offset_byte, "", decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
   }

   nestest_log("A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu.ac, cpu.X, cpu.Y, cpu.status_flags, cpu.sp);

   return num_of_bytes;
}

/**
 * Decodes an opcode by using the opcode to index into the opcode lookup table
 * The decoded opcode will be executed on the next call to instruction_execute.
 * The amount of bytes to increment the pc register by is also set by this function.
 * @param opcode the opcode to decode
*/
void instruction_decode(uint8_t opcode)
{
   decoded_opcode = &opcode_lookup_table[opcode];
}

/**
 * execute the instruction that was previously decoded from a call to instruction_decode().
 * Only call this function after instruction_decode() has been called.
 * Will also increment the program counter prior to instruction execution.
 * @param opcode the opcode to execute is passed for logging purposes
 * @returns the number of cycles the executed instruction takes
*/
uint8_t instruction_execute(uint8_t opcode)
{
   uint8_t extra_cycles = 0;
   uint8_t num_of_bytes = get_addressing_mode(decoded_opcode->mode, opcode, &extra_cycles);

   cpu.pc += num_of_bytes;  // increment pc to point to next instruction before executing current instruction
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
      cpu.X = (uint8_t) instruction_operand;
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

static uint8_t LDY(void){return 0;}
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

static uint8_t STY(void){return 0;}

// transfer instructions

static uint8_t SHS(void){return 0;}
static uint8_t TAX(void){return 0;}
static uint8_t TAY(void){return 0;}
static uint8_t TSX(void){return 0;}
static uint8_t TXA(void){return 0;}
static uint8_t TXS(void){return 0;}
static uint8_t TYA(void){return 0;}

// stack instructions

static uint8_t PHA(void){return 0;}
static uint8_t PHP(void){return 0;}
static uint8_t PLA(void){return 0;}
static uint8_t PLP(void){return 0;}

// shift instructions

static uint8_t ASL(void){return 0;}
static uint8_t LSR(void){return 0;}
static uint8_t ROL(void){return 0;}
static uint8_t ROR(void){return 0;}

// logic instructions

static uint8_t AND(void){return 0;}

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

static uint8_t EOR(void){return 0;}
static uint8_t ORA(void){return 0;}

// arithmetic instructions

static uint8_t ADC(void){return 0;}
static uint8_t ANC(void){return 0;}
static uint8_t ARR(void){return 0;}
static uint8_t ASR(void){return 0;}
static uint8_t CMP(void){return 0;}
static uint8_t CPX(void){return 0;}
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

static uint8_t DEC(void){return 0;}
static uint8_t DEX(void){return 0;}
static uint8_t DEY(void){return 0;}
static uint8_t INC(void){return 0;}
static uint8_t INX(void){return 0;}
static uint8_t INY(void){return 0;}

// control

static uint8_t BRK(void){return 0;}

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
   uint8_t hi = ( cpu.pc & 0xFF00 ) >> 8;
   bus_write(CPU_STACK_ADDRESS + cpu.sp, hi);
   cpu.sp -= 1;

   uint8_t lo = cpu.pc & 0x00FF;
   bus_write(CPU_STACK_ADDRESS + cpu.sp, lo);
   cpu.sp-= 1;

   cpu.pc = instruction_operand;

   return 0;
}

static uint8_t RTI(void){return 0;}

/**
 * return from subroutine
*/
static uint8_t RTS(void)
{
   

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
 * brannch if overflow flag is cleared
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

static uint8_t CLD(void){return 0;}
static uint8_t CLI(void){return 0;}
static uint8_t CLV(void){return 0;}

/**
 * sets the carry flag to 1
*/
static uint8_t SEC(void)
{
   set_bit(cpu.status_flags, 0);
   return 0;
}

static uint8_t SED(void){return 0;}
static uint8_t SEI(void){return 0;}
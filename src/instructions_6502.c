#include <stdio.h>

#include "../includes/instructions_6502.h"
#include "../includes/cpu.h"
#include "../includes/cpu_ram.h"
#include "../includes/log.h"
#include "../includes/bus.h"
#include "../includes/util.h"

/* instruction functions */

// load instructions

static void LAS(void);
static void LAX(void);
static void LDA(void);
static void LDX(void);
static void LDY(void);
static void SAX(void);
static void SHA(void);
static void SHX(void);
static void SHY(void);
static void STA(void);
static void STX(void);
static void STY(void);

// transfer instructions

static void SHS(void);
static void TAX(void);
static void TAY(void);
static void TSX(void);
static void TXA(void);
static void TXS(void);
static void TYA(void);

// stack instructions

static void PHA(void);
static void PHP(void);  
static void PLA(void);
static void PLP(void);

// shift instructions

static void ASL(void);
static void LSR(void);
static void ROL(void);
static void ROR(void);

// logic instructions

static void AND(void);
static void BIT(void);
static void EOR(void);
static void ORA(void);

// arithmetic instructions

static void ADC(void);
static void ANC(void);
static void ARR(void);
static void ASR(void);
static void CMP(void);
static void CPX(void);
static void CPY(void);
static void DCP(void);
static void ISC(void);
static void RLA(void);
static void RRA(void);
static void SBC(void);
static void SBX(void);
static void SLO(void);
static void SRE(void);
static void XAA(void);

// increment instructions

static void DEC(void);
static void DEX(void);
static void DEY(void);
static void INC(void);
static void INX(void);
static void INY(void);

// control

static void BRK(void);
static void JMP(void);
static void JSR(void);
static void RTI(void);
static void RTS(void);

// branch instructions

static void BCC(void);
static void BCS(void);
static void BEQ(void);
static void BMI(void);
static void BNE(void);
static void BPL(void);
static void BVC(void);
static void BVS(void);

// flags instructions

static void CLC(void);
static void CLD(void);
static void CLI(void);
static void CLV(void);
static void SEC(void);
static void SED(void);
static void SEI(void);

// kil

static void JAM(void){}

// nop instructions

static void NOP(void){return;}

static void TMP(void){} // temp function to handle illegal opcode for now

static uint8_t get_addressing_mode(address_modes_t address_mode, uint8_t opcode);

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
 * set instruction parameters depending on the provide address mode
 * @param address_mode the address mode of the current opcode
 * @param opcode opcode to execute is passed in for logging purposes
 * @returns the number of bytes the instruction occupies, 
 * can be used to determine how much to increment the program counter by
 * to point to the next instruction.
*/
static uint8_t get_addressing_mode(address_modes_t address_mode, uint8_t opcode)
{
   uint8_t num_of_bytes = 1;

   switch (address_mode)
   {
      case IMP: // implied adressing modes are 1 byte
      {
         nestest_log("%04X  %02X %6s %s %28s", cpu.pc, opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case ACC: // acccumulator addresing modes are 1 byte
      { 
         nestest_log("%04X  %02X %7s %s %28s", cpu.pc, opcode, "", decoded_opcode->mnemonic, "");
         break;
      }
      case IMM:
      {
         num_of_bytes = 2;
         instruction_operand = bus_read(cpu.pc + 1);
         nestest_log("%04X  %02X %02X %3s %s #$%02X %23s", cpu.pc, opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case ABS:
      {
         num_of_bytes = 3;
         instruction_operand = bus_read_u16(cpu.pc + 1);
         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2), decoded_opcode->mnemonic, instruction_operand, "");
         break;
      }
      case XAB:
      {
         num_of_bytes = 3;
         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2), decoded_opcode->mnemonic, bus_read_u16(cpu.pc + 1), "");
         break;
      }
      case YAB:
      {
         num_of_bytes = 3;
         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2), decoded_opcode->mnemonic, bus_read_u16(cpu.pc + 1), "");
         break;
      }
      case ABI:
      {
         num_of_bytes = 3;
         nestest_log("%04X  %02X %02X %-03X %s $%04X %22s", cpu.pc, opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2), decoded_opcode->mnemonic, bus_read_u16(cpu.pc + 1), "");
         break;
      }
      case ZPG:
      {
         instruction_operand = bus_read(cpu.pc + 1);
         num_of_bytes = 2;
         nestest_log("%04X  %02X %02X %3s %s $%02X = %02X %19s", cpu.pc, opcode, instruction_operand, "", decoded_opcode->mnemonic, instruction_operand, bus_read(instruction_operand), "");
         break;
      }
      case XZP:
      {
         num_of_bytes = 2;
         nestest_log("%04X  %02X %02X %3s %s ", cpu.pc, opcode, bus_read(cpu.pc + 1), "", decoded_opcode->mnemonic);
         break;
      }
      case YZP:
      {
         num_of_bytes = 2;
         nestest_log("%04X  %02X %02X %3s %s ", cpu.pc, opcode, bus_read(cpu.pc + 1), "", decoded_opcode->mnemonic);
         break;
      }
      case XZI:
      {
         num_of_bytes = 2;
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
         nestest_log("%04X  %02X %02X %3s %s ", cpu.pc, opcode, bus_read(cpu.pc + 1), "", decoded_opcode->mnemonic);
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
   uint8_t num_of_bytes = get_addressing_mode(decoded_opcode->mode, opcode);

   cpu.pc += num_of_bytes;            // increment pc to point to next instruction before executing current instruction
   decoded_opcode->opcode_function(); // execute the current instruction

   return decoded_opcode->cycles;
}

// load instructions

static void LAS(void){}
static void LAX(void){}

/**
 * load accumulator with value from memory
 * sets zero flag if accumulator is set to zero, otherwise zero flag is reset
 * sets negative flag if bit 7  of accumulator is 1, otherwises negative flag is reset
*/
static void LDA(void){}

/**
 * Load X register with value from memory
 * Set zero bit if loaded value is zero.
 * Set negative bit if loaded value has bit 7 set to 1.
*/
static void LDX(void)
{
   if (instruction_operand == 0)
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   uint8_t bit_7 = ( instruction_operand & ( 1 << 7 ) ) >> 7;
   if (bit_7 == 1)
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   cpu.X = (uint8_t) instruction_operand;
}

static void LDY(void){}
static void SAX(void){}
static void SHA(void){}
static void SHX(void){}
static void SHY(void){}
static void STA(void){}
static void STX(void)
{
   bus_write(instruction_operand, cpu.X);
}

static void STY(void){}

// transfer instructions

static void SHS(void){}
static void TAX(void){}
static void TAY(void){}
static void TSX(void){}
static void TXA(void){}
static void TXS(void){}
static void TYA(void){}

// stack instructions

static void PHA(void){}
static void PHP(void){}
static void PLA(void){}
static void PLP(void){}

// shift instructions

static void ASL(void){}
static void LSR(void){}
static void ROL(void){}
static void ROR(void){}

// logic instructions

static void AND(void){}
static void BIT(void){}
static void EOR(void){}
static void ORA(void){}

// arithmetic instructions

static void ADC(void){}
static void ANC(void){}
static void ARR(void){}
static void ASR(void){}
static void CMP(void){}
static void CPX(void){}
static void CPY(void){}
static void DCP(void){}
static void ISC(void){}
static void RLA(void){}
static void RRA(void){}
static void SBC(void){}
static void SBX(void){}
static void SLO(void){}
static void SRE(void){}
static void XAA(void){}

// increment instructions

static void DEC(void){}
static void DEX(void){}
static void DEY(void){}
static void INC(void){}
static void INX(void){}
static void INY(void){}

// control

static void BRK(void){}

/**
 * loads program counter with new jump value
*/
static void JMP(void)
{
   cpu.pc = instruction_operand;
}

/**
 * Jumps to a subroutine.
 * Save return address onto the stack
*/
static void JSR(void)
{
   uint8_t hi = ( cpu.pc & 0xFF00 ) >> 8;
   bus_write(CPU_STACK_ADDRESS + cpu.sp, hi);
   cpu.sp -= 1;

   uint8_t lo = cpu.pc & 0x00FF;
   bus_write(CPU_STACK_ADDRESS + cpu.sp, lo);
   cpu.sp-= 1;

   cpu.pc = instruction_operand;
}

static void RTI(void){}
static void RTS(void){}

// branch instructions

static void BCC(void){}
static void BCS(void){}
static void BEQ(void){}
static void BMI(void){}
static void BNE(void){}
static void BPL(void){}
static void BVC(void){}
static void BVS(void){}

// flags instructions

static void CLC(void){}
static void CLD(void){}
static void CLI(void){}
static void CLV(void){}
static void SEC(void){}
static void SED(void){}
static void SEI(void){}
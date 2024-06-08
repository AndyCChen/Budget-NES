#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../includes/cpu.h"
#include "../includes/log.h"
#include "../includes/bus.h"
#include "../includes/util.h"
#include "../includes/ppu.h"
#include "../includes/disassembler.h"
#include "../includes/controllers.h"
#include "../includes/display.h"

#define NMI_VECTOR       0xFFFA // address of non-maskable interrupt vector
#define RESET_VECTOR     0xFFFC // address of reset vector
#define INTERRUPT_VECTOR 0xFFFE // address of the interrupt vector

/**
 * Bottom address of the stack.
 * Stack pointer value is added to this address as a offset.
 * When a value is pushed, the stack pointer is decremented and when
 * a value is popped, the stack pointer is incremented
*/
#define CPU_STACK_ADDRESS 0x0100

static cpu_6502_t cpu;

static uint8_t cpu_fetch(void);
static uint8_t cpu_fetch_no_increment(void);
static uint8_t cpu_execute(void);
static void cpu_decode(uint8_t opcode);
static uint8_t branch(void);
static void set_instruction_operand(address_modes_t address_mode, uint8_t *extra_cycle);
static void stack_push(uint8_t value);
static uint8_t stack_pop(void);
static bool check_opcode_access_mode(uint8_t opcode);
static inline void update_disassembly(uint16_t pc, uint8_t next);

// current opcode of the current instruction
static uint8_t current_opcode;

// current instruction to be executed
static const instruction_t* current_instruction = NULL;

/**
 * Stores the operand of the instrucion depending on it's adressing mode.
 * Is unused by certain addressing modes such as implied and accumulator modes
 * because the operations are well... implied.
*/
static uint16_t instruction_operand;

// ------------------------------- instruction functions ----------------------------------------------------

/**
 * most instruction functions will return 0 but branch instructions 
 * for example return 1 to represent 1 extra cycle when a branch is taken
 * or 2 if a branch is taken and page boudary is crossed.
 * Function signature commented with * are undocumented opcodes, most games
 * do not make use of these instructions.
*/

// load instructions

static uint8_t LAS(void); // *
static uint8_t LAX(void); // * implemented
static uint8_t LDA(void);
static uint8_t LDX(void);
static uint8_t LDY(void);
static uint8_t SAX(void); // * implemented
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
static uint8_t ANC(void); // *
static uint8_t ARR(void); // * 
static uint8_t ASR(void); // *
static uint8_t CMP(void);
static uint8_t CPX(void);
static uint8_t CPY(void);
static uint8_t DCP(void); // * implemented
static uint8_t ISB(void); // * implemented
static uint8_t RLA(void); // * implemented
static uint8_t RRA(void); // * implemented
static uint8_t SBC(void);
static uint8_t SBX(void); // *
static uint8_t SLO(void); // * implemented
static uint8_t SRE(void); // * implemented
static uint8_t XAA(void); // *

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

// nop instruction

static uint8_t NOP(void){
   switch (current_instruction->mode)
   {
      case ZPG:
      case XZP:
      case XAB:
      case ABS:
         cpu_bus_read(instruction_operand);
         break;
      default:
         break;
   }

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

static const instruction_t instruction_lookup_table[256] = 
{
   /* 0x00 - 0x 0F */
   {"BRK", &BRK, IMP, 7},  {"ORA", &ORA, XZI, 6},  {"*JAM", &JAM, IMP, 0},
   {"*SLO", &SLO, XZI, 8}, {"*NOP", &NOP, ZPG, 3}, {"ORA", &ORA, ZPG, 3},
   {"ASL", &ASL, ZPG, 5},  {"*SLO", &SLO, ZPG, 5}, {"PHP", &PHP, IMP, 3},
   {"ORA", &ORA, IMM, 2},  {"ASL", &ASL, ACC, 2},  {"*ANC", &ANC, IMM, 2},
   {"*NOP", &NOP, ABS, 4}, {"ORA", &ORA, ABS, 4},  {"ASL", &ASL, ABS, 6},
   {"*SLO", &SLO, ABS, 6},

   /* 0x10 - 0x1F */
   {"BPL", &BPL, REL, 2},  {"ORA", &ORA, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*SLO", &SLO, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"ORA", &ORA, XZP, 4},
   {"ASL", &ASL, XZP, 6},  {"*SLO", &SLO, XZP, 6}, {"CLC", &CLC, IMP, 2},
   {"ORA", &ORA, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*SLO", &SLO, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"ORA", &ORA, XAB, 4},  {"ASL", &ASL, XAB, 7},
   {"*SLO", &SLO, XAB, 7},

   /* 0x20 - 0x2F */
   {"JSR", &JSR, ABS, 6},  {"AND", &AND, XZI, 6},  {"*JAM", &JAM, IMP, 0},
   {"*RLA", &RLA, XZI, 8}, {"BIT", &BIT, ZPG, 3},  {"AND", &AND, ZPG, 3},
   {"ROL", &ROL, ZPG, 5},  {"*RLA", &RLA, ZPG, 5}, {"PLP", &PLP, IMP, 4},
   {"AND", &AND, IMM, 2},  {"ROL", &ROL, ACC, 2},  {"*ANC", &ANC, IMM, 2},
   {"BIT", &BIT, ABS, 4},  {"AND", &AND, ABS, 4},  {"ROL", &ROL, ABS, 6},
   {"*RLA", &RLA, ABS, 6},

   /* 0x30 - 0x3F */
   {"BMI", &BMI, REL, 2},  {"AND", &AND, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*RLA", &RLA, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"AND", &AND, XZP, 4},
   {"ROL", &ROL, XZP, 6},  {"*RLA", &RLA, XZP, 6}, {"SEC", &SEC, IMP, 2},
   {"AND", &AND, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*RLA", &RLA, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"AND", &AND, XAB, 4},  {"ROL", &ROL, XAB, 7},
   {"*RLA", &RLA, XAB, 7},

   /* 0x40 - 0x4F */
   {"RTI", &RTI, IMP, 6},  {"EOR", &EOR, XZI, 6},  {"*JAM", &JAM, IMP, 0},
   {"*SRE", &SRE, XZI, 8}, {"*NOP", &NOP, ZPG, 3}, {"EOR", &EOR, ZPG, 3},
   {"LSR", &LSR, ZPG, 5},  {"*SRE", &SRE, ZPG, 5}, {"PHA", &PHA, IMP, 3}, 
   {"EOR", &EOR, IMM, 2},  {"LSR", &LSR, ACC, 2},  {"*ASR", &ASR, IMM, 2}, 
   {"JMP", &JMP, ABS, 3},  {"EOR", &EOR, ABS, 4},  {"LSR", &LSR, ABS, 6},
   {"*SRE", &SRE, ABS, 6},

   /* 0x50 - 0x5F */
   {"BVC", &BVC, REL, 2},  {"EOR", &EOR, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*SRE", &SRE, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"EOR", &EOR, XZP, 4},
   {"LSR", &LSR, XZP, 6},  {"*SRE", &SRE, XZP, 6}, {"CLI", &CLI, IMP, 2},
   {"EOR", &EOR, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*SRE", &SRE, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"EOR", &EOR, XAB, 4},  {"LSR", &LSR, XAB, 7},
   {"*SRE", &SRE, XAB,7},

   /* 0x60 - 0x6F */
   {"RTS", &RTS, IMP, 6},  {"ADC", &ADC, XZI, 6},  {"*JAM", &JAM, IMP, 0},
   {"*RRA", &RRA, XZI, 8}, {"*NOP", &NOP, ZPG, 3}, {"ADC", &ADC, ZPG, 3},
   {"ROR", &ROR, ZPG, 5},  {"*RRA", &RRA, ZPG, 5}, {"PLA", &PLA, IMP, 4},
   {"ADC", &ADC, IMM, 2},  {"ROR", &ROR, ACC, 2},  {"*ARR", &ARR, IMM, 2},
   {"JMP", &JMP, ABI, 5},  {"ADC", &ADC, ABS, 4},  {"ROR", &ROR, ABS, 6},
   {"*RRA", &RRA, ABS, 6},

   /* 0x70 - 7F */
   {"BVS", &BVS, REL, 2},  {"ADC", &ADC, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*RRA", &RRA, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"ADC", &ADC, XZP, 4},
   {"ROR", &ROR, XZP, 6},  {"*RRA", &RRA, XZP, 6}, {"SEI", &SEI, IMP, 2},
   {"ADC", &ADC, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*RRA", &RRA, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"ADC", &ADC, XAB, 4},  {"ROR", &ROR, XAB, 7},
   {"*RRA", &RRA, XAB, 7},

   /* 0x80 - 0x8F */
   {"*NOP", &NOP, IMM, 2}, {"STA", &STA, XZI, 6},  {"*NOP", &NOP, IMM, 2},
   {"*SAX", &SAX, XZI, 6}, {"STY", &STY, ZPG, 3},  {"STA", &STA, ZPG, 3},
   {"STX", &STX, ZPG, 3},  {"*SAX", &SAX, ZPG, 3}, {"DEY", &DEY, IMP, 2},
   {"*NOP", &NOP, IMM, 2}, {"TXA", &TXA, IMP, 2},  {"*XAA", &XAA, IMM, 2},
   {"STY", &STY, ABS, 4},  {"STA", &STA, ABS, 4},  {"STX", &STX, ABS, 4},
   {"*SAX", &SAX, ABS, 4},

   /* 0x90 - 0x9F */
   {"BCC", &BCC, REL, 2},  {"STA", &STA, YZI, 6},  {"*JAM", &JAM, IMP, 0},
   {"*SHA", &SHA, YZI, 6}, {"STY", &STY, XZP, 4},  {"STA", &STA, XZP, 4},
   {"STX", &STX, YZP, 4},  {"*SAX", &SAX, YZP, 4}, {"TYA", &TYA, IMP, 2},
   {"STA", &STA, YAB, 5},  {"TXS", &TXS, IMP, 2},  {"*SHS", &SHS, YAB, 5},
   {"*SHY", &SHY, XAB, 5}, {"STA", &STA, XAB, 5},  {"*SHX", &SHX, YAB, 5},
   {"*SHA", &SHA, YAB, 5},

   /* 0xA0 - 0xAF */
   {"LDY", &LDY, IMM, 2},  {"LDA", &LDA, XZI, 6},  {"LDX", &LDX, IMM, 2},
   {"*LAX", &LAX, XZI, 6}, {"LDY", &LDY, ZPG, 3},  {"LDA", &LDA, ZPG, 3},
   {"LDX", &LDX, ZPG, 3},  {"*LAX", &LAX, ZPG, 3}, {"TAY", &TAY, IMP, 2},
   {"LDA", &LDA, IMM, 2},  {"TAX", &TAX, IMP, 2},  {"*LAX", &LAX, IMM, 2},
   {"LDY", &LDY, ABS, 4},  {"LDA", &LDA, ABS, 4},  {"LDX", &LDX, ABS, 4},
   {"*LAX", &LAX, ABS, 4},

   /* 0xB0 - 0xBF */
   {"BCS", &BCS, REL, 2},  {"LDA", &LDA, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*LAX", &LAX, YZI, 5}, {"LDY", &LDY, XZP, 4},  {"LDA", &LDA, XZP, 4},
   {"LDX", &LDX, YZP, 4},  {"*LAX", &LAX, YZP, 4}, {"CLV", &CLV, IMP, 2},
   {"LDA", &LDA, YAB, 4},  {"TSX", &TSX, IMP, 2},  {"*LAS", &LAS, YAB, 4},
   {"LDY", &LDY, XAB, 4},  {"LDA", &LDA, XAB, 4},  {"LDX", &LDX, YAB, 4},
   {"*LAX", &LAX, YAB, 4},

   /* 0xC0 - 0xCF */
   {"CPY", &CPY, IMM, 2},  {"CMP", &CMP, XZI, 6},  {"*NOP", &NOP, IMM, 2},
   {"*DCP", &DCP, XZI, 8}, {"CPY", &CPY, ZPG, 3},  {"CMP", &CMP, ZPG, 3},
   {"DEC", &DEC, ZPG, 5},  {"*DCP", &DCP, ZPG, 5}, {"INY", &INY, IMP, 2},
   {"CMP", &CMP, IMM, 2},  {"DEX", &DEX, IMP, 2},  {"*SBX", &SBX, IMM, 2},
   {"CPY", &CPY, ABS, 4},  {"CMP", &CMP, ABS, 4},  {"DEC", &DEC, ABS, 6},
   {"*DCP", &DCP, ABS, 6},

   /* 0xD0 - 0xDF */
   {"BNE", &BNE, REL, 2},  {"CMP", &CMP, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*DCP", &DCP, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"CMP", &CMP, XZP, 4},
   {"DEC", &DEC, XZP, 6},  {"*DCP", &DCP, XZP, 6}, {"CLD", &CLD, IMP, 2},
   {"CMP", &CMP, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*DCP", &DCP, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"CMP", &CMP, XAB, 4},  {"DEC", &DEC, XAB, 7},
   {"*DCP", &DCP, XAB, 7},

   /* 0xE0 - 0xEF */
   {"CPX", &CPX, IMM, 2},  {"SBC", &SBC, XZI, 6},  {"*NOP", &NOP, IMM, 2},
   {"*ISB", &ISB, XZI, 8}, {"CPX", &CPX, ZPG, 3},  {"SBC", &SBC, ZPG, 3},
   {"INC", &INC, ZPG, 5},  {"*ISB", &ISB, ZPG, 5}, {"INX", &INX, IMP, 2},
   {"SBC", &SBC, IMM, 2},  {"NOP", &NOP, IMP, 2},  {"*SBC", &SBC, IMM, 2},
   {"CPX", &CPX, ABS, 4},  {"SBC", &SBC, ABS, 4},  {"INC", &INC, ABS, 6},
   {"*ISB", &ISB, ABS, 6},

   /* 0xF0 - 0xFF */
   {"BEQ", &BEQ, REL, 2},  {"SBC", &SBC, YZI, 5},  {"*JAM", &JAM, IMP, 0},
   {"*ISB", &ISB, YZI, 8}, {"*NOP", &NOP, XZP, 4}, {"SBC", &SBC, XZP, 4},
   {"INC", &INC, XZP, 6},  {"*ISB", &ISB, XZP, 6}, {"SED", &SED, IMP, 2},
   {"SBC", &SBC, YAB, 4},  {"*NOP", &NOP, IMP, 2}, {"*ISB", &ISB, YAB, 7},
   {"*NOP", &NOP, XAB, 4}, {"SBC", &SBC, XAB, 4},  {"INC", &INC, XAB, 7},
   {"*ISB", &ISB, XAB, 7}
};

/**
 * Read more on different addressing modes here -> https://www.pagetable.com/c64ref/6502/?tab=3
 * Forms the instruction operand depending on the provided address mode.
 * @param address_mode the address mode of the current opcode
 * @param extra_cycle set to 1 if page is crossed to represent 1 extra cycle,
 * else is set to zero
*/
static void set_instruction_operand(address_modes_t address_mode, uint8_t *extra_cycle)
{
   *extra_cycle = 0;

   switch (address_mode)
   {
      case IMP:
      {
         cpu_bus_read(cpu.pc); // dummy read next byte
         break;
      }
      case ACC:
      { 
         cpu_bus_read(cpu.pc); // dummy read next byte
         break;
      }
      case IMM:
      {
         instruction_operand = cpu_fetch();
         break;
      }
      case ABS:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         instruction_operand = ( hi << 8 ) | lo;
         
         break;
      }
      case XAB:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         instruction_operand = abs_address + cpu.X;

         /**
          * Only read instructions are able to take advantage of optimizing away a extra cycle
          * when a page is not crossed. Write instructions always incur this penalty which is
          * by default included in the base clock cycle count. This is why the extra cycle is ignored
          * for write instructions as it is already accounted for.
         */
         if ( !check_opcode_access_mode(current_opcode) )
         {
            /**
             * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
             * This happens because the offset is added to the low byte first which can result in a carry out.
             * The carry out bit then needs to be added back into the high byte which results in a
             * extra cycle being taken. 
            */
            *extra_cycle = ( (instruction_operand & 0xFF00) != (hi << 8) ) ? 1 : 0;

            if (*extra_cycle)
            {
               // when page is crossed, emulate the dummy read to the incorrect address
               cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.X) );
            }
         }
         else
         {
            cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.X) );
         }

         break;
      }
      case YAB:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         instruction_operand = abs_address + cpu.Y;

         /**
          * Only read instructions are able to take advantage of optimizing away a extra cycle
          * when a page is not crossed. Write instructions always incur this penalty which is
          * by default included in the base clock cycle count. This is why the extra cycle is ignored
          * for write instructions as it is already accounted for.
         */
         if ( !(check_opcode_access_mode(current_opcode)) )
         {
            /**
             * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
             * This happens because the offset is added to the low byte first which can result in a carry out.
             * The carry out bit then needs to be added back into the high byte which results in a
             * extra cycle being taken. 
            */
            *extra_cycle = ( (instruction_operand & 0xFF00) != hi << 8 ) ? 1 : 0;

            if (*extra_cycle)
            {
               // when page is crossed, emulate the dummy read to the incorrect address
               cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.Y) );
            }
         }
         else
         {
            cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.Y) );
         }

         break;
      }
      case ABI:
      {
         uint8_t lo = cpu_fetch();
         uint8_t hi = cpu_fetch();

         uint16_t abs_address = ( hi << 8 ) | lo;

         // does not handle page crossing, i.e fetching at 0x02FF will read from 0x02FF and 0x0200

         uint8_t indirect_address_lo = cpu_bus_read(abs_address);
         uint8_t indirect_address_hi = cpu_bus_read( ( hi << 8 ) | (uint8_t) (lo + 1) );

         instruction_operand = ( indirect_address_hi << 8 ) | indirect_address_lo;

         break;
      }
      case ZPG:
      {
         instruction_operand = cpu_fetch();

         break;
      }
      case XZP:
      {
         uint8_t zpg_address = cpu_fetch();

         cpu_bus_read(zpg_address); // dummy read while adding index
         instruction_operand = (uint8_t) ( zpg_address + cpu.X );

         break;
      }
      case YZP:
      {
         uint8_t zpg_address = cpu_fetch();

         cpu_bus_read(zpg_address); // dummy read while adding index
         instruction_operand = (uint8_t) ( zpg_address + cpu.Y );

         break;
      }
      case XZI:
      {
         uint8_t zpg_base_address  = cpu_fetch();
         cpu_bus_read(zpg_base_address); // 1 cycle for dummy fetched at address and to add X offset
         uint8_t zpg_address = ( zpg_base_address + cpu.X ); // add X index offsest to base address to form zpg address 

         uint8_t lo = cpu_bus_read(zpg_address);
         uint8_t hi = cpu_bus_read( (uint8_t) ( zpg_address + 1 ) ); // use 8-bit cast to stay within the zero page

         instruction_operand = ( hi << 8 ) | lo;

         break;
      }
      case YZI:
      {
         uint8_t zpg_address = cpu_fetch();

         uint8_t lo = cpu_bus_read(zpg_address);
         uint8_t hi = cpu_bus_read( (uint8_t) (zpg_address + 1) ); // 8-bit cast to stay within the zero page

         uint16_t base_address = ( hi << 8 ) | lo;

         instruction_operand = base_address + cpu.Y;

         /**
          * Only read instructions are able to take advantage of optimizing away a extra cycle
          * when a page is not crossed. Write instructions always incur this penalty which is
          * by default included in the base clock cycle count. This is why the extra cycle is ignored
          * for write instructions as it is already accounted for.
         */
         if ( !(check_opcode_access_mode(current_opcode)) )
         {
            /**
             * +1 extra cycle when the hi bytes are not equal, meaning page is crossed.
             * This happens because the offset is added to the low byte first which can result in a carry out.
             * The carry out bit then needs to be added back into the high byte which results in a
             * extra cycle being taken. 
            */
            *extra_cycle = ( (instruction_operand & 0xFF00) != (base_address & 0xFF00) ) ? 1 : 0;

            if (*extra_cycle)
            {
               // when page is crossed, emulate the dummy read to the incorrect address
               cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.Y) );
            }
         }
         else
         {
            cpu_bus_read( (hi << 8) | (uint8_t) (lo + cpu.Y) );
         }

         break;
      }
      case REL:
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
            instruction_operand = cpu.pc + ( offset_byte | 0xFF00 );
         }
         else
         {
            instruction_operand = cpu.pc + offset_byte;
         }

         break;
      }
   }
}

// load instructions

static uint8_t LAS(void){return 0;}

/**
 * Undocumented.
 * Loads the accumulator and X register with value form memory.
 * Set zero flag if loaded value is zero, else reset.
 * Set negative flag if loaded value has bit 7 set, else resest.
*/
static uint8_t LAX(void)
{
   uint8_t value;

   if (current_instruction->mode == IMM)
   {
      value = instruction_operand;
   }
   else
   {
      value = cpu_bus_read(instruction_operand);
   }

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

   cpu.ac = value;
   cpu.X = value;

   return 0;
}

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
   if (current_instruction->mode == IMM)
   {
      cpu.ac = (uint8_t) instruction_operand;
   }
   else
   {
      cpu.ac = cpu_bus_read(instruction_operand);
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
   if (current_instruction->mode == IMM)
   {
      cpu.X = (uint8_t) instruction_operand;
   }
   else 
   {
      cpu.X = cpu_bus_read(instruction_operand);
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
   if ( current_instruction->mode == IMM )
   {
      cpu.Y = (uint8_t) instruction_operand;
   }
   else // else use operand as effective memory address
   {
      cpu.Y = cpu_bus_read(instruction_operand);
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

/**
 * Undocumented.
 * Perform bitwise AND between accumulator and X register,
 * store result into memory
*/
static uint8_t SAX(void)
{
   cpu_bus_write(instruction_operand, cpu.ac & cpu.X);

   return 0;
}

static uint8_t SHA(void){return 0;}
static uint8_t SHX(void){return 0;}
static uint8_t SHY(void){return 0;}

/**
 * transfer accumulator value into memory
*/
static uint8_t STA(void)
{
   cpu_bus_write(instruction_operand, cpu.ac);
   return 0;
}

/**
 * transfers X register value into memory location
*/
static uint8_t STX(void)
{
   cpu_bus_write(instruction_operand, cpu.X);
   return 0;
}

/**
 * transfers Y register value into memory location
*/
static uint8_t STY(void)
{
   cpu_bus_write(instruction_operand, cpu.Y);
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
   stack_push(cpu.status_flags | 0x30);
   return 0;
}

/**
 * Pop a value from the stack and loads it into the accumulator.
 * Sets negative flag if bit 7 of accumulator is 1, else reset flag
 * Sets zero flag if if accumulator is 0, else reset
*/
static uint8_t PLA(void)
{
   cpu_tick(); // 1 cycle to increment stack pointer
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
*/
static uint8_t PLP(void)
{
   cpu_tick(); // 1 cycle to increment stack pointer
   cpu.status_flags = stack_pop();
   clear_bit(cpu.status_flags, 4); // make sure break flag is cleared when retrieving cpu flags from stack

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
   uint8_t shifted_value = 0, carry_bit = 0;

   if (current_instruction->mode == ACC)
   {
      carry_bit = cpu.ac >> 7; 
      shifted_value = cpu.ac << 1;

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = cpu_bus_read(instruction_operand);
      cpu_bus_write(instruction_operand, value_to_shift); // dummy write

      carry_bit |= value_to_shift >> 7;
      shifted_value = value_to_shift << 1;

      cpu_bus_write(instruction_operand, shifted_value);
   }

   store_bit(cpu.status_flags, carry_bit, 0); // move bit 7 into carry flag

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
   uint8_t shifted_value = 0, carry_bit = 0;

   if (current_instruction->mode == ACC)
   {
      carry_bit = ( cpu.ac & 0x01 ); 
      shifted_value = cpu.ac >> 1;

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = cpu_bus_read(instruction_operand);
      cpu_bus_write(instruction_operand, value_to_shift); // dummy write

      carry_bit |= value_to_shift & 0x01;
      shifted_value = value_to_shift >> 1;

      cpu_bus_write(instruction_operand, shifted_value);
   }

   store_bit(cpu.status_flags, carry_bit, 0); // move bit 0 into carry flag

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
 * The right most bit (bit 0) after the shift takes the value of the carry flag.
 * The left most bit (bit 7) that is shifted out is moved into the carry flag.
 * Set negative flag to bit 7 of the shifted result.
 * Set zero flag is shifted value is zero, else reset.
*/
static uint8_t ROL(void)
{
   uint8_t shifted_value, carry_bit;
   uint8_t carry_flag = cpu.status_flags & 0x01;

   if (current_instruction->mode == ACC)
   {
      carry_bit = cpu.ac & 0x80;                // store left most bit prior to shift
      shifted_value = cpu.ac << 1;              // left shift 1 bit
      store_bit(shifted_value, carry_flag, 0);  // store the carry flag into the right most bit

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = cpu_bus_read(instruction_operand);
      cpu_bus_write(instruction_operand, value_to_shift); // dummy write

      carry_bit = value_to_shift & 0x80;         // store left most bit prior to shift
      shifted_value = value_to_shift << 1;       // left shift 1 bit
      store_bit(shifted_value, carry_flag, 0);   // store the carry flag into the right most bit

      cpu_bus_write(instruction_operand, shifted_value);
   }

   carry_bit = carry_bit >> 7;
   store_bit(cpu.status_flags, carry_bit, 0); // store carry bit into carry flag

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
 * The left most bit (bit 7) after the shift takes the value of the carry flag.
 * The right most bit (bit 0) that is shifted out is moved into the carry flag.
 * Set negative flag to bit 7 of the shifted result.
 * Set zero flag is shifted value is zero, else reset.
*/
static uint8_t ROR(void)
{
   uint8_t shifted_value, carry_bit;
   uint8_t carry_flag = cpu.status_flags & 0x01;

   if (current_instruction->mode == ACC)
   {
      carry_bit = cpu.ac & 0x01;                // store right most bit prior to shift
      shifted_value = cpu.ac >> 1;              // right shift 1 bit
      store_bit(shifted_value, carry_flag, 7);  // store the carry flag into the leftmost bit

      cpu.ac = shifted_value;
   }
   else
   {
      uint8_t value_to_shift = cpu_bus_read(instruction_operand);
      cpu_bus_write(instruction_operand, value_to_shift); // dummy write

      carry_bit = value_to_shift & 0x01;         // store right most bit prior to shift
      shifted_value = value_to_shift >> 1;       // right shift 1 bit
      store_bit(shifted_value, carry_flag, 7);   // store the carry flag into the leftmost bit

      cpu_bus_write(instruction_operand, shifted_value);
   }

   store_bit(cpu.status_flags, carry_bit, 0); // store carry bit into carry flag

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
   if (current_instruction->mode == IMM)
   {
      cpu.ac = cpu.ac & (uint8_t) instruction_operand;
   }
   // else treat operand as a effective memory address
   else
   {
      cpu.ac = cpu.ac & cpu_bus_read(instruction_operand);
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
   uint8_t value = cpu_bus_read(instruction_operand);

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

   if (current_instruction->mode == IMM)
   {
      operand = (uint8_t) instruction_operand;
   }
   else
   {
      operand = cpu_bus_read(instruction_operand);
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

   if (current_instruction->mode == IMM)
   {
      value = (uint8_t) instruction_operand;
   }
   else
   {
      value =  cpu_bus_read(instruction_operand);
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

/**
 * Add value in memory and carry bit to the value in the accumulator,
 * the result is stored in the accumulator.
 * Set carry flag if the sum exceeds 255 to indicate a carry, else reset.
 * Set overflow flag when bit 7 of sum is changed due to exceeding +127 or -128, else reset.
 * Set negative flag if bit 7 of sum is set, else reset.
 * Set zero flag if result is 0, else reset.
*/
static uint8_t ADC(void)
{
   uint32_t sum;
   uint8_t value;
   uint8_t carry_bit = cpu.status_flags & 1;

   if (current_instruction->mode == IMM)
   {
      value = (uint8_t) instruction_operand;
   }
   else
   {
      value = cpu_bus_read(instruction_operand);
   }

   sum = cpu.ac + value + carry_bit; // do addition

   // set/reset carry flag
   if ( sum > 255 )
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }   

   // set/reset overflow flag

   // ~( cpu.ac ^ value)  ---- has bit 7 on if the sign bit (bit 7) is the same on both operands, else it is off
   //  ( cpu.ac ^ sum )   ---- has bit 7 on if the sign bit (bit 7) is different on both values, else it is off
   /**
    * Overflowing 127 or -128 will only ever happen if both operands are of a different sign.
    * If the sign bit of both values are different, the expression will always evaluate as false
    * as overflow will never occur.
    * Otherwise if both sign bits are the same, we check if the sign bit of the value prior to
    * the addition in cpu.ac is different to the sign bit in sum after the addition. If the sign bits
    * are different AND the sign bits of both operands the same, then we know a overflow has happened.
   */
   if ( ( ~( cpu.ac ^ value ) & ( cpu.ac ^ sum ) ) & 0x80 )
   {
      set_bit(cpu.status_flags, 6);
   }
   else
   {
      clear_bit(cpu.status_flags, 6);
   }

   cpu.ac = sum & 0xFF;

   // set/reset negative flag
   if ( cpu.ac & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
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
   if ( current_instruction->mode == IMM )
   {
      value = (uint8_t) instruction_operand;
   }
   else // use operand as effective memory address
   {
      value = cpu_bus_read(instruction_operand);
   }

   result = cpu.ac - value;

   // set/reset zero flag
   if ( value == cpu.ac )
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

   if (current_instruction->mode == IMM)
   {
      value = (uint8_t) instruction_operand;
   }
   else
   {
      value = cpu_bus_read(instruction_operand);
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
   if ( cpu.X == value )
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
 * Subtract value in memory from the Y register,
 * but does not store the result.
 * Set carry flag when absolute value of Y is >= than value in memory.
 * Set negative flag if result has bit 7 set, else reset.
 * Set zero flag if value in memory is equal to Y, else reset.
*/
static uint8_t CPY(void)
{
   uint8_t value, result;

   if (current_instruction->mode == IMM)
   {
      value = (uint8_t) instruction_operand;
   }
   else
   {
      value = cpu_bus_read(instruction_operand);
   }

   result = cpu.Y - value;

   // set/reset carry flag
   if ( cpu.Y >= value )
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
   if ( cpu.Y == value )
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
 * Undocumented.
 * Decrements value in memory by 1 in 2's complement. Then subtract the result from the accumulator
 * and does not store the result.
 * Set zero flag if value in memory - 1 equals value in accumulator, else reset.
 * Set negative flag if bit 7 of result from accumulator - decremented value in memory is set, else reset.
 * Set carry flag if decremented value in memory is <= to the accumulator, else reset.
*/
static uint8_t DCP(void)
{
   uint8_t result = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, result); // dummy write
   result = result + ( ~(0x01) + 1 ); // use 2's complement to add negative 1 which is equal to minus 1.

   // set/reset zero flag
   if (result == cpu.ac )
   {
      set_bit(cpu.status_flags, 1);
   }
   else
   {
      clear_bit(cpu.status_flags, 1);
   }

   // set/reset negative flag
   if ( (cpu.ac - result) & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset carry flag
   if ( result <= cpu.ac )
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }

   cpu_bus_write(instruction_operand, result);

   return 0;
}

/**
 * Undocumented.
 * Increments value in memory by 1, then performs the SBC instruction.
 * The incremented value in memory is written back into memory and the result of SBC
 * is stored back into the accumulator.
 * Set carry flag if result if > 255 to indicate a carry, else reset.
 * Set overflow flag if result exceeds -128 or 127, else reset.
 * Set negative flag if result has bit 7 set, else reset.
 * Set zero flag if result is 0, else reset.
*/
static uint8_t ISB(void)
{
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write
   value += 1;
   cpu_bus_write(instruction_operand, value);

   value = ~value; // negate value since subtraction is done using 2's complement addition

   uint8_t carry_bit = cpu.status_flags & 1;
   uint32_t sum = cpu.ac + value + carry_bit;

   // set/reset carry flag
   if (sum > 255)
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }

   // set/reset overflow flag
   if ( (cpu.ac ^ sum) & (value ^ sum) & 0x80 )
   {
      set_bit(cpu.status_flags, 6);
   }
   else
   {
      clear_bit(cpu.status_flags, 6);
   }

   cpu.ac =  (uint8_t) sum;

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
 * Undocumented.
 * Shifts/rotates value in memory 1 bit to the left where carry flag is shifted in
 * and the bit that is shifted out is stored in the carry flag, 
 * result is stored back into memory. Then performs bitwise and between
 * shifted value and the accumulator, storing the result into the accumulator.
 * Set carry flag to the bit that is shifted out.
 * Set negative flag if result in accumulator has bit 7 set, else reset.
 * Set zero flag if result in accumulator is 0, else reset
*/
static uint8_t RLA(void)
{
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write

   uint8_t shifted_in_bit = cpu.status_flags & 1;
   uint8_t shifted_out_bit = (value & 0x80) != 0;

   value = value << 1;
   
   store_bit(value, shifted_in_bit, 0);             // carry flag is rotated into value
   store_bit(cpu.status_flags, shifted_out_bit, 0); // bit that is rotated out is moved into carry flag

   cpu_bus_write(instruction_operand, value);           // store result back into memory

   cpu.ac = cpu.ac & value;

   // set/reset negative
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
 * Undocumented
 * Shifts/rotates value in memory right 1 bit where the shifted in bit
 * takes the value of the carry flag and the shifted out bit is stored in
 * the carry flag, the result is stored back into memory. Then perform
 * ADC instruction with rotated value and accumulator, storing the sum in
 * the accumulator.
 * Set carry flag if the sum exceeds 255 to indicate a carry, else reset.
 * Set overflow flag when bit 7 of sum is changed due to exceeding +127 or -128, else reset.
 * Set negative flag if bit 7 of sum is set, else reset.
 * Set zero flag if result is 0, else reset.
*/
static uint8_t RRA(void)
{
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value);

   uint8_t shifted_out_bit = value & 1;
   uint8_t shifted_in_bit = cpu.status_flags & 1;

   value = value >> 1;

   store_bit(cpu.status_flags, shifted_out_bit, 0);
   store_bit(value, shifted_in_bit, 7);

   cpu_bus_write(instruction_operand, value);

   uint8_t carry_bit = cpu.status_flags & 1;
   uint32_t sum = cpu.ac + value + carry_bit;

   // set/reset carry
   if (sum > 255)
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }

   // set/reset overflow flag
   if ( (cpu.ac ^ sum) & (value ^ sum) & 0x80 )
   {
      set_bit(cpu.status_flags, 6);
   }
   else
   {
      clear_bit(cpu.status_flags, 6);
   }

   cpu.ac = (uint8_t) sum;

   // set/reset negative
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
 * Subtract value in memory and the complemented carry bit from the value in the accumulator,
 * the result is stored in the accumulator.
 * Set carry flag set if result is > 255 else reset.
 * Set overflow flag when bit 7 of difference is changed due to exceeding +127 or -128, else reset.
 * Set negative flag if bit 7 of difference is set, else reset.
 * Set zero flag if difference is 0, else reset.
*/
static uint8_t SBC(void)
{
   uint32_t sum;
   uint8_t value;
   uint8_t carry_bit = cpu.status_flags & 1;

   if (current_instruction->mode == IMM)
   {
      value = (uint8_t) instruction_operand;
   }
   else
   {
      value = cpu_bus_read(instruction_operand);
   }

   // 8-bit int cast to prevent negation of value being promoted to 32-bit int
   sum = cpu.ac + (uint8_t) ~value + carry_bit; // use 2's complement to do subtraction, ( i.e. 5 - 2 == 5 + (-2) )

   // set/reset carry flag
   if ( sum > 255 )
   {
      set_bit(cpu.status_flags, 0);
   }
   else
   {
      clear_bit(cpu.status_flags, 0);
   }   

   // set/reset overflow flag

   // ~( cpu.ac ^ (~value + carry_bit) )  ---- has bit 7 on if the sign bit (bit 7) is the same on both operands, else it is off
   //  ( cpu.ac ^ sum )                   ---- has bit 7 on if the sign bit (bit 7) is different on both values, else it is off
   /**
    * Overflowing 127 or -128 will only ever happen if both operands are of a different sign.
    * If the sign bit of both values are different, the expression will always evaluate as false
    * as overflow will never occur.
    * Otherwise if both sign bits are the same, we check if the sign bit of the value prior to
    * the addition in cpu.ac is different to the sign bit in sum after the addition. If the sign bits
    * are different AND the sign bits of both operands the same, then we know a overflow has happened.
   */
   if ( ( ~( cpu.ac ^ (~value) ) & ( cpu.ac ^ sum ) ) & 0x80 )
   {
      set_bit(cpu.status_flags, 6);
   }
   else
   {
      clear_bit(cpu.status_flags, 6);
   }

   cpu.ac = (uint8_t) sum;

   // set/reset negative flag
   if ( cpu.ac & 0x80 )
   {
      set_bit(cpu.status_flags, 7);
   }
   else
   {
      clear_bit(cpu.status_flags, 7);
   }

   // set/reset zero flag
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

static uint8_t SBX(void){return 0;}

/**
 * Undocumented.
 * Shifts value in memory 1 bit to the left where bit 7 is shifted out into the
 * carry flag, result is stored back into memory. Then performs bitwise or between
 * shifted value and the accumulator, storing the result into the accumulator.
 * Set carry flag to the bit that is shifted out.
 * Set negative flag if result in accumulator has bit 7 set, else reset.
 * Set zero flag if result in accumulator is 0, else reset
*/
static uint8_t SLO(void)
{
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write

   uint8_t shifted_out_bit = (value & 0x80) != 0;
   store_bit(cpu.status_flags, shifted_out_bit, 0);

   value = value << 1;
   cpu_bus_write(instruction_operand, value);

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

/**
 * Undocumented
 * Shift value in memory right 1 bit, shifted out bit is stored
 * in carry flag, result is stored back into memory. Then perform
 * bitwise XOR the shifted value with accumulator, store result back into
 * accumulator.
 * Set negative flag if bit 7 of accumulator is set, else reset.
 * Set zero flag if result in accumulator is 0, else reset.
*/
static uint8_t SRE(void)
{
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write

   uint8_t shifted_out_bit = value & 1;
   store_bit(cpu.status_flags, shifted_out_bit, 0);

   value = value >> 1;
   cpu_bus_write(instruction_operand, value);

   cpu.ac = cpu.ac ^ value;

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
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write
   cpu_bus_write(instruction_operand, --value);

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
   uint8_t value = cpu_bus_read(instruction_operand);
   cpu_bus_write(instruction_operand, value); // dummy write
   cpu_bus_write(instruction_operand, ++value);

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
 * The cpu then transfers control to the address located at the interrupt vector 0xFFFE.
*/
static uint8_t BRK(void)
{
   //cpu_fetch(); // read next byte and ingore fetched result while incrementing pc
   cpu.pc += 1;
   stack_push( (cpu.pc & 0xFF00) >> 8 );
   stack_push( cpu.pc & 0x00FF );

   stack_push(cpu.status_flags | 0x30); // break and unused flag pushed as 1

   set_bit(cpu.status_flags, 2); // set interrupt disable flag

   uint8_t lo = cpu_bus_read(INTERRUPT_VECTOR);
   uint8_t hi = cpu_bus_read(INTERRUPT_VECTOR + 1);

   cpu.pc = (hi << 8) | lo;

   update_disassembly(cpu.pc, MAX_NEXT);

   return 0;
}

/**
 * loads program counter with new jump value
*/
static uint8_t JMP(void)
{
   cpu.pc = instruction_operand;

   update_disassembly(cpu.pc, MAX_NEXT);
   return 0;
}

/**
 * Jumps to a subroutine.
 * Save return address onto the stack
*/
static uint8_t JSR(void)
{  
   cpu_tick();

   // push the pc address that points to the last byte of the JSR instruction
   uint16_t return_address = cpu.pc - 1; // the pc currently points to next opcode byte so we minus one to point to last byte of JSR

   uint8_t hi = ( return_address & 0xFF00 ) >> 8;
   stack_push(hi);

   uint8_t lo = return_address & 0x00FF;
   stack_push(lo);

   cpu.pc = instruction_operand;

   update_disassembly(cpu.pc, MAX_NEXT);

   return 0;
}

/**
 * Returns from a interrupt.
 * Pops the status flag register and the program counter from the stack
 * and loads them back into respective cpu registers.
*/
static uint8_t RTI(void)
{
   cpu_fetch_no_increment();
   cpu_tick(); // 1 cycle to increment stack pointer
   cpu.status_flags = stack_pop();
   uint8_t lo = stack_pop();
   uint8_t hi = stack_pop();

   clear_bit(cpu.status_flags, 4); // break flag is always reset when popped from stack

   cpu.pc = ( hi << 8) | lo;

   update_disassembly(cpu.pc, MAX_NEXT);

   return 0;
}

/**
 * return from subroutine
*/
static uint8_t RTS(void)
{
   cpu_tick(); // inital stack pointer increment takes 1 cycle
   uint8_t lo = stack_pop();
   uint8_t hi = stack_pop();
   
   cpu.pc = ( hi << 8 ) | lo;
   ++cpu.pc;
   cpu_tick(); // 1 cycle used for incrementing pc

   update_disassembly(cpu.pc, MAX_NEXT);

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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
      return 1 + branch();
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
 * Function that services hardware interrupts.
*/
void cpu_IRQ(void)
{ 
   if (cpu.status_flags & 4) return; // ignore IRQ if interrupt disable flag is set

   stack_push( ( cpu.pc & 0xFF00 ) >> 8 );
   stack_push( cpu.pc & 0x00FF );

   clear_bit(cpu.status_flags, 4); // make sure the break flag is cleared when pushed
   stack_push(cpu.status_flags);

   set_bit(cpu.status_flags, 2);  // set interrupt flag to ignore further IRQs

   uint8_t lo = cpu_bus_read(INTERRUPT_VECTOR);
   uint8_t hi = cpu_bus_read(INTERRUPT_VECTOR + 1);

   cpu.pc = (hi << 8) | lo;

   update_disassembly(cpu.pc, MAX_NEXT + 1);
}

void cpu_NMI(void)
{
   cpu_fetch_no_increment(); // fetch opcode
   cpu_fetch_no_increment(); // attempt to fetch next instruction by fail since pc increment is supressed

   stack_push( (cpu.pc & 0xFF00) >> 8 );
   stack_push(cpu.pc & 0x00FF);

   clear_bit(cpu.status_flags, 4); // make sure the break flag is cleared when pushed
   stack_push(cpu.status_flags);

   set_bit(cpu.status_flags, 2);  // set interrupt flag to ignore further IRQs

   uint8_t lo = cpu_bus_read(NMI_VECTOR);
   uint8_t hi = cpu_bus_read(NMI_VECTOR + 1);

   cpu.pc = (hi << 8) | lo;

   update_disassembly(cpu.pc, MAX_NEXT + 1);
}

/**
 * Performs a branch by using loading calculated address in instruction_operand
 * into the program counter.
 * @returns 1 when page boundary is crossed to represent +1 extra cycle taken to
 * correct the high byte.
*/
static uint8_t branch(void)
{
   cpu_fetch_no_increment(); // next instruction byte is fetched in the cpu pipeline
   uint8_t extra_cycle = (instruction_operand & 0xFF00) != (cpu.pc & 0xFF00);

   if (extra_cycle) cpu_tick(); // extra cycle to fix pc high byte due to page cross

   cpu.pc = instruction_operand;

   update_disassembly(cpu.pc, MAX_NEXT);

   return extra_cycle;
}

/**
 * Wrapper function around bus_read() that fetches a byte at current location of program counter address.
 * Increments program counter after fetching.
 * @returns the fetched byte
*/
static uint8_t cpu_fetch(void)
{
   // fetch
   uint8_t fetched_byte = cpu_bus_read(cpu.pc);
   ++cpu.pc;

   return fetched_byte;
}

/**
 * Same exact function as cpu_fetch but does not increment the program counter
 * @returns the fetched byte
*/
static uint8_t cpu_fetch_no_increment(void)
{
   // fetch
   uint8_t fetched_byte = cpu_bus_read(cpu.pc);

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
   current_instruction = &instruction_lookup_table[opcode];
   current_opcode = opcode;
}

/**
 * execute the instruction that was previously decoded from a call to cpu_decode().
 * Only call this function after cpu_decode() has been called.
 * @returns the number of cycles the executed instruction takes
*/
static uint8_t cpu_execute(void)
{
   uint8_t extra_cycles = 0;
   set_instruction_operand(current_instruction->mode, &extra_cycles);
   extra_cycles += current_instruction->opcode_function(); // execute the current instruction

   return current_instruction->cycles + extra_cycles;
}

/**
 * push a value onto the stack and decrement stack pointer
 * @param value the value to push onto the stack
 */ 
static void stack_push(uint8_t value)
{
   cpu_bus_write(CPU_STACK_ADDRESS + cpu.sp, value);
   --cpu.sp;
}

/**
 * increment stack pointer and pops value from stack
 * @returns the popped stack value
*/ 
static uint8_t stack_pop(void)
{
   ++cpu.sp;
   return cpu_bus_read(CPU_STACK_ADDRESS + cpu.sp);
}

/**
 * Emulate the execution of one 6502 cpu instruction
 * and counts the number of cycles that the execution would have taken.
*/
void cpu_emulate_instruction(void)
{  
   //if (cpu.pc == 0x8082) get_emulator_state()->run_state = EMULATOR_PAUSED;
   log_cpu_state("A:%02X X:%02X Y:%02X SP:%02X P:%02X", cpu.ac, cpu.X, cpu.Y, cpu.sp, cpu.status_flags);

   uint8_t opcode = cpu_fetch();
   cpu_decode(opcode);
   cpu_execute();

   controller_reload_shift_registers(); // check if controller shifts registers need to be reloaded
   disassemble();

   if (cpu.nmi_flip_flop)
   {
      cpu.nmi_flip_flop = false;
      cpu_NMI();
   }
}
#include "SDL_timer.h"
/**
 * Run the cpu for an x amount of clock cycles per frame depending refresh rate
*/
void cpu_run()
{
   static uint64_t delta_time = 0;
   static uint64_t previous_time = 0;
   static uint64_t current_time = 0;

   current_time = SDL_GetTicks64();
   delta_time += current_time - previous_time;

   if ( delta_time >= 17 )
   {
      delta_time -= 17;

      while ( cpu.cycle_count <= 1789773 / 60.0f  )
      {
         cpu_emulate_instruction();
      }
      cpu.cycle_count = 0;
       static int counter = 0;
      if (counter++ == 59)
      {
         counter = 0;
         static int i = 0;
         printf("%d %lld\n", i++, fc());
      }

   }

/*    int i = 0;
   while ( i < 2000 )
   {
      ++i;
      cpu_emulate_instruction();
   } */

   
   previous_time = current_time;
   //printf("%lld\n", fc());
}

/**
 * Ticks cpu by 1 clock cycle and runs ppu for 3 cycles
*/
void cpu_tick(void)
{
   cpu.cycle_count += 1;
   ppu_cycle();
   ppu_cycle();
   ppu_cycle();
}

/**
 * resets cpu register states
*/
void cpu_reset(void)
{
   cpu.cycle_count = 0;
   cpu.sp = 0xFD;
   cpu.status_flags = cpu.status_flags | 0x4;
   uint8_t lo = cpu_bus_read(RESET_VECTOR);
   uint8_t hi =  cpu_bus_read(RESET_VECTOR + 1);
   cpu.pc = (hi << 8) | lo;

   update_disassembly(cpu.pc, MAX_NEXT + 1);
}

/**
 * Initilize cpu state at power up
*/
void cpu_init(void)
{
   cpu.cycle_count = 0;
   cpu.nmi_flip_flop = false;
   cpu.ac = 0;
   cpu.X = 0;
   cpu.Y = 0;
   cpu.sp = 0xFD;
   cpu.status_flags = 0x04;
   uint8_t lo = cpu_bus_read(RESET_VECTOR);
   uint8_t hi =  cpu_bus_read(RESET_VECTOR + 1);
   cpu.pc = (hi << 8) | lo;

   disassemble_set_position(cpu.pc); // tell the disassembler to begin disassembling intructions at the current pc value
   disassemble_next_x(MAX_NEXT + 1); // disassemble the next x instructions
}

/**
 * This function checks if a opcode which uses a addressing mode
 * that contains page crossing is a write or read instruction.
 * This is to handle edge cases where write instructions do not take an
 * extra cycle as the extra cycle is already included in the base cycle count.
 * @param opcode the opcode to check
 * @returns true if opcode is a write instruction, else return false.
*/
static bool check_opcode_access_mode(uint8_t opcode)
{
   bool is_a_write_instruction;

   switch (opcode)
   {
      // X-indexed absolute
      case 0x1E: // ASL
      case 0xDF: // DCP
      case 0xDE: // DEC
      case 0xFE: // INC
      case 0xFF: // ISC
      case 0x5E: // LSR
      case 0x3F: // RLA
      case 0x3E: // ROL
      case 0x7E: // ROR
      case 0x7F: // RRA
      case 0x9C: // SHY
      case 0x1F: // SLO
      case 0x5F: // SRE
      case 0x9D: // STA

      // Y-indexed absolute
      case 0xDB: // DCP
      case 0xFB: // ISC
      case 0x3B: // RLA
      case 0x7B: // RRA
      case 0x9F: // SHA
      case 0x9B: // SHS
      case 0x9E: // SHX
      case 0x1B: // SLO
      case 0x5B: // SRE
      case 0x99: // STA

      // Zero page indirect Y-indexed
      case 0xD3: // DCP
      case 0xF3: // ISC
      case 0x33: // RLA
      case 0x73: // RRA
      case 0x93: // SHA
      case 0x13: // SLO
      case 0x53: // SRE
      case 0x91: // STA
         is_a_write_instruction = true;
         break;

      default:
         is_a_write_instruction = false;
   }

   return is_a_write_instruction;
}

/**
 * Returns pointer to cpu struct
*/
cpu_6502_t* get_cpu(void)
{
   return &cpu;
}

/**
 * @param position the opcode byte index to lookup
 * @return pointer to lookup entry
*/
const instruction_t* get_instruction_lookup_entry(uint8_t position)
{
   return &instruction_lookup_table[position];
}

/**
 * When ever program jumps to new location to begin execution such as when branches are taken
 * or during jmp instructions, we must update the future disassembled instructions to reflect the new execution starting point.
*/
static inline void update_disassembly(uint16_t pc, uint8_t next)
{
   log_rewind(next);
   disassemble_set_position(pc);
   disassemble_next_x(next);
}

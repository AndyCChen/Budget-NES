#include "../includes/instructions_6502.h"

/* instruction functions */

// load instructions

static void LDA(void);
static void LDX(void);
static void LDY(void);
static void STA(void);
static void STX(void);
static void STY(void);

// transfer instructions

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
static void CMP(void);
static void CPX(void);
static void CPY(void);
static void SBC(void);

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

// nop instructions

static void NOP(void);

static void TMP(void); // temp function to handle illegal opcode for now

static opcode_t instruction_lookup_table[256] = 
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
};

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
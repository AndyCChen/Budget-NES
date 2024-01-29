#include "../includes/instructions_6502.h"

/* instruction functions */

// load instructions

static void LDA(void);
static void LSX(void);
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
   {"ROL", }
};
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
   {"BRK", &BRK, IMP, 7}, {"ORA", &ORA, XZI, 6}, {"???", &TMP, IMP, 0},
   {"???", &TMP, IMP, 0}, {"???", &TMP, IMP, 0}, {"ORA", &ORA, ZPG, 3},
};
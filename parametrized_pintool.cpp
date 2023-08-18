#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include "pin.H"
using std::cerr;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;

KNOB<UINT64> KnobInterval(KNOB_MODE_WRITEONCE, "pintool", "interval", "100", "Interval to instrument instructions");
KNOB<UINT32> KnobMaxCount(KNOB_MODE_WRITEONCE, "pintool", "maxcount", std::to_string(UINT32_MAX), "Maximum count of instructions to instrument (optional)");
static UINT32 instrumentedCount = 0; // count of times instrumentation has occurred
static UINT64 icount = 0;

std::vector<std::string> printBuffer;
const size_t PRINT_BUFFER_SIZE = 100000;
UINT64 bufferPrintCount = 0;



VOID printAfter(ADDRINT ip, CONTEXT* ctxt, string dis) {
    std::stringstream ss;

    if (KnobMaxCount.Value() != UINT32_MAX && instrumentedCount >= KnobMaxCount.Value()) {
        PIN_ExitApplication(0);
        return; // stop instrumenting after reaching the maximum count
    }
    icount++;

    if (icount % KnobInterval.Value() == 0) {
       instrumentedCount++;
       ss << "InstrumentedCount: " << instrumentedCount << std::endl;
       ss << "Instruction at address: " << ip << std::endl;
       ss << "Disassembled instruction: " << dis << std::endl;
       //std::cout << "Instruction at address: " << ip << std::endl;
       //std::cout << "InstrumentedCount: " << instrumentedCount << std::endl;
       //std::cout << "Disassembled instruction: " << dis << std::endl;

       // Predefined set of usually available general purpose registers
       REG usuallyAvailableRegisters[] = {REG_RAX, REG_RBX, REG_RCX, REG_RDX, REG_RDI, REG_RSI, REG_RBP, REG_RSP, REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15};
       for (auto& reg : usuallyAvailableRegisters) {
           //std::cout << REG_StringShort(reg) << ": " << PIN_GetContextReg(ctxt, reg) << std::endl;
	   ss << REG_StringShort(reg) << ": " << PIN_GetContextReg(ctxt, reg) << std::endl;
       }

       // XMM registers
       REG xmmRegisters[] = {REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7, REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11, REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15};
       for (auto& reg : xmmRegisters) {
           UINT32 xmmVals[4];
           PIN_GetContextRegval(ctxt, reg, reinterpret_cast<UINT8*>(xmmVals));
	   ss << REG_StringShort(reg) << ": ";
           //std::cout << REG_StringShort(reg) << ": ";
           for (int i = 0; i < 4; ++i) {
               //std::cout << xmmVals[i] << " ";
	       ss << xmmVals[i] << " ";
           }
           //std::cout << std::endl;
	   ss << std::endl;
       }
       ss << "----------------------------" << std::endl;

       printBuffer.push_back(ss.str());
       // If the buffer reaches the specified size, print and clear it
       if (printBuffer.size() >= PRINT_BUFFER_SIZE) {
           for (const auto& str : printBuffer) {
               std::cout << str;
           }
           printBuffer.clear();
           bufferPrintCount++; // Increment the counter when the buffer is printed
       }
   }
}

VOID Instruction(INS ins, VOID *v) {
    if (INS_IsValidForIpointAfter(ins)) {
	string dis = INS_Disassemble(ins);
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)printAfter, IARG_INST_PTR, IARG_CONTEXT, IARG_PTR, new string(dis), IARG_END);
    }
}

VOID Fini(INT32 code, VOID *v) {
    for (const auto& str : printBuffer) {
        std::cout << str;
    }
    std::cout << "Buffer printed " << bufferPrintCount << " times." << std::endl;
    std::cout << "Buffer size:" << PRINT_BUFFER_SIZE << std::endl;
    std::cout << "Program finished with code " << code << std::endl;
    std::cout << "Total instrumented instructions: " << instrumentedCount << std::endl;
}


INT32 Usage() {
    std::cerr << "This tool prints out register values for each operand of every Nth instruction, up to M times." << std::endl;
    std::cerr << "  N is specified by the -interval flag, default is 100" << std::endl;
    std::cerr << "  M is specified by the -maxcount flag, default is 10" << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();

    return 0;
}


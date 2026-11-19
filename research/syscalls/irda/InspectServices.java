// Run with -readOnly on a correctly based, big-endian 68000 ROM program.
// Argument: as3000, neo_3_01, or neo_3_21. Output is disposable research evidence.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;

public class InspectServices extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected ROM inventory name");
        long table;
        switch (args[0]) {
            case "as3000": table = 0x4eac1c; break;
            case "neo_3_01": table = 0x5f1546; break;
            case "neo_3_21": table = 0x44f15e; break;
            default: throw new IllegalArgumentException("Unknown ROM inventory name");
        }
        DecompInterface decompiler = new DecompInterface();
        try {
            decompiler.openProgram(currentProgram);
            for (int slot = 146; slot <= 173; ++slot) {
                if (monitor.isCancelled()) return;
                long target = Integer.toUnsignedLong(getInt(toAddr(table + slot * 4)));
                println(String.format("SLOT %d A%03X target=%08X", slot, slot * 4, target));
                if (target == 0) continue;
                var address = toAddr(target);
                disassemble(address);
                var function = getFunctionAt(address);
                if (function == null) function = createFunction(address, null);
                if (function == null) throw new IllegalStateException("No function at " + address);
                var instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
                while (instructions.hasNext()) {
                    var instruction = instructions.next();
                    println(instruction.getAddress() + " " + instruction);
                }
                var result = decompiler.decompileFunction(function, 20, monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                println(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
    }
}

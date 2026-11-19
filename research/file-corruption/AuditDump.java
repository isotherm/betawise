import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import java.io.PrintWriter;
public class AuditDump extends GhidraScript {
 public void run() throws Exception {
  String[] args=getScriptArgs();
  DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  try(PrintWriter out=new PrintWriter(args[0])) {
   for(int n=1;n<args.length;n++) {
    var a=toAddr(Long.parseLong(args[n],16)); disassemble(a);
    var f=getFunctionAt(a); if(f==null) f=createFunction(a,null);
    out.println("\n=== "+a+" ==="); if(f==null){out.println("NO FUNCTION");continue;}
    var it=currentProgram.getListing().getInstructions(f.getBody(),true);
    while(it.hasNext()){var i=it.next();out.println(i.getAddress()+" "+i);}
    var r=d.decompileFunction(f,30,monitor);
    if(r.decompileCompleted())out.println(r.getDecompiledFunction().getC());else out.println(r.getErrorMessage());
   }
  } finally {d.dispose();}
 }
}

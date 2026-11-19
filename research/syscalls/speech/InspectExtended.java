import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
public class InspectExtended extends GhidraScript {
 public void run() throws Exception {
  String[] args=getScriptArgs();
  if(args.length==0)args=new String[]{"434b14","435064","43f63c","434a74","434bd2","434ca6","434eea","435112","435948","4359e2","435a20","435b80","43f542","43ed3e","41c40e"};
  for(String arg:args){
   var a=toAddr(Long.parseLong(arg,16));
   disassemble(a);
   var f=getFunctionAt(a);
   if(f==null) f=createFunction(a,null);
   if(f==null) continue;
   var it=currentProgram.getListing().getInstructions(f.getBody(),true);
   while(it.hasNext()){var i=it.next(); println(i.getAddress()+" "+i);}
   var d=new DecompInterface(); d.openProgram(currentProgram);
   var r=d.decompileFunction(f,10,monitor);
   if(r.decompileCompleted()) println(r.getDecompiledFunction().getC());
   d.dispose();
  }
 }
}

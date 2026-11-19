import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
public class InspectNeoTail extends GhidraScript {
 public void run() throws Exception {
  String[] args=getScriptArgs();
  if(args.length==0)args=new String[]{"43ed3e","43ecee","43daca","43dad2","43dae2","43dafc","43efe8","43f29a","43f508","43f0d8","433c18","433de0","433e0a","433e34","433e5e","433f60","433fe2","434048","43409c","4343a2","4341fe","43420a","434218","434230","43425c","4342d4","433b86","433dae","434348","4347d0","43430e","43467c","43490c","43496a","442f86","443052","44313a","443214","4432b0","44331c","434980"};
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

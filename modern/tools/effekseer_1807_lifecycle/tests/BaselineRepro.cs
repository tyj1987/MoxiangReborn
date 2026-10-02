using System;
using Effekseer;
using Effekseer.Internal;
class BaselineRepro {
 static int failures;
 static void Test(string name,Action test){try{test();Console.WriteLine("PASS "+name);}catch(Exception e){failures++;Console.WriteLine("FAIL "+name+": "+e.GetType().Name+" "+e.Message);}}
 static int Main(){
  Test("repeat disable",()=>{var p=new EffekseerSoundPlayer();p.OnEnable();p.OnDisable();p.OnDisable();p.Dispose();});
  Test("callback after dispose",()=>{var p=new EffekseerSoundPlayer();p.OnEnable();var callback=Plugin.StopAll;p.Dispose();callback();});
  Test("stale owner dispose",()=>{var old=new EffekseerSoundPlayer();old.OnEnable();old.OnDisable();var next=new EffekseerSoundPlayer();next.OnEnable();old.Dispose();if(!ReferenceEquals(EffekseerSoundPlayer.Instance,next))throw new Exception("active owner cleared by old Dispose");next.Dispose();});
  Console.WriteLine(failures+" baseline failures");return failures==0?0:1;
 }
}

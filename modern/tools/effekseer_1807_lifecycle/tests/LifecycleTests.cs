using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using Effekseer;
using Effekseer.Internal;
class LifecycleTests {
 static int passed;
 static void Assert(bool condition,string message){if(!condition)throw new Exception(message);}
 static void Call(object instance,string name){try{instance.GetType().GetMethod(name,BindingFlags.Instance|BindingFlags.NonPublic).Invoke(instance,null);}catch(TargetInvocationException e){throw e.InnerException;}}
 static void Throws<T>(Action action) where T:Exception {try{action();}catch(T){return;}throw new Exception("Expected "+typeof(T).Name);}
 static List<Action> Events(EffekseerSoundPlayer player){return (List<Action>)typeof(EffekseerSoundPlayer).GetField("events",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(player);}
 static void Queue(Action<EffekseerSoundPlayer> action){typeof(EffekseerSoundPlayer).GetMethod("Enqueue",BindingFlags.Static|BindingFlags.NonPublic).Invoke(null,new object[]{action});}
 static void Worker(Action action){Exception error=null;var worker=new Thread(()=>{try{action();}catch(Exception e){error=e;}});worker.IsBackground=true;worker.Start();Assert(worker.Join(2000),"Native callback blocked: managed gate held across native call");if(error!=null)throw error;}
 static void Reset(){Plugin.FailRegister=Plugin.FailUnregister=false;Plugin.DuringRegister=Plugin.DuringUnregister=null;if(EffekseerSoundPlayer.Instance!=null)EffekseerSoundPlayer.Instance.Dispose();Plugin.Registers=Plugin.Unregisters=0;EffekseerSettings.Fail=false;EffekseerSystem.FailInit=false;EffekseerSystem.Instance=null;EffekseerSystem.Terms=EffekseerSystem.Enables=EffekseerSystem.Disables=0;}
 static void Test(string name,Action test){Reset();test();Reset();passed++;Console.WriteLine("PASS "+name);}
 static void Main(){
  Test("normal queue executes only in Update",()=>{using(var p=new EffekseerSoundPlayer()){p.OnEnable();int ran=0;Queue(owner=>{Assert(ReferenceEquals(owner,p),"wrong owner");ran++;});Assert(ran==0,"callback executed off-thread");p.Update();Assert(ran==1,"work lost");}});
  Test("actual sound play and stop stay queued and audible in managed simulation",()=>{using(var p=new EffekseerSoundPlayer()){
   var child=new EffekseerSoundInstance();Call(child,"Awake");
   ((List<EffekseerSoundInstance>)typeof(EffekseerSoundPlayer).GetField("childInstances",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(p)).Add(child);
   var audio=(UnityEngine.AudioSource)typeof(EffekseerSoundInstance).GetField("audio",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(child);
   var handle=GCHandle.Alloc(new EffekseerSoundResource{clip=new UnityEngine.AudioClip()});
   try{p.OnEnable();Plugin.Play(new IntPtr(7),GCHandle.ToIntPtr(handle),1,0,0,false,0,0,0,1);Assert(!audio.isPlaying,"Play bypassed queue");p.Update();Assert(audio.isPlaying,"Play feedback swallowed");Call(child,"Update");Assert(Plugin.Check(new IntPtr(7)),"playing query lost");Plugin.Stop(new IntPtr(7));Assert(audio.isPlaying,"Stop bypassed queue");p.Update();Assert(!audio.isPlaying,"Stop lost");}
   finally{handle.Free();}
  }});
  Test("repeat disable and dispose are idempotent",()=>{var p=new EffekseerSoundPlayer();p.OnEnable();p.OnDisable();p.OnDisable();p.Update();p.Dispose();p.Dispose();Assert(Plugin.Registers==1&&Plugin.Unregisters==1,"duplicate native registration/cleanup");});
  Test("disable before enable",()=>{using(var p=new EffekseerSoundPlayer()){p.OnDisable();Assert(Plugin.Unregisters==0,"uninitialized native cleanup");}});
  Test("in-flight callback during native register and unregister",()=>{var p=new EffekseerSoundPlayer();Plugin.DuringRegister=()=>Worker(()=>Plugin.StopAll());p.OnEnable();var late=Plugin.StopAll;Plugin.DuringUnregister=()=>Worker(()=>{late();Assert(!Plugin.Check(IntPtr.Zero),"disabled check returned true");});p.OnDisable();Assert(Events(p).Count==0,"shutdown accepted queued work");p.Dispose();});
  Test("dispose followed by all native callback shapes",()=>{var p=new EffekseerSoundPlayer();p.OnEnable();var play=Plugin.Play;var stop=Plugin.Stop;var pause=Plugin.Pause;var all=Plugin.StopAll;var check=Plugin.Check;p.Dispose();Worker(()=>{all();stop(IntPtr.Zero);pause(IntPtr.Zero,true);play(IntPtr.Zero,IntPtr.Zero,1,0,0,false,0,0,0,1);Assert(!check(IntPtr.Zero),"disposed check");});Assert(Events(p).Count==0,"disposed queue");Throws<ObjectDisposedException>(()=>p.OnEnable());});
  Test("old detached queue rejected after re-enable",()=>{using(var p=new EffekseerSoundPlayer()){p.OnEnable();int ran=0;Queue(owner=>ran++);var old=Events(p).ToArray();p.OnDisable();p.OnEnable();foreach(var action in old)action();Assert(ran==0,"old generation executed");Queue(owner=>ran++);p.Update();Assert(ran==1,"new generation lost");}});
  Test("callback already waiting on gate cannot enter a new generation",()=>{using(var p=new EffekseerSoundPlayer()){
   p.OnEnable();int ran=0;Exception error=null;
   var gate=typeof(EffekseerSoundPlayer).GetField("CallbackGate",BindingFlags.Static|BindingFlags.NonPublic).GetValue(null);
   var worker=new Thread(()=>{try{Queue(owner=>ran++);}catch(Exception e){error=e;}});worker.IsBackground=true;
   lock(gate){
    worker.Start();var until=DateTime.UtcNow.AddSeconds(2);
    while((worker.ThreadState&ThreadState.WaitSleepJoin)==0 && DateTime.UtcNow<until)Thread.Yield();
    Assert((worker.ThreadState&ThreadState.WaitSleepJoin)!=0,"callback did not reach gate");
    p.OnDisable();p.OnEnable();
   }
   Assert(worker.Join(2000),"waiting callback did not finish");if(error!=null)throw error;
   p.Update();Assert(ran==0&&Events(p).Count==0,"in-flight old generation accepted");
  }});
  Test("stale owner cannot clear replacement",()=>{var old=new EffekseerSoundPlayer();old.OnEnable();old.OnDisable();using(var next=new EffekseerSoundPlayer()){next.OnEnable();int before=Plugin.Unregisters;old.Dispose();Assert(ReferenceEquals(EffekseerSoundPlayer.Instance,next)&&Plugin.Unregisters==before,"stale dispose cleared owner");}});
  Test("duplicate owner rejected",()=>{using(var p=new EffekseerSoundPlayer()){p.OnEnable();using(var other=new EffekseerSoundPlayer()){Throws<InvalidOperationException>(()=>other.OnEnable());Assert(ReferenceEquals(EffekseerSoundPlayer.Instance,p),"duplicate owner replaced active one");}}});
  Test("partial native registration error surfaces and cleans",()=>{var p=new EffekseerSoundPlayer();Plugin.FailRegister=true;Throws<InvalidOperationException>(()=>p.OnEnable());Assert(EffekseerSoundPlayer.Instance==null&&Plugin.Unregisters==1,"partial registration not cleaned");Plugin.FailRegister=false;p.OnEnable();p.Dispose();});
  Test("failed unregister reserves owner until retry",()=>{var p=new EffekseerSoundPlayer();p.OnEnable();Plugin.FailUnregister=true;Throws<InvalidOperationException>(()=>p.OnDisable());Assert(!EffekseerSoundPlayer.IsValid&&ReferenceEquals(EffekseerSoundPlayer.Instance,p),"failed cleanup lost reservation");using(var other=new EffekseerSoundPlayer()){Throws<InvalidOperationException>(()=>other.OnEnable());}Plugin.FailUnregister=false;p.OnDisable();p.Dispose();});
  Test("off-thread lifecycle rejected",()=>{using(var p=new EffekseerSoundPlayer()){p.OnEnable();Worker(()=>Throws<InvalidOperationException>(()=>p.OnDisable()));Assert(EffekseerSoundPlayer.IsValid,"thread error disabled owner");}});
  Test("Runtime plugin initialization failure",()=>{var r=new EffekseerRuntime();EffekseerSystem.FailInit=true;Throws<InvalidOperationException>(()=>Call(r,"Awake"));Call(r,"OnEnable");Call(r,"OnDisable");Call(r,"OnDestroy");Assert(EffekseerSystem.Terms==0&&Plugin.Registers==0,"uninitialized plugin touched");});
  Test("Runtime sound initialization failure",()=>{var r=new EffekseerRuntime();EffekseerSettings.Fail=true;Throws<InvalidOperationException>(()=>Call(r,"Awake"));Call(r,"OnEnable");Call(r,"OnDisable");Call(r,"OnDestroy");Call(r,"OnDestroy");Assert(EffekseerSystem.Terms==1&&EffekseerSystem.Disables==0&&Plugin.Registers==0,"partial init cleanup count");});
  Test("Runtime normal disable reenable destroy",()=>{var r=new EffekseerRuntime();Call(r,"Awake");Call(r,"OnEnable");Call(r,"OnDisable");Call(r,"OnDisable");Call(r,"OnEnable");Call(r,"OnDestroy");Call(r,"OnDestroy");Assert(EffekseerSystem.Enables==2&&EffekseerSystem.Disables==2&&EffekseerSystem.Terms==1&&Plugin.Registers==2&&Plugin.Unregisters==2,"runtime lifecycle counts");});
  Console.WriteLine(passed+" lifecycle tests passed (managed stubs; not Unity/native acceptance)");
 }
}

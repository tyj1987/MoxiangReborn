// Test doubles only. Compile the actual patched vendor pair with these instead
// of Unity/native libraries. This does not validate the native ABI or Play mode.
using System;
using System.Collections.Generic;
namespace UnityEngine {
 public class SerializeField : Attribute {}
 public class RuntimeInitializeOnLoadMethodAttribute : Attribute {public RuntimeInitializeOnLoadMethodAttribute(RuntimeInitializeLoadType t){}}
 public enum RuntimeInitializeLoadType {BeforeSceneLoad}
 public class Object {public static implicit operator bool(Object x){return x!=null;} public static void DontDestroyOnLoad(Object x){}}
 public class Transform {public Transform parent;public Vector3 position;}
 public class GameObject : Object {public string name;public Transform transform=new Transform();public GameObject(){}public GameObject(string n){name=n;}public T AddComponent<T>() where T:new(){return new T();}}
 public class MonoBehaviour : Object {public GameObject gameObject=new GameObject();public Transform transform=new Transform();}
 public class AudioClip : Object {}
 public class AudioSource : Object {public bool playOnAwake,isPlaying;public float spatialBlend,volume,pitch,panStereo,minDistance,maxDistance;public AudioClip clip;public void Play(){isPlaying=true;}public void Stop(){isPlaying=false;}public void Pause(bool ignored=false){}public void UnPause(){}}
 public static class Application {public static bool isPlaying=true;}
 public static class Time {public static float time,deltaTime,unscaledDeltaTime;}
 public static class Mathf {public static float Max(float x,float y){return Math.Max(x,y);}public static float Pow(float x,float y){return (float)Math.Pow(x,y);}}
 public struct Vector3 {public Vector3(float x,float y,float z){}}
}
namespace AOT {public class MonoPInvokeCallbackAttribute:Attribute {public MonoPInvokeCallbackAttribute(Type t){}}}
namespace Effekseer {
 public class EffekseerSettings {public int soundInstances;public static bool Fail;static EffekseerSettings settings=new EffekseerSettings();public static EffekseerSettings Instance {get {if(Fail)throw new InvalidOperationException("sound init failed");return settings;}}}
 public class EffekseerEffectAsset {public static Dictionary<int,WeakReference> enabledAssets=new Dictionary<int,WeakReference>();}
 public class EffekseerSystem {
  public static EffekseerSystem Instance;public static bool FailInit,FailDisable,FailAfterDisable,FailTerm,FailAfterTerm;public static int Terms,TermAttempts,Enables,Disables,DisableAttempts,Updates;
  public void InitPlugin(){if(FailInit)throw new InvalidOperationException("plugin init failed");Instance=this;}
  public void TermPlugin(){TermAttempts++;if(FailTerm)throw new InvalidOperationException("termination failed before native");Terms++;Instance=null;if(FailAfterTerm)throw new InvalidOperationException("termination failed after native");}public void OnEnable(){Enables++;}public void OnDisable(){DisableAttempts++;if(FailDisable)throw new InvalidOperationException("renderer cleanup failed before release");Disables++;if(FailAfterDisable)throw new InvalidOperationException("renderer cleanup failed after release");}
  public void Update(float a,float b){Updates++;}public void LoadEffect(EffekseerEffectAsset x){}public static IntPtr GetCachedSound(IntPtr x){return x;}
 }
 public static class Plugin {
  public delegate void EffekseerSoundPlayerPlay(IntPtr tag,IntPtr data,float volume,float pan,float pitch,bool mode,float x,float y,float z,float distance);
  public delegate void EffekseerSoundPlayerStopTag(IntPtr tag);
  public delegate void EffekseerSoundPlayerPauseTag(IntPtr tag,bool pause);
  public delegate bool EffekseerSoundPlayerCheckPlayingTag(IntPtr tag);
  public delegate void EffekseerSoundPlayerStopAll();
  public static EffekseerSoundPlayerStopTag Stop;public static EffekseerSoundPlayerPauseTag Pause;public static EffekseerSoundPlayerPlay Play;public static EffekseerSoundPlayerStopAll StopAll;public static EffekseerSoundPlayerCheckPlayingTag Check;
  public static int Registers,Unregisters,NetworkUpdates;public static bool FailRegister,FailUnregister;public static Action DuringRegister,DuringUnregister;
  public static void EffekseerSetSoundPlayerEvent(EffekseerSoundPlayerPlay play,EffekseerSoundPlayerStopTag stop,EffekseerSoundPlayerPauseTag pause,EffekseerSoundPlayerCheckPlayingTag check,EffekseerSoundPlayerStopAll all){
   if(play!=null){Registers++;Play=play;Stop=stop;Pause=pause;StopAll=all;Check=check;if(DuringRegister!=null)DuringRegister();if(FailRegister)throw new InvalidOperationException("registration failed");}
   else {Unregisters++;if(DuringUnregister!=null)DuringUnregister();if(FailUnregister)throw new InvalidOperationException("unregistration failed");Play=null;Stop=null;Pause=null;StopAll=null;Check=null;}
  }
  public static void UpdateNetwork(){NetworkUpdates++;}public static int Effekseer_Manager_GetEffectHandles(int[] x,int n){return 0;}public static IntPtr Effekseer_Manager_GetName(int x){return IntPtr.Zero;}public static int EffekseerGetInstanceCount(int x){return 0;}
 }
}

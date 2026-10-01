using UnityEngine;
namespace Moxiang {
public enum WuguanTrainingPointKind { Instructor, Quest, Entrance, BackyardExit, WeaponRack, Dummy }
public sealed class WuguanTrainingPoint : MonoBehaviour {
 public WuguanTrainingPointKind kind;
 public int index;
 public bool IsInteractive => kind==WuguanTrainingPointKind.Instructor || kind==WuguanTrainingPointKind.Quest || kind==WuguanTrainingPointKind.WeaponRack || kind==WuguanTrainingPointKind.Dummy;
}}

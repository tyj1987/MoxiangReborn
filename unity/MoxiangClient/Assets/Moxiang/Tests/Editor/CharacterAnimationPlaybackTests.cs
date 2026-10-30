using System.Collections;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Moxiang.Tests
{
    public sealed class CharacterAnimationPlaybackTests
    {
        GameObject owner;
        [UnitySetUp] public IEnumerator EnterRuntime() { yield return new EnterPlayMode(); }
        [UnityTearDown] public IEnumerator LeaveRuntime()
        {
            if(owner!=null)Object.Destroy(owner);
            yield return null;
            yield return new ExitPlayMode();
        }
        [UnityTest]
        public IEnumerator ContinuousDriverAnimatesAndDisableUnsubscribes()
        {
            owner=new GameObject("CharacterPlaybackTest");
            var driver=owner.AddComponent<LegacyAnimationDriver>();
            var actor=new GameObject("Actor");actor.transform.SetParent(owner.transform);actor.SetActive(false);
            AnimatedModelPart Part(string name) => Object.Instantiate(AssetDatabase.LoadAssetAtPath<GameObject>(
                "Assets/Moxiang/Derived/Man/"+name+".mxhmodel"),actor.transform).GetComponent<AnimatedModelPart>();
            var body=Part("m_body01"); var face=Part("m_face01");
            var animation=actor.AddComponent<CharacterAnimation>();
            animation.skinnedParts=new[] {body,Part("m_hair01"),Part("m_hand01"),Part("m_shoes01")};
            animation.face=face;animation.headSkeleton=body;animation.driver=driver;
            animation.motions=new[] {new CharacterMotion {motionNumber=1,source=AssetDatabase.LoadAssetAtPath<ImportedMotion>(
                "Assets/Moxiang/Derived/Man/Motions/m001.mxhmotion")}};
            actor.SetActive(true);
            var mesh=body.GetComponentInChildren<MeshFilter>().sharedMesh;
            var initial=mesh.vertices;
            int callbacks=0;driver.FrameAdvanced+=increment=>++callbacks;
            yield return new WaitForSecondsRealtime(0.2f);
            Assert.That(callbacks,Is.GreaterThan(0),"Scene clock did not execute Update.");
            Assert.That(animation.CurrentFrame,Is.GreaterThan(0));
            bool moved=false;var current=mesh.vertices;
            for(int i=0;i<current.Length;++i)if(Vector3.Distance(initial[i],current[i])>1e-7f)moved=true;
            Assert.That(moved,Is.True,"Runtime mesh remained in its initial pose.");
            actor.SetActive(false);uint pausedFrame=animation.CurrentFrame;
            int previousCallbacks=callbacks;
            yield return new WaitForSecondsRealtime(0.15f);
            Assert.That(callbacks,Is.GreaterThan(previousCallbacks));
            Assert.That(animation.CurrentFrame,Is.EqualTo(pausedFrame));
            Assert.That(mesh==null,Is.True,"Disabled actor retained its owned mesh.");
            actor.SetActive(true);
            var replacement=body.GetComponentInChildren<MeshFilter>().sharedMesh;
            Assert.That(replacement,Is.Not.Null);
            yield return new WaitForSecondsRealtime(0.15f);
            Assert.That(animation.CurrentFrame,Is.Not.EqualTo(pausedFrame));
            Object.Destroy(actor);yield return null;
            Assert.That(replacement==null,Is.True,"Destroyed actor retained its owned mesh.");
            LogAssert.NoUnexpectedReceived();
        }
    }
}

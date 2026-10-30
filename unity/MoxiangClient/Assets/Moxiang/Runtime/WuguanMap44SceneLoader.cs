using System;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Moxiang
{
    /// <summary>Owns only the reviewed additive Map44 scene; generations cancel stale completion.</summary>
    public sealed class WuguanMap44SceneLoader : MonoBehaviour
    {
        public const string ScenePath="Assets/Moxiang/Scenes/Map44_Wuguan.unity";
        public bool HasOwnedScene => owned.IsValid()&&owned.isLoaded;
        private Scene owned;
        private AsyncOperation flight,unload;
        private int revision;
        public void Begin(Action<WuguanMap44Content> ready,Action<string> failed)
        {
            Clear();int token=revision;
            QueueStart(token,ready,failed);
        }
        private void QueueStart(int token,Action<WuguanMap44Content> ready,Action<string> failed)
        {
            if(!this||token!=revision)return;
            if(flight!=null&&!flight.isDone){flight.completed+=_=>QueueStart(token,ready,failed);return;}
            if(unload!=null&&!unload.isDone){unload.completed+=_=>QueueStart(token,ready,failed);return;}
            if(!Application.isPlaying){failed("Map44 streamed content requires a running Player.");return;}
            if(SceneManager.GetSceneByPath(ScenePath).isLoaded){failed("Map44 scene already belongs to another owner.");return;}
            try
            {
                flight=SceneManager.LoadSceneAsync(ScenePath,LoadSceneMode.Additive);
                if(flight==null){failed("Map44 scene is not in the build catalog.");return;}
                flight.completed+=_=>Complete(token,ready,failed);
            }
            catch(Exception ex){failed(ex.Message);}
        }
        private void Complete(int token,Action<WuguanMap44Content> ready,Action<string> failed)
        {
            flight=null;var scene=SceneManager.GetSceneByPath(ScenePath);
            if(!scene.IsValid()||!scene.isLoaded){if(this&&token==revision)failed("Map44 load returned no scene.");return;}
            if(!this||token!=revision){UnloadOwned(scene);return;}
            try
            {
                WuguanMap44Content content=null;
                foreach(var root in scene.GetRootGameObjects())foreach(var found in root.GetComponentsInChildren<WuguanMap44Content>(true))
                {if(content)throw new InvalidOperationException("Duplicate Map44 content root.");content=found;}
                if(!content)throw new InvalidOperationException("Map44 content descriptor missing.");
                content.Validate();owned=scene;ready(content);
            }
            catch(Exception ex){UnloadOwned(scene);failed(ex.Message);}
        }
        private void UnloadOwned(Scene scene)
        {
            foreach(var root in scene.GetRootGameObjects())root.SetActive(false);
            unload=SceneManager.UnloadSceneAsync(scene);
        }
        public void Clear()
        {
            revision++;
            if(HasOwnedScene)UnloadOwned(owned);
            owned=default;
        }
        private void OnDisable(){Clear();}
    }
}

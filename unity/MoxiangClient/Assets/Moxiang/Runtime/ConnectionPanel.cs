using System;
using TMPro;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Development connection UI backed exclusively by native server snapshots.</summary>
    public sealed class ConnectionPanel : MonoBehaviour
    {
        public TMP_InputField host, port, user, password;
        public TMP_Text status;
        public UnityEngine.UI.Button connect, disconnect, presentRevive, loginRevive;
        public UnityEngine.UI.Button[] characters;
        public TMP_Text[] characterLabels;
        public MapVisualController mapVisuals;
        public bool MapPresentationReady => mapVisuals == null || mapVisuals.IsReady;
        private NativeClient client;
        private CoreSnapshot observed;
        private ulong sequence;
        private string failure;
        public CoreSnapshot Observed => observed;
        public ShopCatalog Shop { get; } = new ShopCatalog();
        public InventoryState Inventory { get; } = new InventoryState();
        public ShopAppearanceState ShopAppearance { get; } = new ShopAppearanceState();
        public ShopItemNoticeState ShopItemNotice { get; } = new ShopItemNoticeState();
        public LocalAppearanceState Appearance { get; } = new LocalAppearanceState();
        public CoreResult BuyOffer(int index, ushort quantity)
        {
            if (!Shop.Ready || index < 0 || index >= Shop.Offers.Count) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitBuy(Shop.Offers[index].ItemId, quantity, observed);
            failure = result == CoreResult.Ok ? "Purchase: awaiting server response" : "Purchase: " + result;
            return result;
        }
        public CoreResult UseInventorySlot(ushort position)
        {
            if (!Inventory.Ready || position >= InventoryState.SlotCount) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitUseItem(position, observed);
            failure = result == CoreResult.Ok ? "Item: awaiting server response" : "Item use: " + result;
            return result;
        }
        public CoreResult MoveInventoryItem(ushort source, ushort target)
        {
            if (!Inventory.Ready || source >= 90 || target >= 90 || source == target) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitMoveItem(source, target, observed);
            failure = result == CoreResult.Ok ? "Equipment: awaiting server response" : "Equipment move: " + result;
            return result;
        }
        public CoreResult SellInventorySlot(ushort position, ushort quantity)
        {
            if (!Inventory.Ready || !Shop.Ready || position >= InventoryState.CarriedSlotCount ||
                quantity == 0 || Shop.NpcId == 0 || Shop.NpcId > ushort.MaxValue) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitSell(position, quantity, (ushort)Shop.NpcId, observed);
            failure = result == CoreResult.Ok ? "Sale: awaiting server response" : "Sale: " + result;
            return result;
        }
        public CoreResult DiscardInventorySlot(ushort position)
        {
            if (!Inventory.Ready || position >= InventoryState.CarriedSlotCount) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitDiscardItem(position, observed);
            failure = result == CoreResult.Ok ? "Discard: awaiting server response" : "Discard: " + result;
            return result;
        }
        public event Action<CoreEvent> CoreEventReceived;
        public CoreResult Move(ushort x, ushort z, bool stop)
        {
            if (!MapPresentationReady) return CoreResult.NotReady;
            return client == null ? CoreResult.NotReady : client.Move(x, z, stop, observed);
        }
        public CoreResult UseSkill(uint skillId, uint targetObjectId, float targetX, float targetZ)
        {
            if (!MapPresentationReady) return CoreResult.NotReady;
            return client == null ? CoreResult.NotReady : client.SubmitSkill(skillId, targetObjectId, targetX, targetZ, observed);
        }
        public CoreResult Pickup(uint dropObjectId)
        {
            if (!MapPresentationReady) return CoreResult.NotReady;
            return client == null ? CoreResult.NotReady : client.SubmitPickup(dropObjectId, observed);
        }

        public CoreResult InteractWithNpc(uint npcObjectId)
        {
            if (!MapPresentationReady) return CoreResult.NotReady;
            var result = client == null ? CoreResult.NotReady : client.SubmitNpcInteraction(npcObjectId, observed);
            if (result == CoreResult.Ok) Shop.Clear();
            failure = result == CoreResult.Ok ? "NPC: awaiting server response" : "NPC interaction: " + result;
            return result;
        }
        public CoreResult Quest(ushort questId, byte protocol)
        {
            if (questId == 0 || (protocol != 9 && protocol != 12)) return CoreResult.InvalidArgument;
            return client == null ? CoreResult.NotReady : client.SubmitQuest(questId, protocol, observed);
        }
        private void OnPresentReviveClicked() { RequestPresentRevive(); }
        private void OnLoginReviveClicked() { RequestLoginRevive(); }

        public CoreResult RequestLoginRevive()
        {
            if (client == null || observed.state != CoreState.InGame || observed.game.life != 0)
                return CoreResult.WrongState;
            var result = client.SubmitLoginRevive(observed);
            failure = result == CoreResult.Ok ? "登录点复活：等待服务器结算。" : "登录点复活：" + result;
            return result;
        }

        public CoreResult RequestPresentRevive()
        {
            if (client == null || observed.state != CoreState.InGame || observed.game.life != 0)
                return CoreResult.WrongState;
            var result = client.SubmitPresentRevive(observed);
            failure = result == CoreResult.Ok ? "原地复活：等待服务器结算。" : "原地复活：" + result;
            return result;
        }
        public CoreResult QuestNpcTalk(ushort semanticNpcIndex, ushort questId)
        {
            if (!MapPresentationReady) return CoreResult.NotReady;
            return client == null ? CoreResult.NotReady :
                client.SubmitQuestNpcTalk(semanticNpcIndex, questId, observed);
        }
        public CoreResult Chat(string text)
        {
            return client == null ? CoreResult.NotReady : client.SubmitChat(text, observed);
        }
        public CoreResult CreateCharacterFromUi(string name, byte sex, byte hair, byte face, byte cloth, byte boots, byte weapon)
        {
            if (client == null) return CoreResult.NotReady;
            try
            {
                var result = client.CreateCharacter(name, sex, hair, face, cloth, boots, weapon, observed);
                failure = result == CoreResult.Ok ? null : "Create character: " + result;
                return result;
            }
            catch (Exception exception) { failure = exception.Message; return CoreResult.InvalidArgument; }
        }
        public void BeginDevelopmentProbe(string loginHost, string loginPort, string account, string secret)
        {
            if (!Debug.isDebugBuild) throw new InvalidOperationException("Development probe is disabled.");
            host.text = loginHost; port.text = loginPort; user.text = account; password.text = secret;
            Connect();
        }
        public void SelectFirstForDevelopmentProbe()
        {
            if (!Debug.isDebugBuild || observed.characters == null) return;
            for (int i = 0; i < observed.characters.Length; ++i)
                if (observed.characters[i].valid != 0) { Select(i); return; }
        }

        private UnityEngine.UI.Button CloneReviveButton(string name, string caption, float y)
        {
            var clone = Instantiate(disconnect.gameObject, disconnect.transform.parent);
            clone.name = name;
            var button = clone.GetComponent<UnityEngine.UI.Button>();
            button.onClick.RemoveAllListeners();
            var label = clone.GetComponentInChildren<TMP_Text>();
            if (label != null) label.text = caption;
            var rect = clone.GetComponent<RectTransform>();
            rect.anchoredPosition = new Vector2(rect.anchoredPosition.x, y);
            clone.SetActive(false);
            return button;
        }

        private void EnsureReviveButtons()
        {
            if (disconnect == null) return;
            if (presentRevive == null) presentRevive = CloneReviveButton("PresentRevive", "原地复活", -580f);
            if (loginRevive == null) loginRevive = CloneReviveButton("LoginRevive", "登录点复活", -616f);
        }

        private void OnEnable()
        {
            connect.onClick.AddListener(Connect);
            disconnect.onClick.AddListener(Disconnect);
            EnsureReviveButtons();
            if (presentRevive != null)
            {
                presentRevive.onClick.RemoveListener(OnPresentReviveClicked);
                presentRevive.onClick.AddListener(OnPresentReviveClicked);
            }
            if (loginRevive != null)
            {
                loginRevive.onClick.RemoveListener(OnLoginReviveClicked);
                loginRevive.onClick.AddListener(OnLoginReviveClicked);
            }
            for (int i = 0; i < characters.Length; ++i)
            {
                int slot = i;
                characters[i].onClick.AddListener(() => Select(slot));
            }
            try { client = new NativeClient(); }
            catch (Exception exception) { failure = exception.GetType().Name + ": " + exception.Message; }
            Refresh();
        }

        private void Update()
        {
            if (client == null) return;
            try
            {
                client.Tick();
                observed = client.Snapshot();
                if (mapVisuals != null) mapVisuals.Observe(observed);
                Shop.Observe(observed);
                Inventory.Observe(observed);
                ShopAppearance.Observe(observed);
                ShopItemNotice.Observe(observed);
                for (int i = 0; i < 256 && client.PollEvent(out CoreEvent item); ++i)
                {
                    if (item.sessionGeneration != observed.sessionGeneration || item.mapGeneration != observed.mapGeneration) continue;
                    if (item.sequence <= sequence) continue;
                    sequence = item.sequence;
                    if (item.result != CoreResult.Ok) failure = item.Text;
                    if (item.type == NativeClient.EventNpcResponse)
                        failure = item.result == CoreResult.Ok ? "NPC: interaction accepted" : "NPC: interaction rejected";
                    if (item.type == NativeClient.EventQuestNpcResponse)
                        failure = item.result == CoreResult.Ok
                            ? "任务对话已确认：任务 " + item.argument1
                            : "该 NPC 当前没有可推进的任务步骤";
                    if (item.type == NativeClient.EventMapChange)
                        failure = item.result == CoreResult.Ok ? "已进入地图 " + item.argument1 : "换图未完成，当前地图 " + item.argument1;
                    if (item.type == NativeClient.EventCharacterRevive && item.result == CoreResult.Ok &&
                        item.argument0 == observed.game.playerId)
                        failure = observed.game.life == 0 ? "复活位置已确认，正在同步生命状态。" : "复活状态已同步。";
                    Shop.Accept(item);
                    Inventory.Accept(item);
                    ShopAppearance.Accept(item);
                    if (ShopItemNotice.Accept(item)) failure = ShopItemNotice.Message;
                    if (item.type == NativeClient.EventBuyResponse) {
                        failure = item.result == CoreResult.Ok ? "Purchase accepted" : "Purchase rejected";
                        if (item.result == CoreResult.Ok) Shop.Clear();
                    }
                    if (item.type == NativeClient.EventItemUseResponse)
                        failure = item.result == CoreResult.Ok ? "Item used" : "Item use rejected";
                    if (item.type == NativeClient.EventItemMoveResponse)
                        failure = item.result == CoreResult.Ok ? "Equipment updated" : "Equipment move rejected";
                    if (item.type == NativeClient.EventSellResponse)
                        failure = item.result == CoreResult.Ok ? "Sale accepted" : "Sale rejected";
                    if (item.type == NativeClient.EventDiscardResponse)
                        failure = item.result == CoreResult.Ok ? "Item discarded" : "Discard rejected";
                    if (item.type == NativeClient.EventPickupConfirmed)
                        failure = item.result == CoreResult.Ok ? "拾取成功，背包正在同步。" : "拾取未成功，请检查距离、背包空间或物品是否仍可领取。";
                    CoreEventReceived?.Invoke(item);
                }
                Appearance.Observe(observed, Inventory);
                Refresh();
            }
            catch (Exception exception)
            {
                failure = exception.GetType().Name + ": " + exception.Message;
                Release();
                Refresh();
            }
        }

        private void Connect()
        {
            if (client == null) return;
            if (!ushort.TryParse(port.text, out ushort number) || number == 0) { failure = "Enter a valid login port."; Refresh(); return; }
            try
            {
                var catalogResult = client.LoadMapRoutes(System.IO.Path.Combine(
                    Application.streamingAssetsPath, "Gameplay", "MapChange.bin"));
                if (catalogResult != CoreResult.Ok) { failure = "地图传送配置不可用：" + catalogResult; return; }
                var result = client.Connect(host.text, number, user.text, password.text);
                failure = result == CoreResult.Ok ? null : "Connect: " + result;
                sequence = 0;
            }
            catch (Exception exception) { failure = exception.Message; }
            finally { password.text = string.Empty; }
            Refresh();
        }

        private void Disconnect()
        {
            if (client == null) return;
            var result = client.Disconnect();
            failure = result == CoreResult.Ok ? null : "Disconnect: " + result;
        }

        private void Select(int slot)
        {
            if (client == null || observed.state != CoreState.CharacterListReady || observed.characters == null ||
                slot >= observed.characters.Length || observed.characters[slot].valid == 0) return;
            var result = client.SelectCharacter(observed.characters[slot].characterId, 0, observed);
            if (result != CoreResult.Ok) failure = "Select: " + result;
        }

        private void Refresh()
        {
            status.text = string.IsNullOrEmpty(failure) ? (client == null ? "Native core unavailable" : observed.state.ToString()) : failure;
            if (client != null && observed.state == CoreState.InGame)
                status.text += "\nMap " + observed.game.mapNumber + " | Level " + observed.game.level +
                    " | HP " + observed.game.life + "/" + observed.game.maxLife;
            if (client != null && observed.state == CoreState.InGame)
                status.text += "\nPosition " + observed.game.positionX + ", " + observed.game.positionZ;
            var showRevive = client != null && observed.state == CoreState.InGame && observed.game.life == 0;
            if (presentRevive != null) presentRevive.gameObject.SetActive(showRevive);
            if (loginRevive != null) loginRevive.gameObject.SetActive(showRevive);
            if (mapVisuals != null && observed.state == CoreState.InGame && !mapVisuals.IsReady)
                status.text += "\n" + mapVisuals.Failure;
            connect.interactable = client != null && (observed.state == CoreState.Idle || observed.state == CoreState.Failed);
            disconnect.interactable = client != null && observed.state != CoreState.Idle;
            for (int i = 0; i < characters.Length; ++i)
            {
                bool valid = observed.characters != null && i < observed.characters.Length && observed.characters[i].valid != 0;
                characters[i].interactable = valid && observed.state == CoreState.CharacterListReady;
                characterLabels[i].text = valid ? observed.characters[i].DisplayName + "  Lv." + observed.characters[i].level : "Empty slot";
            }
        }

        private void Release()
        {
            if (mapVisuals != null) mapVisuals.Clear();
            Shop.Clear();
            Inventory.Clear();
            ShopAppearance.Clear();
            Appearance.Clear();
            ShopItemNotice.Clear();
            var owned = client;
            client = null;
            if (owned == null) return;
            try { owned.Dispose(); }
            catch (Exception exception) { Debug.LogError("Native core cleanup failed: " + exception.GetType().Name); }
        }

        private void OnDisable()
        {
            connect.onClick.RemoveListener(Connect);
            disconnect.onClick.RemoveListener(Disconnect);
            foreach (var button in characters) button.onClick.RemoveAllListeners();
            password.text = string.Empty;
            Release();
        }
        private void OnApplicationQuit() { Release(); }
    }
}

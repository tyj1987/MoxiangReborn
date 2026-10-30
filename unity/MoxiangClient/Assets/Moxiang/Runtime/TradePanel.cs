using System;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

namespace Moxiang
{
    [RequireComponent(typeof(CanvasGroup))]
    public sealed class TradePanel : MonoBehaviour
    {
        public ConnectionPanel connection;
        public TMP_Text heading, pageLabel;
        public TMP_InputField quantity;
        public UnityEngine.UI.Button mode, previous, next;
        public UnityEngine.UI.Button[] rows;
        public TMP_Text[] rowLabels;
        private int modeIndex;
        private int selectedMoveSource = -1;
        private int selectedDiscard = -1;
        private int page;
        private IReadOnlyList<ShopOffer> displayedOffers;
        private readonly List<UnityEngine.Events.UnityAction> rowActions = new List<UnityEngine.Events.UnityAction>();

        public static bool TryQuantity(string text, out ushort value) =>
            ushort.TryParse(text, out value) && value != 0;
        public static bool IsUsableInventorySlot(int position, InventoryItem item) =>
            position >= 0 && position < 80 && item.ItemId != 0;
        public static bool IsMoveSource(int position, InventoryItem item) =>
            position >= 0 && position < 90 && item.ItemId != 0;
        public static bool IsMoveTarget(int source, int target) =>
            source >= 0 && source < 90 && target >= 0 && target < 90 && source != target;

        private void OnEnable()
        {
            mode.onClick.AddListener(ToggleMode); previous.onClick.AddListener(Previous); next.onClick.AddListener(Next);
            for (int i = 0; i < rows.Length; ++i) {
                int row = i;
                UnityEngine.Events.UnityAction action = () => ActivateRow(row);
                rowActions.Add(action); rows[i].onClick.AddListener(action);
            }
            Refresh();
        }
        private void OnDisable()
        {
            mode.onClick.RemoveListener(ToggleMode); previous.onClick.RemoveListener(Previous); next.onClick.RemoveListener(Next);
            for (int i = 0; i < rowActions.Count; ++i) rows[i].onClick.RemoveListener(rowActions[i]);
            rowActions.Clear();
        }
        private void ToggleMode() { modeIndex = (modeIndex + 1) % 5; selectedMoveSource = -1; selectedDiscard = -1; page = 0; Refresh(); }
        private void Previous() { page = Math.Max(0, page - 1); Refresh(); }
        private void Next() { ++page; Refresh(); }
        private void Update() { Refresh(); }
        private void ActivateRow(int row)
        {
            if (connection == null) return;
            int index = page * rows.Length + row;
            if (modeIndex == 1) {
                if (connection.Inventory.Ready && index < connection.Inventory.Slots.Count &&
                    IsUsableInventorySlot(index, connection.Inventory.Slots[index]))
                    connection.UseInventorySlot((ushort)index);
                return;
            }
            if (modeIndex == 2) {
                if (!connection.Inventory.Ready || index >= connection.Inventory.Slots.Count || index >= 90) return;
                if (selectedMoveSource < 0) {
                    if (IsMoveSource(index, connection.Inventory.Slots[index])) selectedMoveSource = index;
                } else if (IsMoveTarget(selectedMoveSource, index)) {
                    if (connection.MoveInventoryItem((ushort)selectedMoveSource, (ushort)index) == CoreResult.Ok)
                        selectedMoveSource = -1;
                }
                return;
            }
            if (modeIndex == 3) {
                if (connection.Inventory.Ready && connection.Shop.Ready &&
                    index < connection.Inventory.Slots.Count &&
                    IsSellableInventorySlot(index, connection.Inventory.Slots[index]) &&
                    TryQuantity(quantity.text, out var sellCount))
                    connection.SellInventorySlot((ushort)index, sellCount);
                return;
            }
            if (modeIndex == 4) {
                if (!connection.Inventory.Ready || index >= connection.Inventory.Slots.Count ||
                    !IsDiscardableInventorySlot(index, connection.Inventory.Slots[index])) return;
                if (selectedDiscard != index) selectedDiscard = index;
                else if (connection.DiscardInventorySlot((ushort)index) == CoreResult.Ok) selectedDiscard = -1;
                return;
            }
            if (!ReferenceEquals(displayedOffers, connection.Shop.Offers) || !TryQuantity(quantity.text, out var count)) return;
            connection.BuyOffer(index, count);
        }
        private void Refresh()
        {
            if (connection == null || rows.Length == 0) return;
            bool inGame = connection.Observed.state == CoreState.InGame;
            var visibility = GetComponent<CanvasGroup>();
            if (visibility == null) visibility = gameObject.AddComponent<CanvasGroup>();
            visibility.alpha = inGame ? 1 : 0;
            visibility.interactable = inGame; visibility.blocksRaycasts = inGame;
            if (!ReferenceEquals(displayedOffers, connection.Shop.Offers)) { displayedOffers = connection.Shop.Offers; if (modeIndex == 0) page = 0; }
            bool inventoryMode = modeIndex != 0;
            bool ready = modeIndex == 3 ? connection.Inventory.Ready && connection.Shop.Ready :
                inventoryMode ? connection.Inventory.Ready : connection.Shop.Ready;
            int count = inventoryMode ? connection.Inventory.Slots.Count : connection.Shop.Offers.Count;
            int pages = Math.Max(1, (count + rows.Length - 1) / rows.Length);
            page = Math.Min(page, pages - 1);
            heading.text = modeIndex == 0 ? "Shop — click an offer to buy" :
                modeIndex == 1 ? "Inventory — click a carried item to use" :
                modeIndex == 2 ? (selectedMoveSource < 0 ? "Equipment — select a carried or worn item" :
                "Equipment — select target slot for " + selectedMoveSource) :
                modeIndex == 3 ? "Sell — click a carried item to sell" :
                selectedDiscard < 0 ? "Discard — select a carried item" :
                "Discard — click slot " + selectedDiscard + " again to confirm";
            pageLabel.text = ready ? (page + 1) + " / " + pages : "Waiting for server data";
            previous.interactable = page > 0; next.interactable = page + 1 < pages;
            quantity.interactable = modeIndex == 0 || modeIndex == 3;
            for (int i = 0; i < rows.Length; ++i) {
                int index = page * rows.Length + i;
                bool valid = ready && index < count;
                bool canActivate = false;
                if (valid && modeIndex == 1) canActivate = IsUsableInventorySlot(index, connection.Inventory.Slots[index]);
                if (valid && modeIndex == 2) canActivate = selectedMoveSource < 0
                    ? IsMoveSource(index, connection.Inventory.Slots[index]) : IsMoveTarget(selectedMoveSource, index);
                if (valid && modeIndex == 0) canActivate = TryQuantity(quantity.text, out _);
                if (valid && modeIndex == 3) canActivate = IsSellableInventorySlot(index, connection.Inventory.Slots[index]) &&
                    TryQuantity(quantity.text, out _);
                if (valid && modeIndex == 4) canActivate = IsDiscardableInventorySlot(index, connection.Inventory.Slots[index]);
                rows[i].interactable = inGame && canActivate;
                if (!valid) { rowLabels[i].text = "—"; continue; }
                if (inventoryMode) {
                    var item = connection.Inventory.Slots[index];
                    rowLabels[i].text = item.ItemId == 0 ? "Slot " + index + " · Empty" :
                        "Slot " + index + " · Item " + item.ItemId + " · " + item.ItemParameter;
                } else {
                    var offer = connection.Shop.Offers[index];
                    rowLabels[i].text = "Item " + offer.ItemId + " · " + offer.Price + " gold each";
                }
            }
        }

        public static bool IsSellableInventorySlot(int position, InventoryItem item) =>
            position >= 0 && position < InventoryState.CarriedSlotCount && item.ItemId != 0;
        public static bool IsDiscardableInventorySlot(int position, InventoryItem item) =>
            position >= 0 && position < InventoryState.CarriedSlotCount && item.ItemId != 0;
    }
}

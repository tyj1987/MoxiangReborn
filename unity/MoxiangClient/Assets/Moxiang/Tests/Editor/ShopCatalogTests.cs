using NUnit.Framework;

namespace Moxiang.Tests
{
    public class ShopCatalogTests
    {
        private static CoreSnapshot Snapshot(ulong map = 1) => new CoreSnapshot {
            state = CoreState.InGame, sessionGeneration = 1, mapGeneration = map };
        private static CoreEvent Chunk(uint total, uint offset, uint count)
        {
            var bytes = new byte[count * 6];
            for (int i = 0; i < count; ++i) {
                bytes[i * 6] = (byte)(offset + i + 1);
                for (int j = 2; j < 6; ++j) bytes[i * 6 + j] = 255;
            }
            return new CoreEvent { type = NativeClient.EventShopCatalog, state = CoreState.InGame,
                sessionGeneration = 1, mapGeneration = 1, argument0 = 77, argument1 = total,
                reserved0 = offset, text = bytes, textLength = (uint)bytes.Length };
        }

        [Test] public void PublishesOnlyCompleteCatalogAndRetainsUnsignedPrice()
        {
            var shop = new ShopCatalog(); shop.Observe(Snapshot());
            Assert.That(shop.Accept(Chunk(81, 0, 40)), Is.False);
            Assert.That(shop.Offers, Is.Empty);
            Assert.That(shop.Accept(Chunk(81, 40, 40)), Is.False);
            Assert.That(shop.Accept(Chunk(81, 80, 1)), Is.True);
            Assert.That(shop.Ready, Is.True);
            Assert.That(shop.Offers.Count, Is.EqualTo(81));
            Assert.That(shop.Offers[80].ItemId, Is.EqualTo(81));
            Assert.That(shop.Offers[80].Price, Is.EqualTo(uint.MaxValue));
        }

        [Test] public void OutOfOrderAndTruncatedChunksNeverPublish()
        {
            var shop = new ShopCatalog(); shop.Observe(Snapshot());
            shop.Accept(Chunk(81, 0, 40));
            Assert.That(shop.Accept(Chunk(81, 80, 1)), Is.False);
            Assert.That(shop.Ready, Is.False);
            Assert.That(shop.Accept(Chunk(81, 40, 40)), Is.False);
            var bad = Chunk(1, 0, 1); bad.text = new byte[5];
            Assert.That(shop.Accept(bad), Is.False);
            Assert.That(shop.Offers, Is.Empty);
        }

        [Test] public void MapChangeClearsPartialAndPublishedCatalogs()
        {
            var shop = new ShopCatalog(); shop.Observe(Snapshot());
            shop.Accept(Chunk(81, 0, 40)); shop.Observe(Snapshot(2));
            Assert.That(shop.Accept(Chunk(81, 40, 40)), Is.False);
            shop.Observe(Snapshot()); Assert.That(shop.Accept(Chunk(1, 0, 1)), Is.True);
            shop.Observe(Snapshot(2));
            Assert.That(shop.Ready, Is.False); Assert.That(shop.Offers, Is.Empty);
        }

        [Test] public void EmptyCatalogReplacesExistingOffers()
        {
            var shop = new ShopCatalog(); shop.Observe(Snapshot());
            shop.Accept(Chunk(1, 0, 1));
            var empty = Chunk(0, 0, 0); empty.argument0 = 0;
            Assert.That(shop.Accept(empty), Is.True);
            Assert.That(shop.Ready, Is.True); Assert.That(shop.Offers, Is.Empty);
        }
    }
}

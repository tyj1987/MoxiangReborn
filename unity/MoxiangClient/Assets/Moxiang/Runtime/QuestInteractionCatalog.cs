using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    [Serializable] public sealed class QuestInteractionSource { public string path; public string sha256; }
    [Serializable] public sealed class QuestInteractionSummary
    {
        public int interactionCount;
        public int unresolvedTextCount;
        public int unresolvedPresentationCount;
        public int recoveredPresentationCount;
    }
    [Serializable] public sealed class QuestInteractionOption
    {
        public uint textId;
        public string text;
        public uint linkType;
        public uint target;
    }
    [Serializable] public sealed class QuestInteractionEntry
    {
        public uint questId;
        public uint stage;
        public uint npcIndex;
        public uint mapId;
        public string name;
        public int x;
        public int z;
        public string title;
        public string titleSource;
        public string description;
        public uint pageId;
        public uint[] dialogueIds;
        public string dialogueRaw;
        public string dialogueText;
        public QuestInteractionOption[] options;
        public string pageSource;
        public string recoveryEvidence;
        public bool textResolved;
    }
    [Serializable] public sealed class QuestInteractionDocument
    {
        public int schemaVersion;
        public string textEncoding;
        public QuestInteractionSource[] sources;
        public QuestInteractionSummary summary;
        public QuestInteractionEntry[] entries;
    }

    /// <summary>Validated, read-only view of original QuestScript/QuestString interactions.</summary>
    public sealed class QuestInteractionCatalog
    {
        private readonly Dictionary<uint, List<QuestInteractionEntry>> byNpc = new();
        public int Count { get; private set; }
        public int UnresolvedCount { get; private set; }
        public int UnresolvedPresentationCount { get; private set; }
        public int RecoveredPresentationCount { get; private set; }

        public static QuestInteractionCatalog LoadDefault()
        {
            return Load(File.ReadAllText(Path.Combine(Application.streamingAssetsPath, "Gameplay", "QuestInteractions.json")));
        }

        public static QuestInteractionCatalog Load(string json)
        {
            var document = JsonUtility.FromJson<QuestInteractionDocument>(json);
            if (document == null || document.schemaVersion != 1) throw new InvalidDataException("Unsupported quest interaction schema.");
            if (document.sources == null || document.sources.Length == 0) throw new InvalidDataException("Quest interaction provenance is missing.");
            foreach (var source in document.sources)
                if (source == null || string.IsNullOrWhiteSpace(source.path) || source.sha256 == null || source.sha256.Length != 64)
                    throw new InvalidDataException("Quest interaction source hash is invalid.");
            if (document.entries == null) throw new InvalidDataException("Quest interaction entries are missing.");

            var result = new QuestInteractionCatalog();
            var keys = new HashSet<string>();
            foreach (var entry in document.entries)
            {
                if (entry == null || entry.questId == 0 || entry.npcIndex == 0) throw new InvalidDataException("Quest interaction identity is invalid.");
                var key = entry.npcIndex + ":" + entry.questId + ":" + entry.stage + ":" + entry.mapId + ":" + entry.x + ":" + entry.z;
                if (!keys.Add(key)) throw new InvalidDataException("Duplicate quest interaction key.");
                if (!result.byNpc.TryGetValue(entry.npcIndex, out var list)) result.byNpc.Add(entry.npcIndex, list = new List<QuestInteractionEntry>());
                list.Add(entry);
                if (!entry.textResolved) ++result.UnresolvedCount;
                var hasOptionText = false;
                if (entry.options != null) foreach (var option in entry.options)
                    hasOptionText |= option != null && !string.IsNullOrWhiteSpace(option.text);
                if (string.IsNullOrWhiteSpace(entry.dialogueText) && !hasOptionText) ++result.UnresolvedPresentationCount;
                if (entry.pageSource != null && entry.pageSource.StartsWith("Recovered", StringComparison.Ordinal)) ++result.RecoveredPresentationCount;
            }
            result.Count = document.entries.Length;
            if (document.summary == null || document.summary.interactionCount != result.Count ||
                document.summary.unresolvedTextCount != result.UnresolvedCount ||
                document.summary.unresolvedPresentationCount != result.UnresolvedPresentationCount ||
                document.summary.recoveredPresentationCount != result.RecoveredPresentationCount)
                throw new InvalidDataException("Quest interaction summary does not match entries.");
            return result;
        }

        public IReadOnlyList<QuestInteractionEntry> Find(uint npcIndex)
        {
            return byNpc.TryGetValue(npcIndex, out var entries) ? entries : Array.Empty<QuestInteractionEntry>();
        }
    }
}

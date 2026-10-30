#!/usr/bin/env python3
"""Export versioned, source-hashed quest/NPC interaction text for Unity."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def decode_mhfile(path: Path, encoding: str = "big5", errors: str = "strict") -> str:
    raw = path.read_bytes()
    if len(raw) < 13:
        raise ValueError(f"truncated MHFile: {path}")
    _, kind, size = struct.unpack_from("<III", raw)
    if 13 + size > len(raw):
        raise ValueError(f"declared payload exceeds file: {path}")
    payload = bytearray(raw[13:13 + size])
    for index in range(len(payload)):
        payload[index] = (payload[index] - index - (kind if kind and index % kind == 0 else 0)) & 0xFF
    return payload.decode(encoding, errors=errors)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def braced_blocks(text: str, header: re.Pattern[str]):
    for match in header.finditer(text):
        brace = text.find("{", match.end())
        if brace < 0:
            continue
        depth = 0
        for end in range(brace, len(text)):
            depth += text[end] == "{"
            depth -= text[end] == "}"
            if depth == 0:
                yield match, text[brace + 1:end]
                break


def clean_text(value: str) -> str:
    return "\n".join(line.strip() for line in value.splitlines() if line.strip())


def display_text(value: str) -> str:
    value = re.sub(r"\$C[SE]", "", value, flags=re.I).replace("^s", "\n")
    return clean_text(value)


def parse_strings(text: str) -> dict[tuple[int, int], tuple[str, str]]:
    result = {}
    header = re.compile(r"\$SUBQUESTSTR\s+(\d+)\s+(\d+)")
    for match, body in braced_blocks(text, header):
        title_match = re.search(r"#TITLE\s+([^\r\n]*)", body)
        desc_match = re.search(r"#DESC\s*\{(.*?)\}", body, re.S)
        title = clean_text(title_match.group(1)) if title_match else ""
        desc = clean_text(desc_match.group(1)) if desc_match else ""
        result[(int(match.group(1)), int(match.group(2)))] = (title, desc)
    return result


def parse_npcs(*texts: str) -> dict[int, list[dict]]:
    result: dict[int, list[dict]] = {}
    for text in texts:
        for line in text.splitlines():
            columns = line.strip().split("\t")
            if len(columns) < 6:
                columns = line.strip().split()
            if len(columns) < 6:
                continue
            try:
                map_id, npc_index = int(columns[0]), int(columns[3])
            except ValueError:
                continue
            value = {"mapId": map_id, "name": columns[2].strip(),
                     "x": int(columns[4]), "z": int(columns[5])}
            if value not in result.setdefault(npc_index, []):
                result[npc_index].append(value)
    return result


def parse_messages(text: str) -> dict[int, str]:
    return {int(match.group(1)): clean_text(body)
            for match, body in braced_blocks(text, re.compile(r"#Msg\s+(\d+)"))}


def parse_hypertext(text: str) -> dict[int, str]:
    result = {}
    for line in text.splitlines():
        columns = line.split("\t", 1)
        if len(columns) != 2:
            continue
        try:
            result[int(columns[0])] = columns[1].strip()
        except ValueError:
            pass
    return result


def parse_quest_link_titles(text: str, hypertext: dict[int, str]) -> dict[int, str]:
    result = {}
    for text_id, quest_id in re.findall(r"#HYPERLINK\s+(\d+)\s+4\s+(\d+)", text):
        title = hypertext.get(int(text_id), "").strip()
        if title:
            result.setdefault(int(quest_id), title)
    return result


def parse_npc_pages(text: str, messages: dict[int, str], hypertext: dict[int, str]) -> dict[tuple[int, int], dict]:
    result = {}
    npc_markers = list(re.finditer(r"(?m)^\s*\$NPC\s*$", text))
    for npc_index, marker in enumerate(npc_markers):
        npc_body = text[marker.end():npc_markers[npc_index + 1].start() if npc_index + 1 < len(npc_markers) else len(text)]
        npc_match = re.search(r"#NPCID\s+(\d+)", npc_body)
        if not npc_match:
            continue
        npc_id = int(npc_match.group(1))
        page_markers = list(re.finditer(r"(?m)^\s*\$PAGE\s*$", npc_body))
        for page_index, page_marker in enumerate(page_markers):
            page_body = npc_body[page_marker.end():page_markers[page_index + 1].start() if page_index + 1 < len(page_markers) else len(npc_body)]
            page_match = re.search(r"#PAGEINFO\s+(\d+)", page_body)
            if not page_match:
                continue
            page_id = int(page_match.group(1))
            dialogue_match = re.search(r"#(?:DIALOGUE|DAILOGUE)\s+([^\r\n]+)", page_body)
            dialogue_ids = [int(value) for value in re.findall(r"\d+", dialogue_match.group(1))] if dialogue_match else []
            raw_parts = [messages[value] for value in dialogue_ids if value in messages and messages[value]]
            options = []
            for link in re.finditer(r"#HYPERLINK\s+(\d+)\s+(\d+)\s+(\d+)", page_body):
                text_id, link_type, target = map(int, link.groups())
                options.append({"textId": text_id, "text": hypertext.get(text_id, ""),
                                "linkType": link_type, "target": target})
            result[(npc_id, page_id)] = {
                "dialogueIds": dialogue_ids,
                "dialogueRaw": "\n".join(raw_parts),
                "dialogueText": display_text("\n".join(raw_parts)),
                "options": options,
                "pageSource": "NpcScript",
                "recoveryEvidence": "",
            }
    return result


def apply_verified_page_recoveries(npc_pages: dict, messages: dict[int, str], hypertext: dict[int, str],
                                   source_hashes: tuple[str, str, str]) -> None:
    expected = ("93d964a93be393a8a83fb9a84d80883b27821a593d180a30b365516df1702a52",
                "1c443241334c73f3b028f979e7fb09ae8b48c7021ec0243437fe4e2405eb2d45",
                "29d8540ed9a90d2a6061807580a90c195faa27e93031bd8560740359ee37e3c5")
    if tuple(value.lower() for value in source_hashes) != expected:
        return
    # Npc_Script NPC264 ends at page15/message8362, while the paired message and
    # hyperlink tables retain quest152's contiguous page payload. Keep this
    # explicit and hash-gated so a different resource revision cannot inherit it.
    recoveries = {16: (8363, 480), 19: (8366, 481), 23: (8370, 482)}
    for page_id, (message_id, text_id) in recoveries.items():
        raw = messages.get(message_id, "")
        option = hypertext.get(text_id, "")
        if not raw or not option:
            continue
        npc_pages[(264, page_id)] = {
            "dialogueIds": [message_id], "dialogueRaw": raw,
            "dialogueText": display_text(raw),
            "options": [{"textId": text_id, "text": option, "linkType": 5, "target": 0}],
            "pageSource": "RecoveredContiguousNpcMessageSequence",
            "recoveryEvidence": "Npc264 page table ends at page15/message8362; paired messages8363-8370 and hyperlinks480-482 retain quest152 sequence",
        }


def parse_interactions(text: str, strings: dict, npcs: dict, npc_pages: dict,
                       quest_link_titles: dict[int, str]) -> list[dict]:
    entries = []
    seen = set()
    for quest_match, quest_body in braced_blocks(text, re.compile(r"\$QUEST\s+(\d+)")):
        quest_id = int(quest_match.group(1))
        for sub_match, sub_body in braced_blocks(quest_body, re.compile(r"\$SUBQUEST\s+(\d+)")):
            stage = int(sub_match.group(1))
            title, description = strings.get((quest_id, stage), strings.get((quest_id, 0), ("", "")))
            title_source = "QuestString"
            if not title and quest_id in quest_link_titles:
                title = quest_link_titles[quest_id]
                title_source = "NpcHyperTextQuestLink"
            page_by_npc = {int(value[0]): int(value[1])
                           for value in re.findall(r"@NPC\s+(\d+)\s+(\d+)\s+\d+", sub_body)}
            for talk in re.finditer(r"@TALKTONPCNPC\s+(\d+)\s+(\d+)|@TALKTONPC\s+(\d+)\s+(\d+)", sub_body):
                npc_index = int(talk.group(1) or talk.group(3))
                context = int(talk.group(2) or talk.group(4))
                if context != quest_id:
                    continue
                locations = npcs.get(npc_index, [{"mapId": 0, "name": "", "x": 0, "z": 0}])
                page_id = page_by_npc.get(npc_index, 0)
                page = npc_pages.get((npc_index, page_id), {"dialogueIds": [], "dialogueRaw": "", "dialogueText": "", "options": [],
                                                                  "pageSource": "Unresolved", "recoveryEvidence": ""})
                for location in locations:
                    key = (quest_id, stage, npc_index, location["mapId"], location["x"], location["z"])
                    if key in seen:
                        continue
                    seen.add(key)
                    entries.append({"questId": quest_id, "stage": stage, "npcIndex": npc_index,
                                    **location, "pageId": page_id, **page,
                                    "title": title, "titleSource": title_source,
                                    "description": description,
                                    "textResolved": bool(title)})
    return sorted(entries, key=lambda e: (e["npcIndex"], e["questId"], e["stage"], e["mapId"]))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--playdh", type=Path, default=Path("modern/data/PlayDH"))
    parser.add_argument("--output", type=Path,
                        default=Path("unity/MoxiangClient/Assets/StreamingAssets/Gameplay/QuestInteractions.json"))
    parser.add_argument("--strict", action="store_true", help="fail if any interaction has unresolved source text")
    args = parser.parse_args()
    base = args.playdh / "Resource"
    sources = [base / "QuestScript/QuestScript.bin", base / "QuestScript/QuestString.bin",
               base / "QuestScript/questnpclist.bin", base / "StaticNpc.bin",
               args.playdh / "Image/InterfaceScript/Npc_Script.bin",
               args.playdh / "Image/Npc_Msg.bin", args.playdh / "Image/Npc_HyperText.bin"]
    for source in sources:
        if not source.is_file():
            raise FileNotFoundError(source)
    strings = parse_strings(decode_mhfile(sources[1]))
    npcs = parse_npcs(decode_mhfile(sources[2], errors="replace"),
                      decode_mhfile(sources[3], errors="replace"))
    messages = parse_messages(decode_mhfile(sources[5], errors="replace"))
    hypertext = parse_hypertext(decode_mhfile(sources[6], errors="replace"))
    npc_script = decode_mhfile(sources[4], errors="replace")
    npc_pages = parse_npc_pages(npc_script, messages, hypertext)
    apply_verified_page_recoveries(npc_pages, messages, hypertext,
                                   (sha256(sources[4]), sha256(sources[5]), sha256(sources[6])))
    entries = parse_interactions(decode_mhfile(sources[0]), strings, npcs, npc_pages,
                                 parse_quest_link_titles(npc_script, hypertext))
    unresolved = sum(not e["textResolved"] for e in entries)
    unresolved_presentation = sum(not e["dialogueText"] and not any(option["text"] for option in e["options"])
                                  for e in entries)
    recovered_presentation = sum(e["pageSource"].startswith("Recovered") for e in entries)
    if not entries or (args.strict and (unresolved or unresolved_presentation)):
        raise ValueError(f"quest interaction export empty or has {unresolved} unresolved titles "
                         f"and {unresolved_presentation} unresolved presentations")
    document = {"schemaVersion": 1, "textEncoding": "big5",
                "sources": [{"path": source.relative_to(args.playdh).as_posix(), "sha256": sha256(source)} for source in sources],
                "summary": {"interactionCount": len(entries), "unresolvedTextCount": unresolved,
                            "unresolvedPresentationCount": unresolved_presentation,
                            "recoveredPresentationCount": recovered_presentation},
                "entries": entries}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"exported {len(entries)} interactions ({unresolved} unresolved) from {len(strings)} quest strings to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

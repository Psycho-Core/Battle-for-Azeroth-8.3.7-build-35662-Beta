# modules

This is where my script modules live. One entry right now.

## mod-psycho-house

Player housing for BfA 8.3.7 build 35662. Everything for it is inside that one folder:
reference documents, the captured retail data, the tools, the C++ core, the addon, the SQL,
and three draft modules ready to install.

| Path | What it is |
| --- | --- |
| `mod-psycho-house/README.md` | Start here: the document index, the tools, the audit log |
| `mod-psycho-house/10-draft-modules.md` | The three drafts, side by side |
| `mod-psycho-house/drafts/midnight-port/` | Draft A: retail content and retail numbers |
| `mod-psycho-house/drafts/bfa-native/` | Draft B: 8.3.7 ids only, gold only |
| `mod-psycho-house/drafts/mod-psychohouse/` | Draft C: one island house per character, gossip only, no addon |
| `mod-psycho-house/data/` | 1 056 vendor stock lines, 739 item ids, 39 npc ids |

Install one of them from the module folder:

```
python3 modules/mod-psycho-house/tools/housing/install.py --draft bfa-native --root .
```

That works in this tree and in the older `src/server/scripts/` layout; the installer checks
which one it is looking at and renames the loader export when it has to.

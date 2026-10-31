# Patch 0001 - route `BFAH` addon messages to the housing module

This is the **only** change the housing module makes to existing core code. Everything
else lives in `src/server/game/Housing`, `src/server/scripts/Housing`, the two SQL
updates, the config block and the addon folder.

Without this hook the module still compiles and loads, but the addon gets no replies:
`WorldSession::HandleChatAddonMessage` would route `BFAH` straight into guild/whisper/
party/party-channel handling, where the prefix is unknown and the message is dropped.

Applied on top of `src/server/game/Handlers/ChatHandler.cpp`. Lines are from the tree as
of this branch; the hunk is small enough to re-apply by hand if context drifts.

```diff
diff --git a/src/server/game/Handlers/ChatHandler.cpp b/src/server/game/Handlers/ChatHandler.cpp
index 18c4b75..5a5d831 100644
--- a/src/server/game/Handlers/ChatHandler.cpp
+++ b/src/server/game/Handlers/ChatHandler.cpp
@@ -28,6 +28,7 @@
 #include "Group.h"
 #include "Guild.h"
 #include "GuildMgr.h"
+#include "HousingMgr.h"
 #include "Language.h"
 #include "Log.h"
 #include "ObjectAccessor.h"
@@ -432,6 +433,17 @@ void WorldSession::HandleChatAddonMessage(ChatMsg type, std::string prefix, std:
     if (!sWorld->getBoolConfig(CONFIG_ADDON_CHANNEL))
         return;
 
+    if (!sender)
+        return;
+
+    // The housing module owns the BFAH prefix end to end. This is the only place the
+    // core knows the module exists; see docs/player-housing/02-module-architecture.md.
+    if (prefix == Housing::kAddonPrefix)
+    {
+        sHousingMgr->HandleAddonMessage(sender, text);
+        return;
+    }
+
     switch (type)
     {
         case CHAT_MSG_GUILD:
```

## Notes on the hunk

- **Before the switch on purpose.** Intercepting ahead of the switch means the same
  message works whether the addon sent it as `WHISPER`, `PARTY` or `GUILD`. That matters
  as soon as a UI wants to do group-scoped operations without a protocol change.
- **`if (!sender) return;`** is defensive. It is not reachable today (addon messages come
  from a logged-in player), but the new code dereferences `sender` and the rest of the
  function historically did not have to before this point.
- **`CONFIG_ADDON_CHANNEL` still gates everything.** A realm that disables the addon
  channel also disables housing; that is the correct precedence and I did not want to add
  a second, contradicting switch.
- **`Housing::kAddonPrefix`** is `char const*` in `HousingDefines.h`; the comparison is
  `std::string == char const*`, so no allocation happens on the hot path.

## Alternative hook points I rejected

| Option | Why not |
| --- | --- |
| New opcode / `WorldSession` handler | Needs an opcode in the client's enum space; 8.3.7 is fixed and patching opcodes is a client-patch dependency for every player. |
| `PlayerScript::OnChat` | Never sees addon-channel traffic (it fires on `CMSG_MESSAGECHAT`, not `CMSG_CHAT_ADDON_MESSAGE`). |
| Gossip-only UI | Gossip cannot express a placement cursor or a 55-plot map. |
| Whisper a hidden NPC | Relies on NPC existence in DB2, and whispers are rate-limited and visible in chat logs. |

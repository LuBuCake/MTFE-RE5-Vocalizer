# How It Works

Addresses are virtual addresses of `re5dx9.exe` (Steam, 2023 update), as seen in a disassembler after the executable has unpacked itself (RVA = VA - 0x400000).

## How the game speaks

`uPlayer::Speak` (`0xB5B3C0`, `this` = the player, `ret 0x10`) takes the line id, two values the game always passes as `0, 1`, and a network flag. It plays the line through `sPlayer::PlaySpeech` (`0x7E7280`, which locks `sPlayer` and calls `uPlayer::PlaySpeech`, `0xB7AB60`) and, with the network flag set in an online game, sends it to the partner (`sNetwork::SendSpeech`, `0x793250`). The hotkeys call it with the network flag on. `uPlayer+0x2674` is 1 while a line plays: a key waits for it.

The line ids are the same for every character: 16 is "Go!", 66 is "I'm low on ammo", 82 to 90 ask for a kind of item, and so on. The lines that depend on the partner (their name, said loudly, quietly or shouted) have one id per partner.

## Who is the AI

`uPlayer+0x2DA8` (`AIControlled` in CommonLibRE5) says who drives a player:

| Value | Player |
| --- | --- |
| 0 | the player of this machine |
| 1 | the AI partner |
| 2 | the online partner: the game ignores its damage locally and plays the lines that come from the network only for it |

## The action responses

Player actions are objects of the `cPlAction_pl_act_*` classes (vtable slot 7 updates them), and **the same classes run for the player and for the AI**. Four of them say a response line, but only when their player is the AI: the line is behind `cmp dword ptr [player+0x2DA8], 1` and a `jnz` that skips it.

| Action (slot 7) | Compare | `jnz` | `call Speak` | Lines | Game rule |
| --- | --- | --- | --- | --- | --- |
| `item_hand_partner_second` `0xC746B0`: gives an item | `0xC74871` | `0xC74877` | `0xC748AA` | 32, 33 | speech cooldown 0 off (`0xB5B5E0`), then on for 5 s |
| `item_use_partner_second` `0xC80E50`: heals the partner | `0xC81006` | `0xC8100D` | `0xC81045` | 52, 53, 54 | |
| `coop_treat` `0xC81950`: helps the dying partner | `0xC82772` | `0xC82778` | `0xC827BD` | 52, 53, 54 | at the right animation frame |
| `weapon_reload` `0xC8AC30`: reloads | `0xC8AF71` | `0xC8AF78` | `0xC8AFAE` | 66 | 10% of the reloads, and a flag of `sMain+0x10438` |

The guarded blocks hold only the line (and the cooldown). Every other caller of `Speak` and `sPlayer::PlaySpeech` either already speaks for the player or is not a response to the player's own action.

Vocalizer:

1. Checks the bytes of each compare and jump, and the `call` of each line. If any of them is not the expected code, the responses stay off and the log says where.
2. Turns each `jnz` (`75`) into `ja` (`77`): the line now plays for 0 (this machine's player) and 1 (the AI). 2 (the online partner) still skips it, because the partner's own game speaks for them. Turning the plugin off puts the `jnz` back.
3. Rewrites the four `call Speak` (the previous target is kept and called, so other plugins hooking the same calls keep working): when the speaker is this machine's player, the network flag is set, so the online partner hears the line. The AI's lines never needed it.

Version 1 imitated these responses by watching the player's current move and choosing the lines itself; version 2 runs the game's own code.

Vocalizer
=========

A **Resident Evil 5** plugin for [MTFE](https://github.com/LuBuCake/MTFE) that lets the player speak: the game's own voice lines on hotkeys ("Watch out!", "Nice shot!", calling the partner by name, asking for the weapon the partner has equipped...), plus the **responses the partner AI gives to its own actions**, now for the player too.

## Features

* **25 voice groups on hotkeys**, each cycling through its lines. The ones about the partner pick the line from the partner's character (Chris, Sheva, Jill, Wesker, Josh, Excella, Barry, Rebecca) and from the item the partner has equipped.
* **Action responses**: when the player gives an item to the partner, heals the partner or helps them up, or reloads, the character says the same lines the AI partner says for those actions, with the game's own rules (random line, cooldowns, the reload line on 10% of the reloads).
* **Online**: the lines also reach the online partner, as the game's own voice lines do.
* **Settings in the game's menu**: Extras > Special Settings > MODS > Vocalizer. Every key can be changed there.

## Default Keys

| Key | Group | Key | Group |
| --- | --- | --- | --- |
| Numpad 0 | Watch out | `.` (numpad) | Leave it to me |
| Numpad 1 | Damn | Page Down | Be careful |
| Numpad 2 | Go ahead | Page Up | I'm coming |
| Numpad 3 | Good work | `+` (numpad) | Yes |
| Numpad 4 | You take it | `-` (numpad) | No |
| Numpad 5 | Call the partner (and their name) | `/` (numpad) | The partner's name, quietly |
| Numpad 6 | Fan out | `*` (numpad) | Call for help |
| Numpad 7 | Take care | Insert | Can't carry |
| Numpad 8 | Follow me | Delete | Nice shot |
| Numpad 9 | I owe you | Home | Dying |
| F1 | Go | End | Partner down (shouts their name) |
| F2 | Wait | F3 | Give me... (what the partner has equipped) |
| F4 | Give an item | | |

A key does nothing while a line is still playing, in menus, or when the player is dead.

## Requirements

* Resident Evil 5 (Steam, Gold Edition, version 1.2.0).
* [MTFE](https://github.com/LuBuCake/MTFE) 1.0 or newer.

## Installing

1. Install MTFE.
2. Copy `Vocalizer.dll` into `<game>\MTFE\Plugins\`.
3. Settings: Play Game > Special Settings > MODS > Vocalizer, saved in `<game>\MTFE\Config\Vocalizer.ini`. Log: `<game>\MTFE\Logs\Vocalizer.log`.

| Setting | Default | |
| --- | --- | --- |
| `bEnabled` | 1 | hotkeys and action responses (0 turns the plugin off, responses included) |
| `i<Group>` | see above | game key code of each group (set it from the menu) |

## Building

### Pre-requisites

* [Visual Studio](https://visualstudio.microsoft.com/) with the *Desktop development with C++* workload (MSVC x86) and the Windows SDK 10.0.26100.
* [XMake](https://xmake.io) 3.0 or newer.
* [CommonLibRE5](https://github.com/LuBuCake/CommonLibRE5) cloned **next to** this repository:

```
GitHub\
  MTFE-RE5-Vocalizer\
  CommonLibRE5\
```

### Building

From a PowerShell or Developer prompt in this folder:

```
xmake f -y -p windows -a x86 -m releasedbg --vs_sdkver=10.0.26100.0
xmake build
```

The plugin is `build\windows\x86\releasedbg\Vocalizer.dll`.

## License

[GPL-3.0](LICENSE).

/*
    This file is part of Vocalizer.
    Vocalizer is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.
    Vocalizer is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Vocalizer. If not, see <https://www.gnu.org/licenses/>.
*/

// Vocalizer: the local player says the game's own voice lines on hotkeys ("Watch out!", "Nice shot!", calling the
// partner by name...), and answers their own actions with the lines the partner AI says for them (giving an item,
// healing the partner, reloading). The groups of lines are the ones of the original Vocalizer
// (github.com/LuBuCake/RE5.Mod.Vocalizer): each key cycles through its lines; the lines that depend on the partner
// (name, death, what to ask for) are picked from the partner's character and equipped weapon. The responses are the
// game's own code (see "the responses" below). Hotkeys in Extras > Special Settings > MODS > Vocalizer.

#include <RE/RE5.h>
#include <MTFE/MTFE.h>

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace
{
    // Game key codes (RE::GameKey; table 0x10E5EF0): numpad 0..9 = 0x3B..0x44, * + - . / = 0x45..0x49, F1 = 0x4A,
    // PgUp 0x0D, PgDn 0x0E, End 0x0F, Home 0x10, Insert 0x15, Delete 0x16.
    constexpr std::int32_t kNumpad0 = 0x3B, kMultiply = 0x45, kAdd = 0x46, kSubtract = 0x47, kDecimal = 0x48, kDivide = 0x49;
    constexpr std::int32_t kF1 = RE::GameKey::kF1, kPageUp = 0x0D, kPageDown = 0x0E, kEnd = 0x0F, kHome = 0x10, kInsert = 0x15, kDelete = 0x16;

    // What a group says. Fixed groups cycle through their lines; the others look at the partner.
    enum class Kind
    {
        kFixed,
        kCallPartner,     // "Hey!", the partner's name, "Over here!"
        kCallPartnerSoft, // the partner's name, quietly
        kPartnerDead,     // the partner's name, shouted (Wesker: three lines of his own)
        kGiveMeItem,      // asks for what the partner has equipped
    };

    struct Group
    {
        const char* Key;
        const char* Label;
        const char* Help;
        std::int32_t DefaultKey;
        Kind Type;
        std::vector<std::uint16_t> Lines;
        std::size_t Next = 0;
        MTFE::Menu::Control Hotkey;
        RE::GameKey::Watch Watch;
    };

    // The 25 groups of the original Vocalizer, with its keys and lines (0 in a line list = the partner's line).
    std::array<Group, 25> Groups{ {
        { "iWatchOut", "Watch out", "Warns the partner.", kNumpad0, Kind::kFixed, { 79 } },
        { "iDamn", "Damn", "Curses.", kNumpad0 + 1, Kind::kFixed, { 72, 73 } },
        { "iGoAhead", "Go ahead", "Tells the partner to go first.", kNumpad0 + 2, Kind::kFixed, { 98, 99 } },
        { "iGoodWork", "Good work", "Praises the partner.", kNumpad0 + 3, Kind::kFixed, { 58, 59, 60 } },
        { "iYouTake", "You take it", "Leaves an item to the partner.", kNumpad0 + 4, Kind::kFixed, { 94, 95 } },
        { "iCallPartner", "Call partner", "Calls the partner, by name too.", kNumpad0 + 5, Kind::kCallPartner, { 41, 0, 42 } },
        { "iFanOut", "Fan out", "Tells the partner to split up.", kNumpad0 + 6, Kind::kFixed, { 17, 18, 25, 26 } },
        { "iTakeCare", "Take care", "Wishes the partner luck.", kNumpad0 + 7, Kind::kFixed, { 96, 97 } },
        { "iFollowMe", "Follow me", "Tells the partner to follow.", kNumpad0 + 8, Kind::kFixed, { 14, 15 } },
        { "iIOweYou", "I owe you", "Thanks the partner.", kNumpad0 + 9, Kind::kFixed, { 49, 50, 51 } },
        { "iLeaveToMe", "Leave it to me", "Takes charge.", kDecimal, Kind::kFixed, { 21 } },
        { "iBeCareful", "Be careful", "Warns the partner to be careful.", kPageDown, Kind::kFixed, { 52, 53, 54 } },
        { "iImComing", "I'm coming", "Tells the partner help is on the way.", kPageUp, Kind::kFixed, { 47, 48 } },
        { "iYes", "Yes", "Agrees.", kAdd, Kind::kFixed, { 19, 20 } },
        { "iNo", "No", "Disagrees.", kSubtract, Kind::kFixed, { 22, 23, 24 } },
        { "iCallPartnerSoft", "Call partner (quiet)", "Says the partner's name.", kDivide, Kind::kCallPartnerSoft, { 0 } },
        { "iHelp", "Call for help", "Asks the partner for help.", kMultiply, Kind::kFixed, { 43, 44 } },
        { "iCantCarry", "Can't carry", "Says the inventory is full.", kInsert, Kind::kFixed, { 91, 92, 93 } },
        { "iNiceShot", "Nice shot", "Praises the partner's aim.", kDelete, Kind::kFixed, { 62, 63, 64 } },
        { "iDying", "Dying", "Says you are badly hurt.", kHome, Kind::kFixed, { 45, 46 } },
        { "iPartnerDead", "Partner down", "Shouts the partner's name.", kEnd, Kind::kPartnerDead, { 0 } },
        { "iGo", "Go", "Tells the partner to go.", kF1, Kind::kFixed, { 16 } },
        { "iWait", "Wait", "Tells the partner to wait.", kF1 + 1, Kind::kFixed, { 27 } },
        { "iGiveMeItem", "Give me an item", "Asks the partner for what they have equipped.", kF1 + 2, Kind::kGiveMeItem, { 0 } },
        { "iGiveItem", "Give an item", "Offers the partner an item.", kF1 + 3, Kind::kFixed, { 32, 33 } },
    } };

    // Partner lines by the partner's character (RE::CharacterID 0..7): name ("Chris!"), quiet name, name shouted.
    struct PartnerLines
    {
        std::uint16_t Name, Soft, Dead;
    };
    constexpr PartnerLines kPartner[8] = {
        { 37, 10, 67 }, { 38, 11, 68 }, { 39, 12, 69 }, { 40, 13, 70 }, // Chris, Sheva, Jill, Wesker
        { 28, 6, 55 },  { 36, 9, 65 },  { 34, 7, 56 },  { 35, 8, 57 },  // Josh, Excella, Barry, Rebecca
    };
    constexpr std::uint16_t kWeskerPartnerDead[3] = { 67, 68, 69 };

    MTFE::Menu::Control EnabledToggle;
    std::size_t WeskerDeadNext = 0;

    // ---- the game ----------------------------------------------------------------------------------------------

    std::int32_t LocalIndex()
    {
        auto* GameSys = RE::cGameSys::GetSingleton();
        const auto Mask = GameSys ? GameSys->LocalPlayerMask() : 0;
        for (std::int32_t i = 0; i < 4; ++i)
        {
            if (Mask >> i & 1)
            {
                return i;
            }
        }
        return 0;
    }

    std::int32_t CharacterOf(RE::uPlayer* Player)
    {
        auto* Players = RE::sPlayer::GetSingleton();
        auto* Info = RE::sInfoMan::GetSingleton();
        if (!Players || !Info || !Player)
        {
            return -1;
        }
        for (std::uint32_t i = 0; i < Players->GetPlayerCount() && i < RE::sPlayer::kMaxPlayers; ++i)
        {
            if (Players->GetPlayer(i) == Player)
            {
                return static_cast<std::int32_t>(Info->GetActiveGame()->PlayerRecords()[i].character);
            }
        }
        return -1;
    }

    // The partner's lines, or none when there is no partner or it is the same character.
    const PartnerLines* PartnerOf(RE::uPlayer* Player)
    {
        const auto Mine = CharacterOf(Player);
        const auto Theirs = CharacterOf(Player->Partner());
        return Theirs >= 0 && Theirs < 8 && Theirs != Mine ? &kPartner[Theirs] : nullptr;
    }

    RE::cItem* Equipped(RE::uPlayer* Player)
    {
        for (std::size_t i = 0; i < 9; ++i)
        {
            if (Player->Item(i).equipType == 1)
            {
                return &Player->Item(i);
            }
        }
        return nullptr;
    }

    // "Give me ..." by the item the partner has equipped (item ids: RE/Game/Items.h).
    std::uint16_t AskFor(RE::uPlayer* Partner)
    {
        const auto* Item = Partner ? Equipped(Partner) : nullptr;
        switch (Item ? Item->id : 0)
        {
        case 258: case 272: case 273: case 274: case 286: case 287: case 297: // handguns
            return 82;
        case 260: case 278: case 279: case 281: // shotguns
            return 83;
        case 261: case 284: case 288: // rifles
            return 84;
        case 267: case 282: case 283: // magnums
            return 85;
        case 259: case 265: case 275: case 285: // machine guns
            return 86;
        case 269: case 309: // rocket launchers
            return 87;
        case 262: case 263: case 264: case 266: // grenades
            return 31;
        case 268: case 271: case 277: case 289: case 290: case 293: case 294: case 295: case 303: case 308: case 313: case 314: case 315:
            return 81; // other weapons
        case 772: // first aid spray
            return 90;
        case 773: case 774: case 775: case 777: // herbs
            return 30;
        case 316: case 317: case 318: // eggs
            return 89;
        case 513: case 514: case 515: case 516: case 518: case 519: case 520: case 521: case 526: case 527: case 528: // ammo
            return 29;
        default:
            return 88; // "Give me something"
        }
    }

    // The next line of a group (0 = nothing to say: no partner for a partner line).
    std::uint16_t NextLine(Group& Item, RE::uPlayer* Player)
    {
        const auto* Partner = PartnerOf(Player);
        switch (Item.Type)
        {
        case Kind::kCallPartnerSoft:
            return Partner ? Partner->Soft : 0;
        case Kind::kPartnerDead:
            if (CharacterOf(Player) == static_cast<std::int32_t>(RE::CharacterID::kWesker))
            {
                const auto Line = kWeskerPartnerDead[WeskerDeadNext];
                WeskerDeadNext = (WeskerDeadNext + 1) % std::size(kWeskerPartnerDead);
                return Line;
            }
            return Partner ? Partner->Dead : 0;
        case Kind::kGiveMeItem:
            return AskFor(Player->Partner());
        default:
            break;
        }
        const auto Line = Item.Lines[Item.Next];
        Item.Next = (Item.Next + 1) % Item.Lines.size();
        if (Item.Type == Kind::kCallPartner && Line == 0)
        {
            return Partner ? Partner->Name : 0;
        }
        return Line;
    }

    void Say(RE::uPlayer* Player, std::uint16_t Line)
    {
        if (Line)
        {
            Player->Speak(Line);
            // uPlayer::Speaking (+0x2674, from GameX) should turn on now: it keeps the keys from cutting a line.
            MTFE::Log::Info("line {} (speaking flag {:X})", Line, Player->Speaking());
        }
    }

    // ---- the responses: the partner AI's own lines, for the local player too --------------------------------------
    // The player actions (cPlAction_*, shared by humans and AI) say their response lines only when their player is
    // the AI: `cmp [player+0x2DA8], 1` (uPlayer::AIControlled: 0 this machine's player, 1 AI, 2 online partner) and
    // `jnz` past the line. That jnz becomes `ja` (75 -> 77): 0 and 1 speak, 2 still skips (its own game speaks for it).
    // The game keeps its own rules (random line, cooldowns, the 10% of the reload). Ghidra VAs, Steam 2023.
    struct Gate
    {
        const char* Action;
        std::uintptr_t Compare; // cmp dword ptr [reg+0x2DA8], 1 (39 /r with the register = 1, or 83 /7 01)
        std::uintptr_t Speak;   // call uPlayer::Speak (0xB5B3C0) of that line, ecx = the player
    };
    constexpr Gate kGates[] = {
        { "cPlAction_pl_act_item_hand_partner_second (gives an item: 32, 33)", 0xC74871, 0xC748AA },
        { "cPlAction_pl_act_item_use_partner_second (heals the partner: 52-54)", 0xC81006, 0xC81045 },
        { "cPlAction_pl_act_coop_treat (treats the dying partner: 52-54)", 0xC82772, 0xC827BD },
        { "cPlAction_pl_act_weapon_reload (10% of the reloads: 66)", 0xC8AF71, 0xC8AFAE },
    };
    constexpr std::uint8_t kJnz = 0x75, kJa = 0x77;
    constexpr std::uintptr_t kSpeakVa = 0xB5B3C0;

    std::uintptr_t Address(std::uintptr_t Va)
    {
        return REL::Offset{ Va - 0x400000 }.GetAddress();
    }

    // The jnz after the compare, or 0 when the code is not the expected one.
    std::uintptr_t JumpOf(const Gate& Site)
    {
        const auto* Code = reinterpret_cast<const std::uint8_t*>(Address(Site.Compare));
        const bool Register = Code[0] == 0x39;                               // cmp [reg+disp32], reg
        const bool Immediate = Code[0] == 0x83 && (Code[1] & 0x38) == 0x38; // cmp [reg+disp32], imm8
        if ((!Register && !Immediate) || (Code[1] & 0xC0) != 0x80 || *reinterpret_cast<const std::uint32_t*>(Code + 2) != 0x2DA8 ||
            (Immediate && Code[6] != 1))
        {
            return 0;
        }
        const auto Jump = Address(Site.Compare) + (Register ? 6 : 7);
        const auto Opcode = *reinterpret_cast<const std::uint8_t*>(Jump);
        return Opcode == kJnz || Opcode == kJa ? Jump : 0;
    }

    bool ResponsesReady = false, ResponsesOn = false;
    std::array<std::uintptr_t, std::size(kGates)> SpeakOriginal{};

    // The line of our own player goes to the online partner too, as the hotkeys' lines do (the AI's never needed it).
    template <std::size_t I>
    void __fastcall SpeakHook(RE::uPlayer* Player, void*, std::uint32_t Line, std::uint32_t A, std::uint32_t B, std::uint32_t Network)
    {
        if (Player && Player->AIControlled() == 0)
        {
            Network = 1;
        }
        using Func = void(__thiscall*)(RE::uPlayer*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
        reinterpret_cast<Func>(SpeakOriginal[I])(Player, Line, A, B, Network);
    }

    template <std::size_t... I>
    void HookSpeak(std::index_sequence<I...>)
    {
        ((SpeakOriginal[I] = REL::WriteCall5(Address(kGates[I].Speak), reinterpret_cast<std::uintptr_t>(&SpeakHook<I>))), ...);
    }

    void InstallResponses()
    {
        for (const auto& Site : kGates)
        {
            const auto* Call = reinterpret_cast<const std::uint8_t*>(Address(Site.Speak));
            if (!JumpOf(Site) || Call[0] != 0xE8)
            {
                MTFE::Log::Error("responses off: unexpected code in {} ({:08X} / {:08X})", Site.Action, Site.Compare, Site.Speak);
                return;
            }
            if (Address(Site.Speak) + 5 + *reinterpret_cast<const std::int32_t*>(Call + 1) != Address(kSpeakVa))
            {
                MTFE::Log::Warn("call at {:08X} already hooked by someone else - chaining", Site.Speak);
            }
        }
        HookSpeak(std::make_index_sequence<std::size(kGates)>{});
        ResponsesReady = true;
        MTFE::Log::Info("responses: {} actions of the partner AI also speak for the local player", std::size(kGates));
    }

    // jnz <-> ja, following the Enabled toggle.
    void SyncResponses(bool On)
    {
        if (!ResponsesReady || On == ResponsesOn)
        {
            return;
        }
        for (const auto& Site : kGates)
        {
            REL::SafeWrite(JumpOf(Site), On ? kJa : kJnz);
        }
        ResponsesOn = On;
    }

    void OnFrame()
    {
        SyncResponses(EnabledToggle.GetBool());
        if (!EnabledToggle.GetBool())
        {
            return;
        }
        auto* Players = RE::sPlayer::GetSingleton();
        auto* Main = RE::sMain::GetSingleton();
        if (!Players || !Main || Players->GetPlayerCount() == 0 || RE::Field<bool>(Main, 0x10428)) // 0x10428: menus get no input
        {
            return;
        }
        auto* Player = Players->GetPlayer(static_cast<std::size_t>(LocalIndex()));
        if (!Player || Player->IsDead() || Player->Speaking())
        {
            return;
        }
        for (auto& Item : Groups)
        {
            if (Item.Watch.Pressed(Item.Hotkey.Get()))
            {
                Say(Player, NextLine(Item, Player));
                return; // one line at a time
            }
        }
    }

    void OnMessage(MTFE::Message* Message)
    {
        if (Message->type == MTFE_MESSAGE_GAME_INIT)
        {
            InstallResponses();
            MTFE::AddFrameCallback(OnFrame);
        }
    }
}

MTFE_PLUGIN_LOAD(const MTFE::LoadInterface* Interface)
{
    MTFE::Init(Interface);
    EnabledToggle = MTFE::Menu::AddToggle("bEnabled", "Enabled",
                                          "Voice lines on the hotkeys below, and the partner AI's responses for your own actions.", true);
    MTFE::Menu::AddHeader("Hotkeys");
    for (auto& Item : Groups)
    {
        Item.Hotkey = MTFE::Menu::AddHotkey(Item.Key, Item.Label, Item.Help, Item.DefaultKey);
    }
    MTFE::Log::Info("Vocalizer loaded: {} voice groups", Groups.size());
    return MTFE::RegisterListener(OnMessage);
}

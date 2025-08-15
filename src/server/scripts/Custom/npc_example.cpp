// /*
//  * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
//  *
//  * This program is free software; you can redistribute it and/or modify it
//  * under the terms of the GNU General Public License as published by the
//  * Free Software Foundation; either version 2 of the License, or (at your
//  * option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful, but WITHOUT
//  * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  * more details.
//  *
//  * You should have received a copy of the GNU General Public License along
//  * with this program. If not, see <http://www.gnu.org/licenses/>.
//  */

// #include "ScriptMgr.h"
// #include "ScriptedCreature.h"
// #include "ScriptedGossip.h"
// #include "Player.h"
// #include "Chat.h"

// enum ExampleNpcText
// {
//     SAY_HELLO = 0,
//     SAY_GOODBYE = 1
// };

// enum ExampleNpcGossip
// {
//     GOSSIP_OPTION_HELLO = 0,
//     GOSSIP_OPTION_GOODBYE = 1,
//     GOSSIP_OPTION_HEAL = 2
// };

// class npc_example : public CreatureScript
// {
// public:
//     npc_example() : CreatureScript("npc_example") { }

//     struct npc_exampleAI : public ScriptedAI
//     {
//         npc_exampleAI(Creature* creature) : ScriptedAI(creature) { }

//         void Reset() override
//         {
//             // 初始化AI状态
//         }

//         void UpdateAI(uint32 diff) override
//         {
//             // 更新AI逻辑
//         }
//     };

//     CreatureAI* GetAI(Creature* creature) const override
//     {
//         return new npc_exampleAI(creature);
//     }

//     bool OnGossipHello(Player* player, Creature* creature) override
//     {
//         // 添加问候选项
//         AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Hello!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + GOSSIP_OPTION_HELLO);
//         // 添加告别选项
//         AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Goodbye!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + GOSSIP_OPTION_GOODBYE);
//         // 添加治疗选项
//         AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Heal me!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + GOSSIP_OPTION_HEAL);

//         SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
//         return true;
//     }

//     bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
//     {
//         player->PlayerTalkClass->ClearMenus();

//         switch (action - GOSSIP_ACTION_INFO_DEF)
//         {
//             case GOSSIP_OPTION_HELLO:
//                 creature->AI()->Talk(SAY_HELLO);
//                 CloseGossipMenuFor(player);
//                 break;
//             case GOSSIP_OPTION_GOODBYE:
//                 creature->AI()->Talk(SAY_GOODBYE);
//                 CloseGossipMenuFor(player);
//                 break;
//             case GOSSIP_OPTION_HEAL:
//                 // 治疗玩家到满血
//                 player->SetHealth(player->GetMaxHealth());
//                 player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
//                 ChatHandler(player->GetSession()).SendSysMessage("You have been healed!");
//                 CloseGossipMenuFor(player);
//                 break;
//         }

//         return true;
//     }
// };

// void AddSC_npc_example()
// {
//     new npc_example();
// }
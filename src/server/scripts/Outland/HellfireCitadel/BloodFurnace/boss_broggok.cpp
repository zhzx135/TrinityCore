/**
 * @file boss_broggok.cpp
 * @brief 血熔炉副本 - 布洛戈克(Broggok)首领AI实现
 *
 * 该文件包含布洛戈克及其相关生物的AI脚本：
 * - 布洛戈克首领(boss_broggok)的战斗逻辑
 * - 囚犯生物(npc_nascent_fel_orc/npc_fel_orc_neophyte)的行为
 * - 牢房门控制杆(go_broggok_lever)的交互逻辑
 * - 毒云法术(spell_broggok_poison_cloud)的效果处理
 *
 * 主要功能：
 * - 实现布洛戈克的多阶段战斗机制
 * - 处理毒云生成和扩散效果
 * - 管理囚犯生物的行为模式
 * - 控制牢房门开启的交互逻辑
 */
/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "blood_furnace.h"
#include "Containers.h"
#include "GameObject.h" 
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

enum BroggokTexts
{
    SAY_INTRO                        = 0,
    SAY_AGGRO                        = 1
};

enum BroggokSpells
{
    SPELL_SLIME_SPRAY                = 30913,
    SPELL_POISON_BOLT                = 30917,
    SPELL_POISON_CLOUD               = 30916,

    SPELL_SUMMON_INCOMBAT_TRIGGER    = 26837,
    SPELL_DESPAWN_INCOMBAT_TRIGGER   = 26838,

    // Cloud
    SPELL_POISON_CLOUD_PASSIVE       = 30914,

    // Prisioners
    SPELL_STOMP                      = 31900,
    SPELL_CONCUSSION_BLOW            = 22427,
    SPELL_FRENZY                     = 8269,
    SPELL_CHARGE                     = 22120
};

enum BroggokEvents
{
    EVENT_SLIME_SPRAY                = 1,
    EVENT_POISON_BOLT,
    EVENT_POISON_CLOUD,

    EVENT_INTRO_1,
    EVENT_INTRO_2,

    EVENT_ACTIVATE_1,
    EVENT_ACTIVATE_2
};

enum BroggokMisc
{
    PATH_ROOM                        = 1381150,
    NPC_BROGGOK_POISON_CLOUD         = 17662,
    NPC_INCOMBAT_TRIGGER             = 16006
};

// 17380 - Broggok
struct boss_broggok : public BossAI
{
    /**
     * @brief 构造 Broggok BOSS 的 AI 实例
     * @param creature 关联的生物实体
     * @note 继承自 BossAI 基类，初始化 Broggok BOSS 数据
     */
    boss_broggok(Creature *creature) : BossAI(creature, DATA_BROGGOK) {}

    /**
     * @brief 重置Boss状态
     *
     * 调用基类的_Reset()方法，并执行ACTION_RESET_BROGGOK动作来重置Broggok的状态
     */
    void Reset() override
    {
        _Reset();
        DoAction(ACTION_RESET_BROGGOK);
    }

    /**
     * @brief 当Boss进入战斗时调用的函数
     *
     * 1. 调用父类的JustEngagedWith函数
     * 2. 对自身施放SPELL_DESPAWN_INCOMBAT_TRIGGER法术
     * 3. 安排3个周期性事件：
     *    - EVENT_SLIME_SPRAY：8-12秒后触发
     *    - EVENT_POISON_BOLT：2-10秒后触发
     *    - EVENT_POISON_CLOUD：5-10秒后触发
     *
     * @param who 触发战斗的单位
     */
    void JustEngagedWith(Unit *who) override
    {
        BossAI::JustEngagedWith(who);
        DoCastSelf(SPELL_DESPAWN_INCOMBAT_TRIGGER);

        events.ScheduleEvent(EVENT_SLIME_SPRAY, 8s, 12s);
        events.ScheduleEvent(EVENT_POISON_BOLT, 2s, 10s);
        events.ScheduleEvent(EVENT_POISON_CLOUD, 5s, 10s);
    }

  /**
 * @brief 处理召唤的生物的逻辑。
 * 
 * 当召唤的生物是毒云或战斗触发器时，设置其行为状态并执行相应的动作。
 * 
 * @param summoned 被召唤的生物指针。
 * 
 * - 如果召唤的是毒云（NPC_BROGGOK_POISON_CLOUD）：
 *   1. 设置其行为状态为被动（REACT_PASSIVE）。
 *   2. 对其施放毒云被动技能（SPELL_POISON_CLOUD_PASSIVE）。
 *   3. 将其添加到召唤列表中。
 * 
 * - 如果召唤的是战斗触发器（NPC_INCOMBAT_TRIGGER）：
 *   1. 设置其行为状态为被动（REACT_PASSIVE）。
 *   2. 强制其进入战斗状态（DoZoneInCombat）。
 *   3. 将其添加到召唤列表中。
 */  
    void JustSummoned(Creature* summoned) override
    {
        if (summoned->GetEntry() == NPC_BROGGOK_POISON_CLOUD)
        {
            summoned->SetReactState(REACT_PASSIVE);
            summoned->CastSpell(summoned, SPELL_POISON_CLOUD_PASSIVE, true);
            summons.Summon(summoned);
        }
        else if (summoned->GetEntry() == NPC_INCOMBAT_TRIGGER)
        {
            summoned->SetReactState(REACT_PASSIVE);
            DoZoneInCombat(summoned);
            summons.Summon(summoned);
        }
    }

    /**
 * @brief 处理Boss进入逃避模式时的逻辑。
 * 
 * 当Boss进入逃避模式时，会执行以下操作：
 * 1. 清除所有毒云召唤物。
 * 2. 施放战斗状态清除的触发效果。
 * 3. 重置Boss状态为未开始。
 * 4. 调用逃避时的反召唤逻辑。
 * 
 * @param why 逃避原因（未使用）。
 */
    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        summons.DespawnEntry(NPC_BROGGOK_POISON_CLOUD);
        DoCastSelf(SPELL_DESPAWN_INCOMBAT_TRIGGER, true);
        instance->SetBossState(DATA_BROGGOK, NOT_STARTED);
        _DespawnAtEvade();
    }
/**
 * @brief 处理 Broggok 的不同动作。
 *
 * 根据传入的动作类型执行相应的逻辑：
 * - ACTION_PREPARE_BROGGOK: 安排 EVENT_INTRO_1 事件。
 * - ACTION_ACTIVATE_BROGGOK: 安排 EVENT_ACTIVATE_1 事件。
 * - ACTION_RESET_BROGGOK: 重置 Broggok 的状态，包括设置被动反应、标记为不可交互、清除毒云、重置战斗状态，并恢复杠杆的初始状态。
 * - 其他动作: 无操作。
 *
 * @param action 动作类型，定义在枚举中。
 */
    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_PREPARE_BROGGOK:
                events.ScheduleEvent(EVENT_INTRO_1, 0s);
                break;
            case ACTION_ACTIVATE_BROGGOK:
                events.ScheduleEvent(EVENT_ACTIVATE_1, 0s);
                break;
            case ACTION_RESET_BROGGOK:
                me->SetReactState(REACT_PASSIVE);
                me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                summons.DespawnEntry(NPC_BROGGOK_POISON_CLOUD);
                DoCastSelf(SPELL_DESPAWN_INCOMBAT_TRIGGER);
                instance->SetBossState(DATA_BROGGOK, NOT_STARTED);
                if (GameObject * lever = instance->GetGameObject(DATA_BROGGOK_LEVER))
                {
                    lever->RemoveFlag(GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
                    lever->SetGoState(GO_STATE_READY);
                }
                break;
            default:
                break;
        }
    }

    /**
 * @brief 更新AI逻辑，处理战斗中的技能释放和事件调度。
 *
 * 该函数负责在战斗状态下更新Boss的行为逻辑，包括技能释放和事件调度。
 * 如果Boss没有目标，则更新非战斗事件并返回。
 * 在战斗状态下，会根据事件ID执行相应的技能释放，并设置技能的冷却时间。
 * 如果Boss正在施法，则直接返回。
 * 最后，如果Boss处于近战攻击范围内，则执行近战攻击。
 *
 * @param diff 时间差，用于更新事件和技能冷却。
 */
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            UpdateOutOfCombatEvents(diff);
            return;
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SLIME_SPRAY:
                    DoCastSelf(SPELL_SLIME_SPRAY);
                    events.Repeat(4s, 12s);
                    break;
                case EVENT_POISON_BOLT:
                    DoCastSelf(SPELL_POISON_BOLT);
                    events.Repeat(4s, 12s);
                    break;
                case EVENT_POISON_CLOUD:
                    DoCastSelf(SPELL_POISON_CLOUD);
                    events.Repeat(20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }
/**
 * @brief 更新非战斗状态的事件处理。
 *
 * 该函数用于处理Boss在非战斗状态下的各种事件，包括：
 * - 触发战斗前的介绍事件。
 * - 激活Boss并设置其行为状态。
 * - 播放Boss的台词和移动路径。
 *
 * @param diff 时间差，用于更新事件计时器。
 */
    void UpdateOutOfCombatEvents(uint32 diff)
    {
        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_INTRO_1:
                    DoCastSelf(SPELL_SUMMON_INCOMBAT_TRIGGER);
                    events.ScheduleEvent(EVENT_INTRO_2, 2s);
                    break;
                case EVENT_INTRO_2:
                    Talk(SAY_INTRO);
                    break;
                case EVENT_ACTIVATE_1:
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                    events.ScheduleEvent(EVENT_ACTIVATE_2, 4s);
                    break;
                case EVENT_ACTIVATE_2:
                    Talk(SAY_AGGRO);
                    me->GetMotionMaster()->MovePath(PATH_ROOM, false);
                    break;
                default:
                    break;
            }
        }
    }
};

static Emote const PrisionersEmotes[] =
{
    EMOTE_ONESHOT_ROAR,
    EMOTE_ONESHOT_SHOUT,
    EMOTE_ONESHOT_BATTLE_ROAR
};
/**
 * @brief 构造 Broggok 囚犯生物的 AI 实例。
 * @param creature 关联的生物实体。
 * @note 继承自 ScriptedAI 基类，并初始化实例脚本。
 */
struct BroggokPrisionersAI : public ScriptedAI
{
    BroggokPrisionersAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }
/**
 * @brief 重置Boss的状态。
 * 
 * 取消所有已调度的任务，并重新调度一个任务，该任务会在1到5秒后随机选择一个囚犯表情动作执行，
 * 并在6到9秒后重复执行。
 */
    void Reset() override
    {
        scheduler.CancelAll();
        scheduler.Schedule(1s, 5s, [this](TaskContext task)
        {
            me->HandleEmoteCommand(Trinity::Containers::SelectRandomContainerElement(PrisionersEmotes));
            task.Repeat(6s, 9s);
        });
    }
/**
 * @brief 当Boss被激活时调用的函数。
 * 
 * 此函数在Boss进入战斗状态时被触发，执行以下操作：
 * 1. 取消所有已调度的任务。
 * 2. 重新调度Boss的战斗事件。
 * 
 * @param who 触发战斗的单位（参数未使用）。
 */
    void JustEngagedWith(Unit* /*who*/) override
    {
        scheduler.CancelAll();
        ScheduleEvents();
    }
/**
 * @brief 调度事件，纯虚函数，由派生类实现具体的事件调度逻辑。
 */
    virtual void ScheduleEvents() = 0;

/**
 * @brief 当Boss返回家园时调用的函数。
 * 
 * 如果Boss Broggok的战斗状态为进行中，则重置Broggok的状态。
 * 最后调用基类的JustReachedHome函数。
 */
    void JustReachedHome() override
    {
        if (instance->GetBossState(DATA_BROGGOK) == IN_PROGRESS)
        {
            if (Creature* broggok = instance->GetCreature(DATA_BROGGOK))
                broggok->AI()->DoAction(ACTION_RESET_BROGGOK);
        }

        ScriptedAI::JustReachedHome();
    }

    /**
 * @brief 更新AI逻辑
 * 
 * 该函数用于更新Boss的AI行为，包括调度器的更新和近战攻击逻辑。
 * 
 * @param diff 时间差，单位为毫秒
 */
    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

protected:
    InstanceScript* instance;
    TaskScheduler scheduler;
};

// 17398 - Nascent Fel Orc
struct npc_nascent_fel_orc : public BroggokPrisionersAI
{
    /**
 * @brief 构造函数，初始化一个初生的邪能兽人NPC。
 * @param creature 指向Creature对象的指针，表示该NPC的实例。
 * @note 继承自BroggokPrisionersAI，用于处理初生邪能兽人的行为逻辑。
 */
    npc_nascent_fel_orc(Creature* creature) : BroggokPrisionersAI(creature) { }

    /**
 * 安排Boss的战斗事件。
 * 使用调度器定时执行以下技能：
 * 1. 15秒后对目标施放SPELL_CONCUSSION_BLOW，之后每8-11秒重复一次。
 * 2. 7秒后对自己施放SPELL_STOMP，之后每16-21秒重复一次。
 */
    void ScheduleEvents() override
    {
        scheduler
            .Schedule(15s, [this](TaskContext task)
            {
                DoCastVictim(SPELL_CONCUSSION_BLOW);
                task.Repeat(8s, 11s);
            })
            .Schedule(7s, [this](TaskContext task)
            {
                DoCastSelf(SPELL_STOMP);
                task.Repeat(16s, 21s);
            });
    }
};

// 17429 - Fel Orc Neophyte
struct npc_fel_orc_neophyte : public BroggokPrisionersAI
{
 /**
 * @brief 构造函数，初始化一个Fel Orc Neophyte NPC。
 * @param creature 指向Creature对象的指针，表示要初始化的NPC。
 */
    npc_fel_orc_neophyte(Creature* creature) : BroggokPrisionersAI(creature) { }

    void ScheduleEvents() override
    {
        scheduler
            .Schedule(5s, [this](TaskContext task)
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                    DoCast(target, SPELL_CHARGE);
                task.Repeat(20s);
            })
            .Schedule(1s, [this](TaskContext task)
            {
                DoCastSelf(SPELL_FRENZY);
                task.Repeat(12s, 13s);
            });
    }
};

// 181982 - Cell Door Lever
struct go_broggok_lever : public GameObjectAI
{
 /**
 * @brief 构造 Broggok 牢房门控制杆的 AI 实例。
 * @param go 关联的游戏对象实体。
 * @note 继承自 GameObjectAI 基类，并初始化实例脚本。
 */   
    go_broggok_lever(GameObject* go) : GameObjectAI(go), instance(go->GetInstanceScript()) { }

    InstanceScript* instance;

 /**
 * @brief 处理玩家与游戏对象的交互（GossipHello事件）。
 * 
 * 当玩家与游戏对象交互时触发，用于启动Boss战斗或更新游戏对象状态。
 * 
 * @param player 交互的玩家对象（未使用）。
 * @return true 表示交互成功处理。
 */   
    bool OnGossipHello(Player* /*player*/) override
    {
        if (instance->GetBossState(DATA_BROGGOK) != DONE && instance->GetBossState(DATA_BROGGOK) != IN_PROGRESS)
        {
            instance->SetBossState(DATA_BROGGOK, IN_PROGRESS);
            if (Creature* broggok = instance->GetCreature(DATA_BROGGOK))
                broggok->AI()->DoAction(ACTION_PREPARE_BROGGOK);
        }

        me->SetFlag(GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
        me->SetGoState(GO_STATE_ACTIVE);

        return true;
    }
};

// 30914, 38462 - Poison
class spell_broggok_poison_cloud : public AuraScript
{
/**
 * @brief 准备布洛戈克毒云法术的Aura脚本。
 *
 * 该脚本用于处理布洛戈克毒云法术的周期性效果，包括：
 * - 验证法术效果的有效性。
 * - 计算并应用法术效果的半径修正值。
 * - 触发周期性伤害效果。
 */
    
    PrepareAuraScript(spell_broggok_poison_cloud);

/**
 * @brief 验证法术信息的有效性。
 * 
 * 检查法术效果中触发的法术ID是否有效。
 * 
 * @param spellInfo 指向法术信息的指针。
 * @return bool 如果触发的法术ID有效则返回true，否则返回false。
 */

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->GetEffect(EFFECT_0).TriggerSpell });
    }

/**
 * @brief 周期性触发效果的处理函数。
 * 
 * 该函数用于处理周期性触发的法术效果，计算法术半径的修正值，并施放触发法术。
 * 
 * @param aurEff 指向当前法术效果的指针。
 * 
 * 功能说明：
 * 1. 阻止默认行为。
 * 2. 如果法术效果没有总触发次数，则直接返回。
 * 3. 计算触发法术的半径修正值，基于当前触发次数与总触发次数的比例。
 * 4. 对目标施放触发法术，并应用半径修正值。
 */
    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        if (!aurEff->GetTotalTicks())
            return;

        uint32 triggerSpell = aurEff->GetSpellEffectInfo().TriggerSpell;
        int32 mod = int32(((float(aurEff->GetTickNumber()) / aurEff->GetTotalTicks()) * 0.9f + 0.1f) * 10000 * 2 / 3);
        GetTarget()->CastSpell(nullptr, triggerSpell, CastSpellExtraArgs(aurEff).AddSpellMod(SPELLVALUE_RADIUS_MOD, mod));
    }

/**
 * @brief 注册周期性效果的处理函数。
 * 
 * 将周期性触发法术的效果与对应的处理函数绑定。
 * 当法术的周期性效果触发时，会调用 `PeriodicTick` 函数。
 */
    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_broggok_poison_cloud::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};


void AddSC_boss_broggok()
{
    RegisterBloodFurnaceCreatureAI(boss_broggok);
    RegisterBloodFurnaceCreatureAI(npc_nascent_fel_orc);
    RegisterBloodFurnaceCreatureAI(npc_fel_orc_neophyte);
    RegisterBloodFurnaceGameObjectAI(go_broggok_lever);
    RegisterSpellScript(spell_broggok_poison_cloud);
}

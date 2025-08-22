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

/* Find a more elegant way to handle channelers */

#include "ScriptMgr.h"
#include "blood_furnace.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"

enum KelidanTexts
{
    SAY_WAKE                    = 0,
    SAY_NOVA                    = 1,
    SAY_SLAY                    = 2,
    SAY_DEATH                   = 3,

    SAY_AGGRO                   = 0
};

enum KelidanSpells
{
    SPELL_CORRUPTION            = 30938,
    SPELL_EVOCATION             = 30935,

    SPELL_FIRE_NOVA             = 33132,
    SPELL_FIRE_NOVA_H           = 37371,

    SPELL_SHADOW_BOLT_VOLLEY    = 28599,
    SPELL_SHADOW_BOLT_VOLLEY_H  = 40070,

    SPELL_BURNING_NOVA          = 30940,
    SPELL_VORTEX                = 37370,

    // Shadowmoon Channeler
    SPELL_SHADOW_BOLT           = 12739,
    SPELL_SHADOW_BOLT_H         = 15472,

    SPELL_MARK_OF_SHADOW        = 30937,
    SPELL_CHANNELING            = 39123
};

enum KelidanMisc
{
    NPC_KELIDAN                 = 17377,
    NPC_CHANNELER               = 17653
};

Position const ShadowmoonChannelers[5] =
{
    { 301.988f, -86.7465f, -24.4517f, 0.15708f },
    { 320.750f, -63.6121f, -24.6361f, 4.88692f },
    { 345.848f, -74.4559f, -24.6402f, 3.59538f },
    { 343.584f, -103.631f, -24.5688f, 2.35619f },
    { 316.274f, -108.877f, -24.6027f, 1.25664f }
};

// 17377 - Keli'dan the Breaker
struct boss_kelidan_the_breaker : public BossAI
{
    /**
     * @brief 构造函数，初始化 Kelidan the Breaker 的 Boss AI。
     *
     * @param creature 指向 Creature 对象的指针，表示该 Boss 的实体。
     * @note 初始化时会调用 Initialize() 方法，并设置 Firenova_Timer 为 0。
     */
    boss_kelidan_the_breaker(Creature *creature) : BossAI(creature, DATA_KELIDAN_THE_BREAKER)
    {
        Initialize();
        Firenova_Timer = 0;
    }

    /**
     * @brief 初始化战斗相关的计时器和状态。
     *
     * 设置以下计时器和状态：
     * - ShadowVolley_Timer: 初始化为1000毫秒。
     * - BurningNova_Timer: 初始化为15000毫秒。
     * - Corruption_Timer: 初始化为5000毫秒。
     * - check_Timer: 初始化为0。
     * - Firenova: 初始化为false。
     */
    void Initialize()
    {
        ShadowVolley_Timer = 1000;
        BurningNova_Timer = 15000;
        Corruption_Timer = 5000;
        check_Timer = 0;
        Firenova = false;
    }

    uint32 ShadowVolley_Timer;
    uint32 BurningNova_Timer;
    uint32 Firenova_Timer;
    uint32 Corruption_Timer;
    uint32 check_Timer;
    bool Firenova;
    ObjectGuid Channelers[5];

    /**
     * 重置当前对象的状态。
     * 执行以下操作：
     * 1. 调用基类的重置逻辑。
     * 2. 初始化当前对象。
     * 3. 召唤相关的通道施法者。
     * 4. 设置对象对NPC免疫。
     * 5. 将对象的反应状态设置为被动。
     * 6. 标记对象为不可攻击。
     */
    void Reset() override
    {
        _Reset();
        Initialize();
        SummonChannelers();
        me->SetImmuneToNPC(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
    }

    /**
     * @brief 当Boss与目标单位进入战斗状态时调用。
     *
     * @param who 触发战斗的目标单位。
     *
     * 功能说明：
     * 1. 调用基类BossAI的JustEngagedWith方法，处理基础战斗逻辑。
     * 2. 播放Boss的唤醒语音（SAY_WAKE）。
     * 3. 如果Boss正在施放非近战法术，则中断这些法术。
     * 4. 启动Boss对目标的移动行为。
     */
    void JustEngagedWith(Unit *who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_WAKE);
        if (me->IsNonMeleeSpellCast(false))
            me->InterruptNonMeleeSpells(true);
        DoStartMovement(who);
    }

    /**
     * 当生物被击杀时触发的事件处理函数。
     *
     * @param victim 被击杀的生物对象（未使用，因此被注释掉）。
     * @note 有50%的概率不执行任何操作，否则会触发 SAY_SLAY 的对话。
     */
    void KilledUnit(Unit * /*victim*/) override
    {
        if (rand32() % 2)
            return;

        Talk(SAY_SLAY);
    }

    /**
     * @brief 当某个单位（who）与当前对象交互时，触发所有未进入战斗状态的 Channelers 攻击该单位。
     *
     * @param who 触发交互的单位指针。如果为 nullptr，则不会触发任何动作。
     * @note 该函数会遍历预定义的 Channelers 列表，检查每个 Channeler 是否未处于战斗状态，
     *       如果是，则通过其 AI 控制模块发起对目标单位的攻击。
     */
    void ChannelerEngaged(Unit *who)
    {
        for (uint8 i = 0; i < 5; ++i)
        {
            Creature* channeler = ObjectAccessor::GetCreature(*me, Channelers[i]);
            if (who && channeler && !channeler->IsInCombat())
                channeler->AI()->AttackStart(who);
        }
    }

    /**
     * @brief 当某个“Channeler”（引导者）死亡时调用此函数。
     *
     * 此函数检查所有引导者是否均已死亡。如果是，则：
     * 1. 将当前单位（me）设置为主动攻击状态（REACT_AGGRESSIVE）。
     * 2. 取消对NPC的免疫状态。
     * 3. 移除“不可攻击”标志（UNIT_FLAG_NON_ATTACKABLE）。
     * 4. 如果存在击杀者（killer），则开始攻击该击杀者。
     *
     * @param killer 击杀引导者的单位指针，可能为空。
     */
    void ChannelerDied(Unit *killer)
    {
        for (uint8 i = 0; i < 5; ++i)
        {
            Creature* channeler = ObjectAccessor::GetCreature(*me, Channelers[i]);
            if (channeler && channeler->IsAlive())
                return;
        }
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetImmuneToNPC(false);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
        if (killer)
            AttackStart(killer);
    }

    /**
     * @brief 获取与指定引导者相关联的另一个引导者的GUID。
     *
     * 此函数首先调用SummonChannelers()来确保所有引导者已生成，然后根据传入的引导者GUID，
     * 在预定义的引导者列表中查找匹配项，并返回列表中偏移量为2的引导者GUID。
     *
     * @param channeler1 指向需要查找的引导者Creature对象的指针。
     * @return ObjectGuid 返回与传入引导者相关联的另一个引导者的GUID；如果传入的指针为空，则返回空GUID。
     */
    ObjectGuid GetChanneled(Creature *channeler1)
    {
        SummonChannelers();
        if (!channeler1)
            return ObjectGuid::Empty;

        uint8 i;
        for (i = 0; i < 5; ++i)
        {
            Creature* channeler = ObjectAccessor::GetCreature(*me, Channelers[i]);
            if (channeler && channeler->GetGUID() == channeler1->GetGUID())
                break;
        }
        return Channelers[(i + 2) % 5];
    }

    /**
     * @brief 召唤并管理5个Channelers（施法者）。
     *
     * 该函数遍历5个位置，检查每个位置是否存在有效的Channeler。如果Channeler不存在或已死亡，
     * 则在该位置召唤一个新的Channeler。召唤成功后，更新Channelers数组中的GUID；
     * 如果召唤失败，则清除对应位置的GUID。
     *
     * @note 召唤的Channeler会在5分钟后自动消失（CORPSE_TIMED_DESPAWN）。
     */
    void SummonChannelers()
    {
        for (uint8 i = 0; i < 5; ++i)
        {
            Creature* channeler = ObjectAccessor::GetCreature(*me, Channelers[i]);
            if (!channeler || channeler->isDead())
                channeler = me->SummonCreature(NPC_CHANNELER, ShadowmoonChannelers[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
            if (channeler)
                Channelers[i] = channeler->GetGUID();
            else
                Channelers[i].Clear();
        }
    }

    /**
     * 当生物死亡时触发的事件处理函数。
     * 重写基类方法，执行死亡后的逻辑：
     * 1. 调用基类的死亡处理逻辑 `_JustDied`。
     * 2. 播放死亡时的对话文本 `SAY_DEATH`。
     *
     * @param killer 击杀者单位指针（未使用，注释掉以明确意图）。
     */
    void JustDied(Unit * /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
    }

    /**
     * @brief 更新AI逻辑，处理怪物的战斗行为。
     *
     * 该函数根据时间差（diff）更新怪物的AI行为，包括技能释放、状态检查和攻击逻辑。
     * 主要逻辑包括：
     * - 检查是否有目标（UpdateVictim），如果没有目标则尝试释放SPELL_EVOCATION。
     * - 处理Firenova状态，释放SPELL_FIRE_NOVA并重置相关计时器。
     * - 释放SPELL_SHADOW_BOLT_VOLLEY和SPELL_CORRUPTION技能，并根据计时器控制释放频率。
     * - 处理BurningNova状态，释放SPELL_BURNING_NOVA，并在英雄模式下执行传送逻辑。
     * - 最后调用DoMeleeAttackIfReady()执行近战攻击。
     *
     * @param diff 时间差（毫秒），用于更新计时器和技能冷却。
     */
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            if (check_Timer <= diff)
            {
                if (!me->IsNonMeleeSpellCast(false))
                    DoCast(me, SPELL_EVOCATION);
                check_Timer = 5000;
            }
            else
                check_Timer -= diff;
            return;
        }

        if (Firenova)
        {
            if (Firenova_Timer <= diff)
            {
                DoCast(me, SPELL_FIRE_NOVA, true);
                Firenova = false;
                ShadowVolley_Timer = 2000;
            }
            else
                Firenova_Timer -=diff;

            return;
        }

        if (ShadowVolley_Timer <= diff)
        {
            DoCast(me, SPELL_SHADOW_BOLT_VOLLEY);
            ShadowVolley_Timer = 5000 + rand32() % 8000;
        }
        else
            ShadowVolley_Timer -=diff;

        if (Corruption_Timer <= diff)
        {
            DoCast(me, SPELL_CORRUPTION);
            Corruption_Timer = 30000 + rand32() % 20000;
        }
        else
            Corruption_Timer -=diff;

        if (BurningNova_Timer <= diff)
        {
            if (me->IsNonMeleeSpellCast(false))
                me->InterruptNonMeleeSpells(true);

            Talk(SAY_NOVA);

            me->AddAura(SPELL_BURNING_NOVA, me);

            if (IsHeroic())
                DoTeleportAll(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());

            BurningNova_Timer = 20000 + rand32() % 8000;
            Firenova_Timer= 5000;
            Firenova = true;
        }
        else
            BurningNova_Timer -=diff;

        DoMeleeAttackIfReady();
    }
};

// 17653 - Shadowmoon Channeler
struct npc_shadowmoon_channeler : public ScriptedAI
{
    /**
     * @brief 构造函数，初始化 Shadowmoon Channeler NPC 的 AI 脚本。
     *
     * @param creature 指向 Creature 对象的指针，表示该 NPC 的实例。
     * @note 此构造函数会调用 Initialize() 方法完成初始化。
     */
    npc_shadowmoon_channeler(Creature *creature) : ScriptedAI(creature)
    {
        Initialize();
    }

    /**
     * @brief 初始化函数，用于设置计时器的初始值。
     *
     * 该函数为以下计时器设置随机初始值：
     * - ShadowBolt_Timer: 1000到2000毫秒之间的随机值
     * - MarkOfShadow_Timer: 5000到7000毫秒之间的随机值
     * - check_Timer: 固定为0
     */
    void Initialize()
    {
        ShadowBolt_Timer = 1000 + rand32() % 1000;
        MarkOfShadow_Timer = 5000 + rand32() % 2000;
        check_Timer = 0;
    }

    uint32 ShadowBolt_Timer;
    uint32 MarkOfShadow_Timer;
    uint32 check_Timer;

    /**
     * @brief 重置当前对象的状态。
     *
     * 该函数会调用 Initialize() 方法重新初始化对象，
     * 并检查当前对象是否正在施放非近战法术，如果是则强制中断所有非近战法术。
     *
     * @note 此函数通常用于重置战斗或状态相关的逻辑。
     */
    void Reset() override
    {
        Initialize();
        if (me->IsNonMeleeSpellCast(false))
            me->InterruptNonMeleeSpells(true);
    }

    /**
     * @brief 当生物与目标单位进入战斗状态时调用。
     *
     * 此函数在生物与目标单位进入战斗时触发，执行以下操作：
     * 1. 播放战斗喊话（SAY_AGGRO）。
     * 2. 如果附近存在NPC_KELIDAN（距离100码内），则通知其AI（boss_kelidan_the_breaker）有通道者进入战斗。
     * 3. 如果当前正在施放非近战法术，则中断这些法术。
     * 4. 开始向目标单位移动。
     *
     * @param who 触发战斗的目标单位指针。
     */
    void JustEngagedWith(Unit *who) override
    {
        Talk(SAY_AGGRO);

        if (Creature* Kelidan = me->FindNearestCreature(NPC_KELIDAN, 100))
            ENSURE_AI(boss_kelidan_the_breaker, Kelidan->AI())->ChannelerEngaged(who);
        if (me->IsNonMeleeSpellCast(false))
            me->InterruptNonMeleeSpells(true);
        DoStartMovement(who);
    }

    /**
     * @brief 当单位死亡时触发的事件处理函数。
     *
     * @param killer 击杀该单位的凶手对象。
     * @note 如果凶手不存在，则直接返回。
     * @note 如果附近存在NPC Kelidan，则通知其AI处理该死亡事件。
     */
    void JustDied(Unit *killer) override
    {
        if (!killer)
            return;

        if (Creature* Kelidan = me->FindNearestCreature(NPC_KELIDAN, 100))
            ENSURE_AI(boss_kelidan_the_breaker, Kelidan->AI())->ChannelerDied(killer);
    }

/**
 * @brief 更新AI逻辑，处理怪物的行为。
 *
 * 该函数是怪物AI的核心逻辑，负责根据时间差（diff）更新怪物的行为状态。
 * 主要功能包括：
 * - 检查是否有目标（UpdateVictim），如果没有目标，则尝试寻找并施放通道法术（SPELL_CHANNELING）。
 * - 如果有目标，则根据计时器施放暗影标记（SPELL_MARK_OF_SHADOW）和暗影箭（SPELL_SHADOW_BOLT）。
 * - 如果目标在近战范围内，则执行近战攻击（DoMeleeAttackIfReady）。
 *
 * @param diff 时间差（毫秒），用于更新计时器。
 */
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            if (check_Timer <= diff)
            {
                if (!me->IsNonMeleeSpellCast(false))
                {
                    if (Creature* Kelidan = me->FindNearestCreature(NPC_KELIDAN, 100))
                    {
                        ObjectGuid channeler = ENSURE_AI(boss_kelidan_the_breaker, Kelidan->AI())->GetChanneled(me);
                        if (Unit* channeled = ObjectAccessor::GetUnit(*me, channeler))
                            DoCast(channeled, SPELL_CHANNELING);
                    }
                }
                check_Timer = 5000;
            }
            else
                check_Timer -= diff;

            return;
        }

        if (MarkOfShadow_Timer <= diff)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                DoCast(target, SPELL_MARK_OF_SHADOW);
            MarkOfShadow_Timer = 15000 + rand32() % 5000;
        }
        else
            MarkOfShadow_Timer -=diff;

        if (ShadowBolt_Timer <= diff)
        {
            DoCastVictim(SPELL_SHADOW_BOLT);
            ShadowBolt_Timer = 5000 + rand32() % 1000;
        }
        else
            ShadowBolt_Timer -=diff;

        DoMeleeAttackIfReady();
    }
};

void AddSC_boss_kelidan_the_breaker()
{
    RegisterBloodFurnaceCreatureAI(boss_kelidan_the_breaker);
    RegisterBloodFurnaceCreatureAI(npc_shadowmoon_channeler);
}

-- 删除已存在的NPC（如果有）
DELETE FROM `creature_template` WHERE `entry` = 90000;
DELETE FROM `creature` WHERE `id` = 90000;

-- 创建NPC模板
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `dmgschool`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `DamageModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES 
(90000, 0, 0, 0, 0, 0, 7010, 0, 0, 0, 'Example NPC', 'Custom Script', NULL, 0, 80, 80, 0, 35, 1, 1, 1.14286, 1, 0, 0, 2000, 2000, 1, 1, 1, 512, 2048, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_example', 12340);

-- 添加NPC文本
DELETE FROM `creature_text` WHERE `CreatureID` = 90000;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(90000, 0, 0, 'Hello, adventurer! How can I help you today?', 12, 0, 100, 1, 0, 0, 0, 0, 'Example NPC - Hello'),
(90000, 1, 0, 'Farewell, adventurer! Safe travels!', 12, 0, 100, 1, 0, 0, 0, 0, 'Example NPC - Goodbye');

-- 在主要城市添加NPC实例
-- 删除已存在的NPC实例
DELETE FROM `creature` WHERE `id` = 90000;

-- 在暴风城添加NPC实例
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `ScriptName`, `VerifiedBuild`) VALUES
(LAST_INSERT_ID() + 1, 90000, 0, 0, 0, 1, 1, 0, 0, -8833.37, 628.62, 94.00, 1.06, 300, 0, 0, 5342, 0, 0, 0, 0, 0, '', 0);

-- 在奥格瑞玛添加NPC实例（力量谷附近）
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `ScriptName`, `VerifiedBuild`) VALUES
(LAST_INSERT_ID() + 1, 90000, 1, 0, 0, 1, 1, 0, 0, 1632.18, -4440.43, 15.59, 2.54, 300, 0, 0, 5342, 0, 0, 0, 0, 0, '', 0);
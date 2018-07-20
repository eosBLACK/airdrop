-- --------------------------------------------------------

--
-- table `snapshot-eos`
--

CREATE TABLE IF NOT EXISTS `snapshot_eos` (
  `etherAddr` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `eosID` varchar(20) COLLATE utf8mb4_unicode_ci NOT NULL,
  `eosAddr` varchar(60) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `eosBalance` varchar(30) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `symbol` varchar(10) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `processed` tinyint(1) UNSIGNED NOT NULL DEFAULT '0',
  `transactionID` text COLLATE utf8mb4_unicode_ci,
  `regDate` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`eosID`),
  KEY `processed` (`processed`),
  KEY `symbol` (`symbol`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

UPDATE `snapshot_eos` SET processed = 0, transactionID = null, regDate = null;
 
UPDATE `snapshot_eos` SET processed = 255 WHERE eosID = 'b1';


-- --------------------------------------------------------
-- --------------------------------------------------------

-- Apply "actor-blacklist" 

UPDATE `snapshot_eos` SET processed = 254 WHERE
eosID in (
-- from https://eoscorearbitration.io/wp-content/uploads/2018/07/ECAF_Arbitrator_Order_2018-06-22-AO-002.pdf
"gu2teobyg4ge","gq4demryhage","q4dfv32fxfkx","ktl2qk5h4bor","haydqnbtgene",
"g44dsojygyge","guzdonzugmge","ha4doojzgyge","gu4damztgyge","haytanjtgige",
"exchangegdax","cmod44jlp14k","2fxfvlvkil4e","yxbdknr3hcxt","yqjltendhyjp",
"pm241porzybu","xkc2gnxfiswe","ic433gs42nky","fueaji11lhzg","w1ewnn4xufob",
"ugunxsrux2a3","gz3q24tq3r21","u5rlltjtjoeo","k5thoceysinj","ebhck31fnxbi",
"pvxbvdkces1x","oucjrjjvkrom",
-- 8 rows match

-- from https://eoscorearbitration.io/wp-content/uploads/2018/07/ECAF_Arbitrator_Order_2018-06-19-AO-001.pdf
"ge2dmmrqgene","gu2timbsguge","ge4tsmzvgege","gezdonzygage","ha4tkobrgqge",
"ha4tamjtguge","gq4dkmzzhege"
-- 7 rows match
);

-- set symbol column to prevent human error.
UPDATE snapshot_eos set symbol='BLACK' where processed=0;

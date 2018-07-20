# eosBLACK Airdrop
This repository contains all the code related to the eosBLACK Airdrop.

## Contract Policy
The token used in eosBLACK is called BLACK, and BLACK will issue a total of 3,000,000,000 (3 billion).

The initial distribution structure of 3 billion BLACK is as follows.

① 900,000,000 BLACKs will Airdrop according to the following Airdrop policy.

② 300,000,000 BLACKs will be used for eosBLACK development and operation. 

③ 900,000,000 BLACKs will be locked for 12 months. It will be unlocked after 12 months and stored in the eosBLACK Reward Bank and it will be used for eosBLACK ecosystem development.

④ 900,000,000 BLACKs will be locked for 12 months and after 3 months 100,000,000 BLACKs will be unlocked  every month for 9 months, and it will be used for all activities participating in eosBLACK and its  o perations. (Some changes and adjustments can be made depending on the ecosystem operation.)

##### < Table > BLACK holding for eosBLACK

| Period | 1p	| 2p	| 3p	| 4p	| 5p	| 6p	| 7p	| 8p	| 9p	| 10p	| 11p	| 12p |
| -------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Day after	| 30d	| 60d	| 90d	| 120d	| 150d	| 180d	| 210d	| 240d	| 270d	| 300d	| 330d | 360d |
| Hold balance | 1.8billion BLACK	| 1.8billion BLACK	| 1.8billion BLACK	| 1.7billion BLACK	| 1.6billion BLACK	| 1.5billion BLACK	| 1.4billion BLACK	| 1.3billion BLACK	| 1.2billion BLACK	|1.1billion BLACK	| 1billion BLACK	| 0.9billion BLACK |


## Airdrop Policy
① Target
  - We will conduct 1: 1 Airdrop to EOS holders based on the account included in EOS genesis snapshot.

② Exclude target
  - Except Block.One ownership account, 'b1'.
  - BP blacklists announced by ECAF are excluded from Airdrop targets. (EOS genesis snapshot contains 15 accounts)
     https://eoscorearbitration.io/wp-content/uploads/2018/07/ECAF_Arbitrator_Order_2018-06-19-AO-001.pdf
     https://eoscorearbitration.io/wp-content/uploads/2018/07/ECAF_Arbitrator_Order_2018-06-22-AO-002.pdf

   - Through the above process, a total of 16 accounts including 'b1' owned by Block.One and 15 announced BP blacklists are excluded.


## Airdropping
Use eos-airdrop.js to sequentially carried out Airdrop according to the order of EOS that each account has.
Each time a transaction completes, it checks the status of the account and performs validation.

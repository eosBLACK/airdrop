
module.exports = {
    // MySQL connection info
    dbConnInfo : {
        connectionLimit : 100,
        host    : '127.0.0.1',
        user    : 'user',
        password: 'password',
        database: 'eosblack',

        tableName : 'snapshot_eos',
    },

    // EOS Network connection info
    eosConnInfo : {
        // for JungleNet
        // httpEndpoint: 'http://ayeaye.cypherglass.com:8888',
        // chainId     : '038f4b0fc8ff18a4f0842a8f0564611f6e96e8535901dd45e43ac8691a1c4dca',

        // for MainNet
        httpEndpoint: 'http://user-api.eoseoul.io',
        chainId     : 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906',

        keyProvider : [],
        sign        : true,
        //verbose     : true,
        broadcast   : true,
        expireInSeconds: 60,
    },

    // keosd wallet connection info
    walletURL : 'http://localhost:8900',

    // issuer's active permission public key
    activePublicKey : '',

    // contract and symbol
    contractInfo: {
        code: 'eosblackteam',
        symbol: 'BLACK'
    },

    // Drop and Packing count
    selectCount : 500,
    packCount   : 10,

    // Check balance on each airdrop
    checkBalance: true, 

    SEL_PRNO  : 1,
    SUCC_PRNO : 11,
    FAIL_PRNO : 12,

    CHK_SEL_PRNO  : 11,
    CHK_SUCC_PRNO : 21,
    CHK_FAIL_PRNO : 22,

    // Interval on each airdrop
    postInterval: 500,

    memo        : 'Creating value together! eosBLACK:http://eosblack.io',

}

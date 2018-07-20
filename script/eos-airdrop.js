/*

eosBLACK Airdrop.

*/

const Eos     = require('eosjs');
const MySQL   = require('mysql');
const Config  = require('./config');
const read    = require('read');
const request = require('request');
//const ecc     = require('eosjs-ecc');



// mysql function
function mysql_createConnection(dbConnInfo) {
    return new Promise((resolve, reject) => {
        const sqlConnection = MySQL.createConnection(dbConnInfo);
        sqlConnection.connect(function (err) {
            if (err) reject(err)
            else resolve(sqlConnection);
        })
    });
}

function mysql_query(sqlConnection, sql) {
    return new Promise((resolve, reject) => {
        sqlConnection.query(sql, function(err, result, fields) {
            if (err) reject(err)
            else resolve(result); 
        })
    })
}

// wait console input
function getInput(prompt, silent) {
    return new Promise((resolve, reject) => {
        read({ prompt: prompt, silent: silent, replace: '*' }, function(err, password) {
            if (err) reject(err);
            else resolve(password);
        })
    })
}


function getChainInfo() {
    return new Promise((resolve, reject) => {
        const options = {
            method: "GET",
            url: Config.eosConnInfo.httpEndpoint + "/v1/chain/get_info",
        }
    
        request(options, function (err, res, body) {
            if (err) reject(err);
            else resolve(JSON.parse(body));
        })
    })
}


function getAccount(account_name) {
    return new Promise((resolve, reject) => {
        const body = {
            account_name: account_name
        };

        const options = {
            method: "POST",
            url: Config.eosConnInfo.httpEndpoint + "/v1/chain/get_account",
            body: JSON.stringify(body)
        }
    
        request(options, function (err, res, body) {
            if (err) reject(err);
            else resolve(JSON.parse(body));
        })
    })
}

// Return active permission's first public key. 
// This script don't produce multi sign. 
function getActivePermission(account_info) {
    if (!account_info.permissions) return; 

    
    for (var i=0; i<account_info.permissions.length; i++) {
        var perm = account_info.permissions[i];

        if (perm.perm_name === 'active') {
            return perm.required_auth.keys[0].key; 
        }
    }
}

function getCurrencyStats(code, symbol) {
    return new Promise((resolve, reject) => {
        const body = {
            //json    : false, 
            code    : code, 
            symbol  : symbol
        }

        const options = {
            method: "POST",
            url: Config.eosConnInfo.httpEndpoint + "/v1/chain/get_currency_stats",
            body: JSON.stringify(body) 
        }

        request(options, function (err, res, body) {
            if (err) reject(err);
            else resolve(JSON.parse(body));
        })
    })    
}




function getWalletKeyList() {
    return new Promise((resolve, reject) => {
        const options = {
            method: "POST",
            url: Config.walletURL + "/v1/wallet/get_public_keys",
        }

        request(options, function (err, res, body) {
            if (err) reject(err);
            else {
                resolve(JSON.parse(body));
            }
        })
    })    
}

async function getTotalAirdropCount() {
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);
    var query =
        `SELECT COUNT(*) AS Count
        FROM \`${Config.dbConnInfo.tableName}\` 
        WHERE \`eosID\` != 'b1' 
        AND \`symbol\` = '${Config.contractInfo.symbol}'
        AND \`processed\` = 0`;    
    var db_ret = await mysql_query(dbConnection, query);
    dbConnection.end(); 

    if (db_ret.length > 0) 
        return db_ret[0].Count;
    else
        return 0; 
}

// select airdrop accounts
async function selectAirdrops(count) {
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);
    var query = 
        `SELECT \`eosID\`, \`eosBalance\`
        FROM \`${Config.dbConnInfo.tableName}\` 
        WHERE \`eosID\` != 'b1' 
        AND \`processed\` = 0
        AND \`symbol\` = '${Config.contractInfo.symbol}'
        ORDER BY (\`eosBalance\` + 0) DESC
        LIMIT ${count}`;

    var db_ret = await mysql_query(dbConnection, query);
    dbConnection.end(); 

    var airdrops = {
        totalBalance : 0,
        datas: []
    }

    var packIndex = 0; 
    var infos = new Array(); 

    for (var i=0; i< db_ret.length; i++) {
        airdrops.totalBalance = airdrops.totalBalance + parseFloat(db_ret[i].eosBalance);

        infos.push(
            {
                destAccount : db_ret[i].eosID,
                sendAmount  : db_ret[i].eosBalance
            }
        )

        packIndex ++; 

        if (packIndex >= Config.packCount) {
            airdrops.datas.push(infos);
            packIndex = 0; 
            infos = new Array(); 
        }
    }

    if (infos.length > 0) {
        airdrops.datas.push(infos);
    }
    airdrops.totalBalance = airdrops.totalBalance.toFixed(4); 


    return airdrops; 
}


function makeIssueActions(dropData, account_name) {
    var actions = []; 

    for (var i=0; i< dropData.length; i++) {
        var action = {
            account : Config.contractInfo.code,
            name    : 'issue',
            authorization : [{
                actor: account_name,
                permission: 'active'
            }],
            data: {
                to: dropData[i].destAccount,
                quantity: `${dropData[i].sendAmount} ${Config.contractInfo.symbol}`,
                memo: Config.memo
            }
        };        

        actions.push(action); 
    }

    return actions; 
}

function getCurrencyBalance(code, account_name, symbol) {

    return new Promise((resolve, reject) => {
        const body = {
            account : account_name, 
            code    : code, 
            symbol  : symbol
        }

        const options = {
            method: "POST",
            url: Config.eosConnInfo.httpEndpoint + "/v1/chain/get_currency_balance",
            body: JSON.stringify(body) 
        }

        request(options, function (err, res, body) {
            if (err) reject(err);
            else resolve(JSON.parse(body));
        })
    })
}

async function doSaveResult(dropData, txID) {
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);
    for (var i=0; i<dropData.length; i++) {
        var query = 
            `UPDATE \`${Config.dbConnInfo.tableName}\`
            SET
                processed = ${Config.SEL_PRNO},
                transactionID = "${txID}",
                regDate = CURRENT_TIMESTAMP
            WHERE
                eosID = "${dropData[i].destAccount}"`;

        await mysql_query(dbConnection, query);
    }
    dbConnection.end(); 

}

async function checkDropResult(dropData) {
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);
    var retValue = true; 
    for (var i=0; i<dropData.length; i++) {
        var flag = Config.FAIL_PRNO; 
        var dropValue = await getCurrencyBalance(Config.contractInfo.code, dropData[i].destAccount, Config.contractInfo.symbol);

        if ((dropValue.length > 0) && (dropValue[0].split(' ')[0] === dropData[i].sendAmount)) {
            flag = Config.SUCC_PRNO; 
        } else {
            retValue = false; 
        }

        var query = 
            `UPDATE \`${Config.dbConnInfo.tableName}\`
            SET
                processed = ${flag}
            WHERE
                eosID = "${dropData[i].destAccount}"`;
        
        await mysql_query(dbConnection, query);

    }
    dbConnection.end(); 

    return retValue; 

}


var eos = null; 

async function signProvider(buf, sign) {

    return new Promise((resolve, reject) => {
        const body = [
            buf.transaction, 
            [Config.activePublicKey],
            Config.eosConnInfo.chainId
        ];

        const options = {
            method: "POST",
            url: Config.walletURL + "/v1/wallet/sign_transaction",
            body: JSON.stringify(body) 
        }

        request(options, function (err, res, body) {
            if (err) reject(err);
            else {
                resolve(JSON.parse(body).signatures);
            }
        })
    })    

}



async function makeTransactionHeader(expireInSeconds, callback) {
    var info = await eos.getInfo({});
    chainDate = new Date(info.head_block_time + 'Z')
    expiration = new Date(chainDate.getTime() + expireInSeconds * 1000)
    expiration = expiration.toISOString().split('.')[0]

    var block = await eos.getBlock(info.last_irreversible_block_num);
    var header = {
        expiration: expiration,
        ref_block_num: info.last_irreversible_block_num & 0xFFFF,
        ref_block_prefix: block.ref_block_prefix,

        net_usage_words: 0,
        max_cpu_usage_ms: 0,
        delay_sec: 0,
        context_free_actions: [],
        actions: [],
        signatures: [],
        transaction_extensions: []
    }

    console.log(`ref_block_num   :\t ${header.ref_block_num} (${info.last_irreversible_block_num})`);
    console.log(`ref_block_prefix:\t ${block.ref_block_prefix}`);

    callback(null, header);
}



function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function doAirdrop(airdrops, account_name) {
    for (var i=0; i<airdrops.datas.length; i++) {
        var succ = true; 
        var actions = makeIssueActions(airdrops.datas[i], account_name)
        console.log('--------------------')
        console.log(`Step ${i+1}/${airdrops.datas.length}. count ${actions.length}`);

        await eos.transaction({actions})
        .then(async (ret) => {
            //console.log(ret);
            await doSaveResult(airdrops.datas[i], ret.transaction_id);
            console.log('transactoin_id: ' + ret.transaction_id); 

            if (Config.checkBalance) {
                await sleep(Config.postInterval);

                if (await checkDropResult(airdrops.datas[i])) {
                    console.log('balance check success!!!')

                } else {
                    console.log('drop balance check fail!!!')

                    succ = false; 
                }
            } else
                console.log('success!!!')
        })
        .catch((err) => {
            console.log('error!!!')
            console.log(err); 
            succ = false; 
        })

        if (!succ) break; 

        if (!Config.checkBalance) 
            await sleep(Config.postInterval);
    }

    Config.eosConnInfo.keyProvider = [];
}

// Precision to 4 with commas
function balanceStrWithCommas(ns) {
    var parts=ns.split(".");
    return parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",") + (parts[1] ? "." + parts[1] : "");
}

function balanceWithCommas(n) {
    var parts=n.toFixed(4).split(".");
    return parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",") + (parts[1] ? "." + parts[1] : "");
}

function numberWithCommas(n) {
    var parts=n.toString().split(".");
    return parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",") + (parts[1] ? "." + parts[1] : "");
}


async function main() {
    console.log('');
    console.log(`************************************`);
    console.log(`** Airdrop ${Config.contractInfo.symbol} (${Config.contractInfo.code})`)
    console.log(`************************************`);
    console.log('');

    // Check chain id
    const chainInfo = await getChainInfo(); 
    if (chainInfo.chain_id !== Config.eosConnInfo.chainId) {
        console.error(' ERR> Check chain_id!');
        return; 
    }

    // Check currency status
    var stats = await getCurrencyStats(Config.contractInfo.code, Config.contractInfo.symbol);
    stats = stats[`${Config.contractInfo.symbol}`];

    if (!stats) {
        console.error(` ERR> Check currency stats of ${Config.contractInfo.symbol}`);
        return; 
    }

    const max_supply = stats.max_supply.split(' ')[0];
    const cur_supply = stats.supply.split(' ')[0];
    const ena_supply = max_supply - cur_supply; 
    var account_name = stats.issuer; 

    console.log(`issuer \t ${stats.issuer}`);
    console.log(`max    \t ${balanceStrWithCommas(max_supply)} ${Config.contractInfo.symbol}`);
    console.log(`supply \t ${balanceStrWithCommas(cur_supply)} ${Config.contractInfo.symbol}`);
    console.log(`enable \t ${balanceWithCommas(ena_supply)} ${Config.contractInfo.symbol}`);// ena_supply.toFixed(4));
    console.log('');

    ///*
    // Check account and permission
    var tmp_name = await getInput(`Account (${account_name}): `, false);
    if (tmp_name !== '') account_name = tmp_name; 

    const account_info = await getAccount(account_name);
    if (account_info.code) {
        console.error(` ERR> Received ${account_info.code} err. Check account name!`);
        return; 

    }

    Config.activePublicKey = getActivePermission(account_info);
     
    var keyList = await getWalletKeyList();

    if (!keyList || (keyList.length ==0)) {
        console.error(` ERR> keosd doesn't seem to be open!`);
        return; 
    }

    var keyFound = false; 
    for (var i=0; i<keyList.length; i++) {
        if (Config.activePublicKey === keyList[i]) {
            keyFound = true; 
            break; 
        }
    }

    if (!keyFound) {
        console.error(` ERR> keosd doesn't have the proper key!`);
        return; 
    }



    var totalCount = await getTotalAirdropCount(); 

    var tempCount = await getInput(`How many accounts do you want to select? (Default ${Config.selectCount} of ${numberWithCommas(totalCount)}): `, false); 
    if (tempCount !== '') Config.selectCount = parseInt(tempCount); 

    var tempCount = await getInput(`How many will you send in one transaction? (Default ${Config.packCount}): `, false);
    if (tempCount !== '') Config.packCount = parseInt(tempCount); 

    var airdrops = await selectAirdrops(Config.selectCount);
    console.log(`\nTotal airdrop balance: ${balanceStrWithCommas(airdrops.totalBalance)} ${Config.contractInfo.symbol}`);

    if (airdrops.totalBalance > ena_supply) {
        console.log('  ERR> You have insuficient supply. Stop processing.');
        return; 
    }

    var continueYN = await getInput(`\nDo you want to start airdrop? (Y/n): `, false);
    if (continueYN !== '')
        if (continueYN.toUpperCase() === 'N') 
            return; 


    Config.eosConnInfo.signProvider = signProvider; 
    Config.eosConnInfo.transactionHeaders = makeTransactionHeader;
    eos = Eos.Localnet(Config.eosConnInfo);
        
    var startTime = Date.now();

    await doAirdrop(airdrops, account_name); 

    var endTime = Date.now();

    console.log('\nStart: ' + startTime);
    console.log('Finish: ' + endTime);
    var millis = endTime - startTime;

    console.log("seconds elapsed = " + Math.floor(millis/1000));
    console.log(''); 

}


main(); 


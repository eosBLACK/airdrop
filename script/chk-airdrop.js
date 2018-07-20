/*
    Verify airdrop

*/

const MySQL   = require('mysql');
const Config  = require('./config');
const read    = require('read');
const request = require('request');


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

async function selectInfo(count) {
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);
    var query = 
        `SELECT \`eosID\`, \`eosBalance\` 
        FROM \`${Config.dbConnInfo.tableName}\` 
        WHERE \`eosID\` != 'b1' 
        AND \`processed\` = ${Config.CHK_SEL_PRNO}
        LIMIT ${count}`;

    var db_ret = await mysql_query(dbConnection, query);
    dbConnection.end(); 

    return db_ret; 


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

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function verifyAirdrop() {
    var chkCount = await getInput(`Check count (100): `, false); 
    if (chkCount === '') chkCount = 100; 

    var chkList = await selectInfo(chkCount);
    console.log(`Select count: ${chkList.length}`);

    var succCount = 0; 
    var failCount = 0; 
    var dbConnection = await mysql_createConnection(Config.dbConnInfo);

    for (var i=0; i< chkList.length; i++) {
        var dropValue = await getCurrencyBalance(Config.contractInfo.code, chkList[i].eosID, Config.contractInfo.symbol);
        var flag = Config.CHK_FAIL_PRNO; 

        if ((dropValue.length > 0) && (dropValue[0].split(' ')[0] === chkList[i].eosBalance)) {
            flag = Config.CHK_SUCC_PRNO; 
            succCount++; 
        } else
            failCount++; 

        var query = 
            `UPDATE \`${Config.dbConnInfo.tableName}\`
            SET
                processed = ${flag}
            WHERE
                eosID = "${chkList[i].eosID}"`;
        
        await mysql_query(dbConnection, query);

        if ((i > 0) && (i % 10 == 0)) {
            console.log(`${i+1}/${chkList.length} processed`)
        }

        await sleep(1); 

    }

    dbConnection.end(); 
    console.log('\nFinish!!');
    console.log(`Succ: ${succCount}\nFail: ${failCount}`)
}

verifyAirdrop(); 
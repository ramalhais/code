// https://medium.com/blockthought/creating-bitcoin-wallets-in-js-69c0773c2954
// http://gobittest.appspot.com/Address
// http://gobittest.appspot.com/PrivateKey

const { exit } = require('process');

const sha256 = require('js-sha256');
const ripemd160 = require('ripemd160');
const secureRandom = require('secure-random');
const ec = require('elliptic').ec;
const base58 = require('bs58');

function sleep(seconds){
    var waitUntil = new Date().getTime() + seconds*1000;
    while(new Date().getTime() < waitUntil) true;
}

function genPrivate() {

    const max = Buffer.from("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140", 'hex');
    let isInvalid = true;
    let privateKey;
    while (isInvalid) {
        privateKey = secureRandom.randomBuffer(32);
/*         r = Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8) +
            Math.random().toString(8).substr(2, 8);
        privateKey = Buffer.from(r, "hex");
        console.log(privateKey);
        console.log(max);*/
        if (Buffer.compare(max, privateKey) === 1) {
            isInvalid = false;
        }
/*         sleep(1); */
    }

    return privateKey;
}

function getPublic(pk) {
    const ecdsa = new ec('secp256k1');
    const keys = ecdsa.keyFromPrivate(pk);
    const publicKey = keys.getPublic('hex');

    return publicKey;
}

function getPubHash(pubk) {
    let hash = sha256(Buffer.from(pubk, 'hex'));
    let publicKeyHash = new ripemd160().update(Buffer.from(hash, 'hex')).digest();

    return publicKeyHash;
}

function createPublicAddress(publicKeyHash) {
    // step 1 - add prefix "00" in hex
    const step1 = Buffer.from("00" + publicKeyHash.toString('hex'), 'hex');
    // step 2 - create SHA256 hash of step 1
    const step2 = sha256(step1);
    // step 3 - create SHA256 hash of step 2
    const step3 = sha256(Buffer.from(step2, 'hex'));
    // step 4 - find the 1st byte of step 3 - save as "checksum"
    const checksum = step3.substring(0, 8);
    // step 5 - add step 1 + checksum
    const step4 = step1.toString('hex') + checksum;
    // return base 58 encoding of step 5
    const address = base58.encode(Buffer.from(step4, 'hex'));
    return address;
}

function createPrivateKeyWIF(privateKey) {
    const step1 = Buffer.from("80" + privateKey, 'hex');
    const step2 = sha256(step1);
    const step3 = sha256(Buffer.from(step2, 'hex'));
    const checksum = step3.substring(0, 8);
    const step4 = step1.toString('hex') + checksum;
    const privateKeyWIF = base58.encode(Buffer.from(step4, 'hex'));
    return privateKeyWIF;
}

function getBalance(address,cb) {
    const https = require('https');

    let url = "https://api.blockchain.info/haskoin-store/btc/address/" + address + "/balance";
    //let url = "https://blockchain.info/rawaddr/" + address + "&limit=1";
//    console.log("> URL: " + url);

    https.get(url, (resp) => {
        let data = '';

        resp.on('data', (chunk) => {
          data += chunk;
        });

        resp.on('end', () => {
          cb(data);
        });

    }).on("error", (err) => {
        console.log("Error: " + err.message);
    });
}

n = 1//0xfbc
async function tryRandomAddress() {
    pk = genPrivate();
/*    str = n.toString(16).padStart(64, '0');
    n=n*2+1;
    pk = Buffer.from(str, 'hex');*/
//    console.log('> Private key: ', pk.toString('hex'));

//    pkwif = createPrivateKeyWIF(pk);
//    console.log('> Private key WIF: ', pkwif.toString('hex'));

    pubk = getPublic(pk);
//    console.log('> Public key: ', pubk);

    pubhash = getPubHash(pubk);
//    console.log('> Public Hash: ', pubhash);

    address = createPublicAddress(pubhash);

    getBalance(address, (resp) => {
        obj = JSON.parse(resp);
//        if (false || (obj.received != 0 || obj.utxo != 0 || obj.txs != 0 || obj.unconfirmed != 0 || obj.confirmed != 0)) {
        if (obj.unconfirmed != 0 || obj.confirmed != 0 /*|| n % 0xff == 0*/) {
            console.log('> Private key: ', pk.toString('hex'));
            console.log('> Address: ', obj.address);
            console.log('> JSON: ', obj);
        } else {
            process.stdout.write(".");
            //sleep(.1);
        }
        tryRandomAddress();
    });
}

tryRandomAddress();

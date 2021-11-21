var bitcore = require('bitcore-lib');
var EthereumBip44 = require('ethereum-bip44');
var EC = require('elliptic').ec;
const ec = new EC('secp256k1');

//var key = bitcore.HDPrivateKey();
//var keydata = "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141";
var keydata =   "0000000000000000000000000000000000000000000000000000000000000001";

//var key = ec.keyFromPrivate(keydata, 16);

var key = bitcore.PrivateKey(keydata);
// create the hd wallet
var wallet = new EthereumBip44(key);
console.log(wallet);
// output the first address
console.log(wallet.getAddress(0));
// output the second address
console.log(wallet.getAddress(1));
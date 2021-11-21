require('https').get('https://api.blockchain.info/haskoin-store/btc/address/3QsnHTY38SFVJiCGZqxMxJbz7xhfjCdvji/balance', (res) => {
    res.setEncoding('utf8');
    res.on('data', function (body) {
        console.log(JSON.parse(body));
    });
});

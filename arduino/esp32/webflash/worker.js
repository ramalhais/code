var module;

async function init()  {
    // const module = await import('https://cdn.jsdelivr.net/npm/@yowasp/yosys/gen/bundle.js');
    module = await import('https://cdn.jsdelivr.net/npm/@yowasp/openfpgaloader@0.3.1-914.18/gen/bundle.js');
}

init();

self.onmessage = function(event) {
    console.log('Message from main:', event.data);
    if (event.data == "flash")
        flash();
};

async function usbList() {
    console.log("Listing USB devices");
    const devices = await navigator.usb.getDevices();
    for (const device of devices) {
        console.log("Device: %o", device);            
    }
}

function help() {
    module.runOpenFPGALoader(["--help"]);
    // module.runYosys(["--version"]);
    // module.runYosys(["-m openFPGALoader"]);
}

function usbScan() {
    console.log("OpenFPGALoader Scanning USB devices");
    module.runOpenFPGALoader(["--scan-usb"]);
}

async function flash() {
    const fileUrl = 'next_kms.fs';
    var fileContent;

    help();
    usbList();
    usbScan();

    await fetch(fileUrl)
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.arrayBuffer();
        })
        .then(arrayBuffer => {
            const uint8Array = new Uint8Array(arrayBuffer);
            fileContent = uint8Array;
            console.log('Length of Uint8Array:', uint8Array.length);
        })
        .catch(error => {
            console.error('There was a problem fetching the file:', error);
        });

    const moduleArgs = ["-b", "tangnano9k", "-v", "-f", "data.fs"];
    const filesData = { "data.fs": fileContent };

    console.log("OpenFPGALoader Flashing");
    module.runOpenFPGALoader(args=moduleArgs, filesIn=filesData);
}

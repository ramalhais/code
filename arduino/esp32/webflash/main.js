// Create a new worker from a JavaScript file
const worker = new Worker('worker.js');

// Handle messages sent from the worker
worker.onmessage = function(event) {
  console.log('Message from worker:', event.data);
};

var text = document.createElement("div");
text.setAttribute("id", "text");
if (crossOriginIsolated) {
  // SharedArrayBuffer is available
  text.innerHTML = 'On Windows, use <a target="_blank" href="https://zadig.akeo.ie/">Zadig</a> app to change the driver of "JTAG Debugger (Interface 0)" to WinUSB';
} else {
  text.innerHTML = "crossOriginIsolated is false. SharedArrayBuffer is not available. Add headers: Cross-Origin-Opener-Policy: same-origin and Cross-Origin-Embedder-Policy: require-corp";
}
document.body.appendChild(text);

var buttonSelectDevice = document.createElement("button");
buttonSelectDevice.setAttribute("id", "buttonSelectDevice");
buttonSelectDevice.setAttribute('type','button');
buttonSelectDevice.textContent = "Select device to flash";
if (crossOriginIsolated)
  document.body.appendChild(buttonSelectDevice);

// Add styles to the button
buttonSelectDevice.style.padding = "10px 20px"; // Add padding
buttonSelectDevice.style.fontSize = "16px"; // Set font size
buttonSelectDevice.style.backgroundColor = "#007bff"; // Set background color
buttonSelectDevice.style.color = "#fff"; // Set text color
buttonSelectDevice.style.border = "none"; // Remove border
buttonSelectDevice.style.borderRadius = "5px"; // Add border radius

let filters = []; // vendorId, productId, classCode, subclassCode, protocolCode, serialNumber
// filters.push({ 'vendorId': 0x0403 });

buttonSelectDevice.addEventListener("click", async () => {
  await navigator.usb
  .requestDevice({ filters: filters })
  .then((usbDevice) => {
    console.log('Selected Device: ${usbDevice.productName}');
    worker.postMessage('flash');
  })
  .catch((e) => {
    console.error(e);
  });
});

var initialized = false;
var messageQueue = [];

Pebble.addEventListener("ready",
  function(e) {
    config = JSON.parse(localStorage.getItem("config") || "{}");
    config["routines"] = config["routines"] || [];
    console.log("JavaScript app ready and running!");
    initialized = true;
    sendConfig(config);
  }
);

Pebble.addEventListener("showConfiguration",
  function() {
    var uri = "https://samuelmr.github.io/pebble-workout/configure.html#"+
    encodeURIComponent(JSON.stringify(config));
    console.log("Configuration url: " + uri);
    Pebble.openURL(uri);
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var config = JSON.parse(decodeURIComponent(e.response));
    console.log("Webview window returned: " + JSON.stringify(config));
    sendConfig(config);
    localStorage.setItem("config", JSON.stringify(config));
  }
);

function sendConfig(config) {
  var msg = {};
  config["routines"] = config["routines"] || []; // just in case
  msg["0"] = config["work"];
  msg["1"] = config["rest"];
  msg["2"] = config["repeat"];
  msg["3"] = config["routines"].length;
  for (var i=0; i<config["routines"].length; i++) {
    msg[i+4] = config["routines"][i];
  }
  messageQueue.push(msg);
  sendNextMessage();
}

function sendNextMessage() {
  if (messageQueue.length > 0) {
    Pebble.sendAppMessage(messageQueue[0], appMessageAck, appMessageNack);
    console.log("Sent message to Pebble! " + JSON.stringify(messageQueue[0]));
  }
}

function appMessageAck(e) {
  console.log("Message accepted by Pebble!");
  messageQueue.shift();
  sendNextMessage();
}

function appMessageNack(e) {
  console.log("Message rejected by Pebble! " + e.error);
}


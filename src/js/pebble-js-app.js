var initialized = false;
var messageQueue = [];

Pebble.addEventListener("ready",
  function(e) {
    config = JSON.parse(localStorage.getItem("config") || "{}");
    config["work"] = parseInt(config["work"]) || 90;
    config["rest"] = parseInt(config["rest"]) || 30;
    config["repeat"] = parseInt(config["repeat"]) || 4;
    config["routines"] = config["routines"] || ["Push-ups", "Sit-ups", "Lunges", "Pull-ups"];
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
  msg["0"] = parseInt(config["work"]) || 90;
  msg["1"] = parseInt(config["rest"]) || 30;
  msg["2"] = parseInt(config["repeat"]) || 4;
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


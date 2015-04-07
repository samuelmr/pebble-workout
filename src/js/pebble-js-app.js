var initialized = false;
var messageQueue = [];

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
  // console.log("options not sent to Pebble: " + e.error.message);
  console.log("Message rejected by Pebble! " + e.error);
  // console.log("type: " + e.type);
  // console.log("return: " + e.returnValue);
  // console.log("eventPhase: " + e.eventPhase);
  // var msg = '';
  // for (i in e) {
  //   if (i=='timeStamp' || i.toLowerCase().indexOf('bubble') >= 0 ||
  //       i.toLowerCase().indexOf('target') >= 0 ||
  //       i.toLowerCase().indexOf('prevent') >= 0 ||
  //       i.toLowerCase().indexOf('propag') >= 0 ||
  //       i.toLowerCase().indexOf('mouse') >= 0 ||
  //       i.toLowerCase().indexOf('click') >= 0 ||
  //       i.toLowerCase().indexOf('initevent') >= 0 ||
  //       i.toLowerCase().indexOf('key') >= 0 ||
  //      i.toLowerCase().indexOf('cancelable') >= 0) {
  //     continue;
  //   }
  //   msg += i + ';';
  //   // msg += i + ': ' + e.data[i] + '; ';
  // }
  // console.log("error: " + e.error);
  // msg = '';
  // for (i in e.payload) {
  //   msg += i + ': ' + e.payload[i] + '; ';
  // }
  // console.log("payload: " + msg);
}

Pebble.addEventListener("ready",
  function(e) {
    console.log("JavaScript app ready and running!");
    initialized = true;
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var options = JSON.parse(decodeURIComponent(e.response));
    console.log("Webview window returned: " + JSON.stringify(options));
    var time = options["0"].split(":");
    var hours = 0;
    if (time.length == 3) {
     hours = parseInt(time[0]);
    }
    var minutes = parseInt(time[time.length-2]);
    var seconds = parseInt(time[time.length-1]);
    var timeopts = {'0': 'time',
                    '1': hours,
                    '2': minutes,
                    '3': seconds};
    console.log("options formed: " + hours + ':' + minutes + ':' + seconds);
    messageQueue.push(timeopts);
    messageQueue.push({'0': 'long', '1': options["1"]});
    messageQueue.push({'0': 'single', '1': options["2"]});
    messageQueue.push({'0': 'double', '1': options["3"]});
    sendNextMessage();
  }
);

Pebble.addEventListener("showConfiguration",
  function() {
    var uri = "https://rawgithub.com/samuelmr/pebble-countdown/master/configure.html";
    console.log("Configuration url: " + uri);
    Pebble.openURL(uri);
  }
);

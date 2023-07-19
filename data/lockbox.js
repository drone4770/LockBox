var values = {};
var document;

function init(documentparam) {
  document = documentparam;
}

function setCookie(cname, cvalue, exdays) {
  const d = new Date();
  d.setTime(d.getTime() + (exdays*24*60*60*1000));
  let expires = "expires="+ d.toUTCString();
  document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

function getCookie(cname) {
  let name = cname + "=";
  let decodedCookie = decodeURIComponent(document.cookie);
  let ca = decodedCookie.split(';');
  for(let i = 0; i <ca.length; i++) {
    let c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}

function getPin() {
  values.pin = document.getElementById("Pin").value;
  if (values.pin =="") {
    alert('PIN is missing!!');
  }
  setCookie('pin', values.pin, 365);
}

function setPin() {
  values.pin = document.getElementById("Pin").value = getCookie('pin');
}

function ShowChangePin() {
  if (document.getElementById("changePinRow").style.display == "none") {
    document.getElementById("changePinRow").style.display = "block";
  } else {
    document.getElementById("changePinRow").style.display = "none";
  }
}
function cancelChangePin() {
  document.getElementById("changePinRow").style.display = "none";
}

function changePin() {
  let ajaxRequest = new XMLHttpRequest;
  let status = 0;
  let oldPin =  document.getElementById("oldPin").value;
  let newPin =  document.getElementById("newPin").value;
  let newPin2 =  document.getElementById("newPin2").value;
  if (newPin == newPin2 && !isNaN(newPin)) {
    ajaxRequest.open("GET", "changePin?pin=" + oldPin + "&newpin=" + newPin, !0);
    ajaxRequest.timeout = 3500;
    ajaxRequest.ontimeout = function(e) {
      alert('Failed to connect to LockBox to change PIN');
    };
    ajaxRequest.onreadystatechange = function() {
      if (ajaxRequest.readyState === XMLHttpRequest.DONE) {
        if (200 == ajaxRequest.status) {
          values.pin = document.getElementById("Pin").value = newPin;
          setCookie('pin', values.pin, 365);
          document.getElementById("changePinRow").style.display = "none";
          document.getElementById("oldPin").value = "";
          document.getElementById("newPin").value = "";
          document.getElementById("newPin2").value = "";
          alert('PIN changed');
        } else if (403 == ajaxRequest.status) {
          alert('Old PIN is not valid');
        } else {
          alert('Unknown error');
        }
      }
    };
    try {
      ajaxRequest.send();
    } catch (e) {
      alert('Failed to connect to LockBox to change PIN');
    }
  } else {
    alert('New PIN Mismatch or not digits');
  }
}

function changeTimerMode () {
  getPin();
  var e;
  var t = 1e3 * values.seconds + 60 * values.minutes * 1e3 + 60 * values.hours * 60 * 1e3;
  if (t <= 864e5) {
    e = null;
    if (values.lockstate == 3) {
      (e = new XMLHttpRequest).open("GET", "setTimerOnce?pin=" + values.pin + "&time=" + t, !0);
    } else {
      (e = new XMLHttpRequest).open("GET", "setTimer?pin=" + values.pin + "&time=" + t, !0);
    }
    e.send();
    refreshTimer();
  }
}


function adjustmin ( min) {
  getPin();
  var e;
  values.minutes += min;

  var t = 1e3 * values.seconds + 60 * values.minutes * 1e3 + 60 * values.hours * 60 * 1e3;
  if (t <= 864e5) {
    e = null;
    if (values.lockstate == 3) {
      (e = new XMLHttpRequest).open("GET", "setTimer?pin=" + values.pin + "&time=" + t, !0);
    } else {
      (e = new XMLHttpRequest).open("GET", "setTimerOnce?pin=" + values.pin + "&time=" + t, !0);
    }
    e.send();
    refreshTimer();
  }
}

function plus1h () {
  adjustmin( 60);
}

function minus1h () {
  adjustmin( -60);
}

function plus30min () {
  adjustmin( +30);
}

function minus30min () {
  adjustmin( -30);
}

function plus5min () {
  adjustmin( +5);
}

function minus5min () {
  adjustmin( -5);
}

function plus5h () {
  adjustmin( 300);
}

function minus5h () {
  adjustmin( -300);
}

function add5Hours() {
  if (values.lockstate < 3) {
    values.settingTimer = 1;
    if (values.hours < 24) {
      values.hours += 5;
    } else {
      values.hours = 0;
    }
    if (24 == values.hours) values.minutes = 0;
    refreshTimer();
  }
}

function addHour() {
  if (values.lockstate < 3) {
    values.settingTimer = 1;
    if (values.hours < 24) {
      values.hours += 1;
    } else {
      values.hours = 0;
    }
    if (24 == values.hours) values.minutes = 0;
    refreshTimer();
  }
}

function addMinute() {
  if (values.lockstate < 3) {
    values.settingTimer = 1;
    if (values.minutes < 60 && values.hours < 24) {
      values.minutes += 5;
    } else {
      values.minutes = 0;
      addHour();
    }
    refreshTimer();
  }
}

function add30Minute() {
  if (values.lockstate < 3) {
    values.settingTimer = 1;
    if (values.minutes < 60 && values.hours < 24) {
      values.minutes += 30;
    } else {
      values.minutes = 0;
      addHour();
    }
    refreshTimer();
  }
}

function resetTimer() {
  var e;
  if (values.lockstate < 3) {
    values.init = 1;
    values.seconds = values.minutes = values.hours = 0;
    refreshTimer();
    values.settingTimer = 0;
  }
}

function refreshTimer() {
  console.log('refreshTimer', values.hours, values.minutes, values.seconds);
  document.getElementById("timer").innerHTML = values.hours.toString().padStart(2, 0) + "h " + values.minutes.toString().padStart(2, 0) + "min " + values.seconds.toString().padStart(2, 0) + "sec";
  if (values.hours > 0 || values.minutes > 0 || values.second > 0) {
    document.getElementById("buttonReset").className = "buttonReset";
    document.getElementById("buttonStart").className = "buttonStart";
    document.getElementById("buttonStartOnce").className = "buttonStart";
    if ( values.lockstate > 2 ) {
      document.getElementById("rowStartTimer").style.display = "none";
    } else {
      document.getElementById("rowStartTimer").style.display = "block";
    }
  } else {
    document.getElementById("buttonReset").className = "buttonInactive";
    document.getElementById("buttonStart").className = "buttonInactive";
    document.getElementById("buttonStartOnce").className = "buttonInactive";
    document.getElementById("rowStartTimer").style.display = "none";
  }
}

function Once() {
  getPin();
  var e = null;
  (e = new XMLHttpRequest).open("GET", "once?pin=" + values.pin, !0);
  e.send();
}
function Lock() {
  getPin();
  var e = null;
  (e = new XMLHttpRequest).open("GET", "lock?pin=" + values.pin, !0);
  e.send();
}
function Free() {
  getPin();
  var e = null;
  (e = new XMLHttpRequest).open("GET", "free?pin=" + values.pin, !0);
  e.send();
}

function startTimer() {
  getPin();
  values.intervalAlive = null;
  values.active = 0;
  var e, t = 1e3 * values.seconds + 60 * values.minutes * 1e3 + 60 * values.hours * 60 * 1e3;
  if (t <= 864e5) {
    e = null;
    (e = new XMLHttpRequest).open("GET", "setTimer?pin=" + values.pin + "&time=" + t, !0);
    e.send();

    if (!values.interval) values.interval = setInterval(() => {countDown();}, 1e3);
  }
  values.settingTimer = 0;
}

function startTimerOnce() {
  getPin();
  values.intervalAlive = null;
  values.active = 0;
  var e, t = 1e3 * values.seconds + 60 * values.minutes * 1e3 + 60 * values.hours * 60 * 1e3;
  if (t <= 864e5) {
    e = null;
    (e = new XMLHttpRequest).open("GET", "setTimerOnce?pin=" + values.pin + "&time=" + t, !0);
    e.send();

    if (!values.interval) values.interval = setInterval(() => {countDown();}, 1e3);
  }
  values.settingTimer = 0;
}

function getBoxMessage() {
  var n = null;
  (n = new XMLHttpRequest).open("GET", "getMessage?pin=" + values.pin, !1), n.onreadystatechange = function() {
    let e;
    if (4 == n.readyState && 200 == n.status) {
      e = n.responseText;
      document.getElementById("Message").value = e;
    }
  };
  n.send();
}

function setMessage() {
  getPin();
  var e, t = document.getElementById("Message").value;
  e = null;
  (e = new XMLHttpRequest).open("GET", "setMessage?pin=" + values.pin + "&message=" + t, !0);
  e.send();
}

function clearMessage() {
  getPin();
  var e;
  e = null;
  document.getElementById("Message").value = "";
  (e = new XMLHttpRequest).open("GET", "clearMessage?pin=" + values.pin, !0);
  e.send();
}

function countDown() {
  if (0 == values.seconds && 0 == values.minutes && 0 == values.hours) {
    clearInterval(values.interval);
    values.interval = null;
    values.init = values.active = 1;
    values.lock = !1;
    return;
  }
  if (0 < values.seconds) {
    --values.seconds;
  } else {
    values.seconds = 59;
    if (0 < values.minutes) {
      --values.minutes;
    } else {
      values.minutes = 59;
      if (0 < values.hours)
        --values.hours;
    }
  }
  refreshTimer();
}

function getState() {
  //console.log();
  let ajaxRequest = new XMLHttpRequest;
  let status = 0;
  ajaxRequest.open("GET", "getState", !0);
  ajaxRequest.timeout = 3500;
  ajaxRequest.ontimeout = function(e) {
    document.getElementById("status").innerHTML = "Not connected";
    status = 0;
    document.getElementById("lockcontrols").style.display = "none";
  };
  ajaxRequest.onreadystatechange = function() {
    if (4 == ajaxRequest.readyState && (200 == ajaxRequest.status)) {
      status = 1;
      document.getElementById("lockcontrols").style.display = "block";
      document.getElementById("lockmessage").style.display = "block";
      values.lidstate = Number(ajaxRequest.responseText.split(";")[0]);
      values.lockstate = Number(ajaxRequest.responseText.split(";")[1]);
      values.lockstatetext = ajaxRequest.responseText.split(";")[2];
      values.keyinbox = Number(ajaxRequest.responseText.split(";")[4]);
      switch (values.lockstate) {
      case 0:
        document.getElementById("Helptext").innerHTML = "Box is free to open anytime.";
        break;
      case 1:
        document.getElementById("Helptext").innerHTML = "Box is free to open only once and will be locked when closed again.";
        break;
      case 2:
        if (values.lidstate == 0) {
          document.getElementById("Helptext").innerHTML = "Box is locked indefinitely.";
        } else {
          document.getElementById("Helptext").innerHTML = "Box will be locked indefinitely when lid is closed.";
        }
        break;
      case 3:
        if (values.lidstate == 0) {
          document.getElementById("Helptext").innerHTML = "Box is locked until end of timer. It will then be free to open until manually locked.";
        } else {
          document.getElementById("Helptext").innerHTML = "Box will be locked until end of timer when lid is closed.";
        }
        break;
      case 4:
        if (values.lidstate == 0) {
          document.getElementById("Helptext").innerHTML = "Box is locked until end of timer. It will then be free to open only once and then locked back.";
        } else {
          document.getElementById("Helptext").innerHTML = "Box will be locked until end of timer when lid is closed.";
        }
        break;

      }
      if (ajaxRequest.responseText.split(";")[3] == '' && values.settingTimer == 0) {
        values.hours = values.minutes = values.seconds  = 0;
        refreshTimer();
      }
      console.log(values.lidstate, values.lockstate, values.lockstatetext);
      if (values.lidstate == 0) {
        document.getElementById("lidstate").innerHTML = "Closed";
        document.getElementById("lidstate").className = "status";
        if (values.lockstate < 2) {
          document.getElementById("HelptextLid").innerHTML = "Box Lid is closed and can be opened by slave.";
        } else {
          document.getElementById("HelptextLid").innerHTML = "Box Lid is closed and cannot be opened by slave.";
        }
      } else {
        document.getElementById("lidstate").innerHTML = "Open";
        if (values.lockstate < 2) {
          document.getElementById("lidstate").className = "status";
          document.getElementById("HelptextLid").innerHTML = "Box Lid is open.";
        } else {
          document.getElementById("lidstate").className = "status warn";
          document.getElementById("HelptextLid").innerHTML = "Box Lid is open and slave is asked to close it.";
        }
      }
      if (values.keyinbox == 0) {
        document.getElementById("keyinbox").innerHTML = "Not In The Box";
        if (values.lockstate < 2) {
          document.getElementById("keyinbox").className = "status";
        } else {
          document.getElementById("keyinbox").className = "status warn";
        }
        document.getElementById("HelptextKIB").innerHTML = "Slave\'s cock may not be locked already. Please give your orders.";
      } else {
        document.getElementById("keyinbox").innerHTML = "In The Box";
        document.getElementById("keyinbox").className = "status ok";
        if (values.lockstate < 2) {
          document.getElementById("HelptextKIB").innerHTML = "Slave\'s cock is locked and key is waiting in the box for Mistress to lock it";
        } else {
          document.getElementById("HelptextKIB").innerHTML = "Slave\'s cock is locked and key is waiting in the box until Mistress sees fit.";
        }
      }
      if (values.lockstate == 0) {
        document.getElementById("status").innerHTML = values.lockstatetext;
        document.getElementById("status").className = "status ok";
        document.getElementById("buttonFree").className = "buttonInactive";
        document.getElementById("buttonOnce").className = "buttonInactive";
        document.getElementById("buttonLock").className = "buttonStart";
        document.getElementById("buttonFree").disabled = !0;
        document.getElementById("buttonOnce").disabled = !0;
        document.getElementById("buttonLock").disabled = !1;
        document.getElementById("timerdiv").style.display = "block";
        document.getElementById("pmButtons").style.display = "none";
        document.getElementById("InitButtons").style.display = "block";
        document.getElementById("rowStartTimer").style.display = "block";
        document.getElementById("rowChangeTimer").style.display = "none";

        if (values.interval) clearInterval(values.interval);
      } else if (values.lockstate == 1) {
        document.getElementById("status").innerHTML = values.lockstatetext;
        document.getElementById("status").className = "status warn";
        document.getElementById("buttonFree").className = "buttonStop";
        document.getElementById("buttonOnce").className = "buttonInactive";
        document.getElementById("buttonLock").className = "buttonStart";
        document.getElementById("buttonFree").disabled = !1;
        document.getElementById("buttonOnce").disabled = !0;
        document.getElementById("buttonLock").disabled = !1;
        document.getElementById("timerdiv").style.display = "block";
        document.getElementById("pmButtons").style.display = "none";
        document.getElementById("InitButtons").style.display = "block";
        document.getElementById("rowStartTimer").style.display = "block";
        document.getElementById("rowChangeTimer").style.display = "none";

        if (values.interval) clearInterval(values.interval);

      } else {
        document.getElementById("status").className = "status locked";
        document.getElementById("status").innerHTML = values.lockstatetext;
        document.getElementById("buttonFree").className = "buttonStop";
        document.getElementById("buttonOnce").className = "buttonReset";
        document.getElementById("buttonFree").disabled = !1;
        document.getElementById("buttonOnce").disabled = !1;
        document.getElementById("timerdiv").style.display = "block";
        if (values.lockstate > 2) {
          document.getElementById("buttonLock").className = "buttonStart";
          document.getElementById("buttonLock").disabled = !1;
          document.getElementById("pmButtons").style.display = "block";
          document.getElementById("InitButtons").style.display = "none";

          let t = Number(ajaxRequest.responseText.split(";")[3]);
          values.hours = Math.floor(t / 36e5);
          values.minutes = Math.floor(t%36e5/6e4);
          values.seconds = Math.floor(t%6e4/1e3);
          if (!values.interval) values.interval = setInterval(() => {countDown();}, 1e3);
          console.log('refreshTimer', values.hours, values.minutes, values.seconds);
          document.getElementById("rowStartTimer").style.display = "none";
          document.getElementById("rowChangeTimer").style.display = "block";
        } else {
          document.getElementById("buttonLock").className = "buttonInactive";
          document.getElementById("buttonLock").disabled = !0;
          document.getElementById("pmButtons").style.display = "none";
          document.getElementById("InitButtons").style.display = "block";
          if (values.settingTimer == 0) {
            document.getElementById("rowStartTimer").style.display = "none";
          }
          document.getElementById("rowChangeTimer").style.display = "none";

          if (values.interval) clearInterval(values.interval);
        }
      }


    } else {
      if (0 == status) {
        document.getElementById("status").innerHTML = "Lock Status: not connected";
        document.getElementById("status").className = "status";
      }
      document.getElementById("keyinbox").innerHTML = "";
      document.getElementById("lidstate").innerHTML = "";
      document.getElementById("HelptextKIB").innerHTML = "";
      document.getElementById("HelptextLid").innerHTML = "";
      document.getElementById("Helptext").innerHTML = "";

      document.getElementById("lockcontrols").style.display = "none";
      document.getElementById("lockmessage").style.display = "none";
      status = 0;
    }
  };
  try {
    ajaxRequest.send();
  } catch (e) {
    if (0 == status) {
      document.getElementById("status").innerHTML = "Not connected";
    }
    document.getElementById("keyinbox").innerHTML = "";
    document.getElementById("lidstate").innerHTML = "";
    document.getElementById("HelptextKIB").innerHTML = "";
    document.getElementById("HelptextLid").innerHTML = "";
    document.getElementById("Helptext").innerHTML = "";

    document.getElementById("lockcontrols").style.display = "none";
    document.getElementById("lockmessage").style.display = "none";

    status = 0;
  }
}

function runLoad() {
  setPin();
  getState();
  getBoxMessage();
  refreshTimer();
  if (!values.intervalAlive) values.intervalAlive = setInterval(() => {getState();}, 4e3);
}

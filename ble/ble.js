// * ****************************************************************************
// *  Copyright (c) 2012-2017 Skreens Entertainment Technologies Incorporated - http://skreens.com
// *
// *  Redistribution and use in source and binary forms, with or without  modification, are
// *  permitted provided that the following conditions are met:
// *
// *  Redistributions of source code must retain the above copyright notice, this list of
// *  conditions and the following disclaimer.
// *
// *  Redistributions in binary form must reproduce the above copyright  notice, this list of
// *  conditions and the following disclaimer in the documentation and/or other materials
// *  provided with the distribution.
// *
// *  Neither the name of Skreens Entertainment Technologies Incorporated  nor the names of its
// *  contributors may be used to endorse or promote products derived from this software without
// *  specific prior written permission.
// *
// *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// * ******************************************************************************
'use strict';

var websocket;
var socketBase;
var refCount = 0;
var glbRefresh = 2000; // 10 seconds
var urlHome = "http://";
var wsHome = "ws://";
var DISPLAY_ON = 0;
var DISPLAY_OFF = 1;
var SCAN_REQUEST = 1;
var NAME_ONLY = 2;
var NAME_ALL = 3;
// Unit options.
var optAddr1 = "0.0.0.0"; 
var optPort1 = "80";
var optName1 = "true"; 
var nextInput = 1;
var completeCount;
var completeKey = [];
var	completeValue = [];
var bleDevicesCount=0;
var bleDevicesAddr = [];
var	bleDevicesName = [];
var	bleDevicesRssi = [];
var	bleDevicesTime = [];
var messLines = [];
var messElements = 0;

var debugToggle = true;
var progress = 0;

function bleDevices()
{
	let sttHTML = "<div style='position:absolute; top:275px; left:10px; width:900px'>";
	sttHTML += "<fieldset><legend><b>Bluetooth Devices</b></legend>";

	sttHTML += "<table border=1 cellspacing=0>";
	sttHTML += "<tr>";
	sttHTML += "<td style='text-align:center; width:250px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'>Address</td>";
	sttHTML += "<td style='text-align:center; width:250px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'>Name</td>";
	sttHTML += "<td style='text-align:center; width:50px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'>RSSI</td>";
	sttHTML += "<td style='text-align:center; width:150px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'>Signal Strength</td>";
	sttHTML += "<td style='text-align:center; width:150px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'>Last Detect</td>";
	sttHTML += "</tr>";
        
	if(bleDevicesCount != 0)
	{
		for(let i=0; i < bleDevicesCount; i++) 
		{		
			sttHTML += "<tr>";

			sttHTML += "<td><input  type='text'  id='dFld1";
			sttHTML += i;
			sttHTML += "' style='width:250px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'></td>";

			sttHTML += "<td><input  type='text'  id='dFld2";
			sttHTML += i;
			sttHTML += "' style='width:250px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'></td>";

			sttHTML += "<td><input  type='text'  id='dFld3";
			sttHTML += i;
			sttHTML += "' style='text-align:center; width:50px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'></td>";

			sttHTML += "<td><input  type='text'  id='dFld4";
			sttHTML += i;
			sttHTML += "' style='text-align:center; width:150px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'></td>";

			sttHTML += "<td><input  type='text'  id='dFld5";
			sttHTML += i;
			sttHTML += "' style='text-align:center; width:150px; font-size:16px; height:20px; color:#000000; background-color:#85C1E9'></td>";

			sttHTML += "</tr>";
		}
	}
			
	sttHTML += "</table></fieldset></div>";
	document.getElementById('bInfo').innerHTML = sttHTML;

	if(bleDevicesCount != 0)
	{
		let eOne = 'dFld1';
		let eTwo = 'dFld2';
		let eThree = 'dFld3';
		let eFour = 'dFld4';
		let eFive = 'dFld5';

		for(let i=0; i < bleDevicesCount; i++) 
		{		
			document.getElementById(eOne+i).value = bleDevicesAddr[i];
			document.getElementById(eTwo+i).value = bleDevicesName[i];
			document.getElementById(eThree+i).value = bleDevicesRssi[i];
			let distance = "unknown";
			if(bleDevicesRssi[i] >= 90)										distance = "Excellent";
			else if((bleDevicesRssi[i] >= 70) && (bleDevicesRssi[i] <= 89))	distance = "Very Good";
			else if((bleDevicesRssi[i] >= 50) && (bleDevicesRssi[i] <= 69))	distance = "Good";
			else if((bleDevicesRssi[i] >= 30) && (bleDevicesRssi[i] <= 49))	distance = "Fair";
			else if((bleDevicesRssi[i] >= 10) && (bleDevicesRssi[i] <= 29))	distance = "Poor";
			else															distance = "Out Of Range";
			document.getElementById(eFour+i).value = distance;
			document.getElementById(eFive+i).value = bleDevicesTime[i];
		}
	}
}

function optionSetup()
{
	let ectl;
	let dctl;

	let optHTML = "<div id='optionsId' style='position:absolute; top:15px; left:10px; width:900px;'>";
	optHTML += "<fieldset><legend><b> Bluetooth Low Energy Configuration</b></legend>";
	optHTML += "<table border='1' width=100% cellspacing='1' style='text-align:center; overflow:hidden; font-family:Times New Roman; font-size:16px; color:#000000; background-color:#85C1E9;'>";

	optHTML += "<tr>"; 
   	optHTML += "<td style='text-align:left; width:300px; font-size:16px;'>IP Address</td>";
   	optHTML += "<td align='left'><input type='text' id='cfgAddr1' style='text-align:left; width:125px; font-size:16px;'></td>";
	optHTML += "</tr>";

	optHTML += "<tr>"; 
   	optHTML += "<td style='text-align:left; width:300px; font-size:16px;'>Network Port</td>";
   	optHTML += "<td align='left'><input type='text' id='cfgPort1' style='text-align:left; width:125px; font-size:16px;'></td>";
	optHTML += "</tr>";

	optHTML += "<tr>"; 
   	optHTML += "<td style='text-align:left; width:300px; font-size:16px;'>Named Devices Only</td>";
   	optHTML += "<td align='left'><input type='checkbox' id='cfgName1' style='text-align:left; font-size:16px;'></td>";
	optHTML += "</tr>";

   	optHTML += "<tr>";
   	optHTML += "<td align='center'>";
   	optHTML += "<button onclick='sendOptions(1)' style='text-align:center; font-size:16px; height:24px; overflow:hidden; color:#000000; background-color:#FFFF66;' id='ALMAction' name'ALM'>Submit</button>";
   	optHTML += "</td>";

   	optHTML += "<td align='center'>";
   	optHTML += "<button onclick='sendOptions(0)' style='text-align:center; font-size:16px; height:24px; overflow:hidden; color:#000000; background-color:#FFFF66;' id='ALMAction' name'ALM'>Cancel</button>";
   	optHTML += "</td>";
   	optHTML += "</tr>";

   	optHTML += "</table></div>";
	document.getElementById('aOptions').innerHTML = optHTML;

	let myAddr1 = document.getElementById('cfgAddr1');
	myAddr1.value = optAddr1;

	let myPort1 = document.getElementById('cfgPort1');
	myPort1.value = optPort1;

	let myName1 = document.getElementById('cfgName1');
	if(myName1.value == "true") {
		document.getElementById('cfgName1').checked = true;
	} else {
		document.getElementById('cfgName1').checked = false;
	}
}

function sendOptions(ctrl)
{
	let dbgSection;

	if(ctrl == 1)
	{
		optAddr1 = document.getElementById('cfgAddr1').value;
		localStorage.setItem('storeIp1',optAddr1);
		optPort1 = document.getElementById('cfgPort1').value;
		localStorage.setItem('storePort1',optPort1);

		let nameCheck = document.getElementById('cfgName1').checked;
		if(nameCheck == true) optName1 = "true";
		if(nameCheck == false) optName1 = "false";
		localStorage.setItem('storeName1',optName1);

		if(optName1 == "true") getFromWebsock(NAME_ONLY);
		else if(optName1 == "false") getFromWebsock(NAME_ALL);

		// After a submit, reset the ble counter
		bleDevicesCount=0;
	}
}

function config_unit(display)
{
	let localIp = localStorage.getItem('storeIp1');
	if(localIp == null) localIp = "192.168.0.0";
	let allText = ("IP1 " + localIp);
	configOptions(allText, display); 

	let localPort = localStorage.getItem('storePort1');
	if(localPort == null) localPort = "80";
	allText = ("PORT1 " + localPort);
	configOptions(allText, display); 

	let localName = localStorage.getItem('storeName1');
	if(localName == null) localName = true;
	allText = ("NAME1 " + localName);
	configOptions(allText, display); 
}

function configOptions(text, display) 
{
	let index;
	let i;
	let cnt;
	let count = text.split(/\r\n|\r|\n/); 

	// Check for no data.
	if(count.length > 0)
	{
		// Split up the config file parameters and init our option array.
		for(i = 0;i < count.length; i++)
		{
			cnt = count[i].split(" "); 
			let option = cnt[0].split(" ", cnt.length);

			switch(cnt[0])
			{
				case 'IP1' :
				{
					optAddr1 = cnt[1];
					break;
				}
				case 'PORT1' :
				{
					optPort1 = cnt[1];
					break;
				}
				case 'NAME1' :
				{
					optName1 = cnt[1];
					break;
				}
			}
		
		}
	}
}
function changeicon(x){
    document.head.innerHTML = "<link href='" + x + "' rel='shortcut icon'><title>" + document.title;
}
function showMyDevices()
{

	// If first starting, refresh fast "shows blank data :-( ", then slow it down.
	if(refCount++ == 0)
	{
		changeicon("favicon.ico");
		config_unit(DISPLAY_ON);
		optionSetup();
	}
	else
	{
		config_unit(DISPLAY_ON);
		bleDevices();
	}

	window.setTimeout(showMyDevices,glbRefresh);
}
function getMyDevices()
{
	getFromWebsock(SCAN_REQUEST);

	window.setTimeout(getMyDevices,1000);
}
function isEmpty(arg) {
  for (let item in arg) {
    return false; // Not Empty or null
  }
  return true; // Empty.
}

function getFromWebsock(key)
{
	let myWs = wsHome.concat(optAddr1);
	let wsUri = myWs.concat(":");
	wsUri = wsUri.concat(optPort1);
	if(key == SCAN_REQUEST)			wsUri = wsUri.concat("/command/ble_read");
	else if(key == NAME_ONLY)		wsUri = wsUri.concat("/command/ble_name_only");
	else if(key == NAME_ALL)		wsUri = wsUri.concat("/command/ble_name_all");

    websocket = new WebSocket(wsUri);
    websocket.onopen = function(evtSocket) { onOpen(evtSocket) };
    websocket.onclose = function(evtSocket) { onClose(evtSocket) };
    websocket.onmessage = function(evtSocket) { onMessage(evtSocket) };
    websocket.onerror = function(evtSocket) { onError(evtSocket, wsUri) };

}

function onMessage(evtSocket)
{
	socketBase = JSON.parse(evtSocket.data);
    websocket.close();

	let levelOne = isEmpty(socketBase.address);
	if(levelOne == false) {

		// If we are getting valid websocket data, advance the progress bar.
		if(progress > 100) progress = 0;
		scanPercent(progress);
		progress += 5;

		let foundDevice = false;
		if(bleDevicesCount == 0) {
			bleDevicesAddr[bleDevicesCount] = socketBase.address;
			bleDevicesName[bleDevicesCount] = socketBase.name;
			bleDevicesRssi[bleDevicesCount] = socketBase.rssi;
			bleDevicesTime[bleDevicesCount] = displayTime();
			bleDevicesCount++; 
		}
		else {
			for(let i=0; i < bleDevicesCount; i++) {
				// If we already have this address, just update the RSSI, name and time.
				if(bleDevicesAddr[i] == socketBase.address) {

					// If this name is unknown, and we already have a real name, don't update the name. 
					if((socketBase.name == "(unknown name)") && (bleDevicesName[i] == "(unknown name)")) {
						console.log("F name=" + socketBase.name + " Sname=" + bleDevicesName[i] + " idx=" + i);
						bleDevicesName[i] = socketBase.name;
					}
					// If this name is real name, always write it.
					else if(socketBase.name != "(unknown name)") {
						bleDevicesName[i] = socketBase.name;
						console.log("N name=" + socketBase.name + " Sname=" + bleDevicesName[i] + " idx=" + i);
					}

					bleDevicesRssi[i] = socketBase.rssi;
					bleDevicesTime[i] = displayTime();
					foundDevice = true;
				}
			}
			if(foundDevice == false) {
				bleDevicesAddr[bleDevicesCount] = socketBase.address;
				bleDevicesName[bleDevicesCount] = socketBase.name;
				console.log("A name=" + socketBase.name + " Sname=" + bleDevicesName[bleDevicesCount]);
				bleDevicesRssi[bleDevicesCount] = socketBase.rssi;
				bleDevicesTime[bleDevicesCount] = displayTime();
				bleDevicesCount++; 
			}
		}
	}
	levelOne = isEmpty(socketBase.namersp);
	if(levelOne == false) {
		console.log("Adrress mode command returned = " + socketBase.namersp);
	}
}
function onOpen(evtSocket)
{
//    doSend('{"ID":23}');
}
function onClose(evtSocket)
{
	let reason;

	// See http://tools.ietf.org/html/rfc6455#section-7.4.1
	if (evtSocket.code == 1000)
	    reason = "Normal closure, meaning that the purpose for which the connection was established has been fulfilled.";
	else if(evtSocket.code == 1001)
	    reason = "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page.";
	else if(evtSocket.code == 1002)
	    reason = "An endpoint is terminating the connection due to a protocol error";
	else if(evtSocket.code == 1003)
	    reason = "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).";
	else if(evtSocket.code == 1004)
	    reason = "Reserved. The specific meaning might be defined in the future.";
	else if(evtSocket.code == 1005)
	    reason = "No status code was actually present.";
	else if(evtSocket.code == 1006)
	   reason = "The connection was closed abnormally, e.g., without sending or receiving a Close control frame";
	else if(evtSocket.code == 1007)
	    reason = "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message).";
	else if(evtSocket.code == 1008)
	    reason = "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.";
	else if(evtSocket.code == 1009)
	   reason = "An endpoint is terminating the connection because it has received a message that is too big for it to process.";
	else if(evtSocket.code == 1010) // Note that this status code is not used by the server, because it can fail the WebSocket handshake instead.
	    reason = "An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: " + event.reason;
	else if(evtSocket.code == 1011)
	    reason = "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.";
	else if(evtSocket.code == 1015)
	    reason = "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).";
	else
	    reason = "Unknown reason";

	if(evtSocket.code != 1005)
		console.log("Websocket onClose() Message : " + reason + " # " + evtSocket.code);
}
function onError(evtSocket, url)
{
    //alert('Websocket onError() error state. URL = ' + url);
}
function doSend(message)
{
    websocket.send(message);
}

/**************************************************************************************
 * 
 ***************************************************************************************/
function scanPercent(qPercent)
{
	let pixels = 0;
/***************************************************************************************************
* Page header data.
***************************************************************************************************/
	let perHTML = "<div style='position:absolute; border-radius: 10px; overflow-y:none; width:950px; height:50px; top:200px; left:10px;'>";
	perHTML += "<style type=\"text/css\">\n";
	perHTML += "div.qProgressPage{position: relative;border: 1px solid black;}";
	perHTML += "div.qProgressSlot{position: absolute;top: 0; left: 0;height: 100%;}";
	perHTML += "div.qProgressText{text-align: center;position: relative;}";
	perHTML += "</style>\n";
/************************************************************************************************
* Paste in the progress bar.
************************************************************************************************/
	pixels = 900 * qPercent;
	pixels = pixels / 100;

	perHTML += "<tr bgcolor=\"#20FF80\">";
	perHTML += "<td width=\"80%%\"align=\"center\">";
	perHTML += "<b style=\"font-family:Courier New; font: 15pt; background:#ffffff none; color:#000000;\">";
	perHTML += "<div id=\"qProgressPage\" style=\"border-radius: 10px; position: relative;border: 3px solid black; width:900px\">";	

	perHTML += "<div id=\"qProgressSlot\" style=\"position: absolute;top: 0; left: 0;height: 18px; width:";
	perHTML += pixels;
	perHTML += "px; background-color:#66ff00\"></div>";  

	perHTML += "<div id=\"qProgressText\" style=\"text-align: center;position: relative; width:900px\">Progress ";  
	perHTML += qPercent;
	perHTML += "%</div>";  

	perHTML += "</div>";  
	perHTML += "</b>"; 
	perHTML += "</td>";
	perHTML += "</tr>";
	perHTML += "</table></div>";
	document.getElementById('pInfo').innerHTML = perHTML;
}

function displayTime() {
    var str = "";

    var currentTime = new Date()
    var hours = currentTime.getHours()
    var minutes = currentTime.getMinutes()
    var seconds = currentTime.getSeconds()

    if (minutes < 10) {
        minutes = "0" + minutes
    }
    if (seconds < 10) {
        seconds = "0" + seconds
    }
    str += hours + ":" + minutes + ":" + seconds + " ";
    if(hours > 11){
        str += "PM"
    } else {
        str += "AM"
    }
    return str;
}

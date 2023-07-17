/*
  helper functions for RemoteID javascript
*/

// helper function for cross-origin requests
function createCORSRequest(method, url) {
    var xhr = new XMLHttpRequest();
    if ("withCredentials" in xhr) {
        // XHR for Chrome/Firefox/Opera/Safari.
        xhr.open(method, url, true);
    } else if (typeof XDomainRequest != "undefined") {
        // XDomainRequest for IE.
        xhr = new XDomainRequest();
        xhr.open(method, url);
    } else {
        // CORS not supported.
        xhr = null;
    }
    return xhr;
}


/*
  fill variables in a page from json
*/
function page_fill_json_value(json) {
    for (var v in json) {
        var element = document.getElementById(v);
        if (element) {
            element.value = json[v];
        }
    }
}

/*
  fill html in a page from json
*/
function page_fill_json_html(json) {
    for (var v in json) {
        var element = document.getElementById(v);
        if (element) {
            element.innerHTML = json[v];
        } else if (v == "STATUS:BOARD_ID") {
	    if(typeof page_fill_json_html.run_once == 'undefined' ) {
		//run this code only once to avoid updating these fields
                if (json[v] == "3") {
                    document.getElementById("logo").src="images/bluemark.png";
                    document.getElementById("logo").alt="BlueMark";
                    document.getElementById("STATUS:BOARD").innerText = "BlueMark db200";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://download.bluemark.io/db200.pdf'>db200 series manual</a></li><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                    document.body.style.background = "#fafafa";
                } else if (json[v] == "4") {
                    document.getElementById("logo").src="images/bluemark.png";
                    document.getElementById("logo").alt="BlueMark";
                    document.getElementById("STATUS:BOARD").innerText = "BlueMark db110";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://download.bluemark.io/db110.pdf'>db110 manual</a></li><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                    document.body.style.background = "#fafafa";
                } else if (json[v] == "10") {
                    document.getElementById("logo").src="images/bluemark.png";
                    document.getElementById("logo").alt="BlueMark";
                    document.getElementById("STATUS:BOARD").innerText = "BlueMark db210pro";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://download.bluemark.io/db200.pdf'>db210pro manual</a></li><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                } else if (json[v] == "8") {
                    document.getElementById("logo").src="images/bluemark.png";
                    document.getElementById("logo").alt="BlueMark";
                    document.getElementById("STATUS:BOARD").innerText = "BlueMark db202mav";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://download.bluemark.io/db200.pdf'>db200 series manual</a></li><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                    document.body.style.background = "#fafafa";
                } else if (json[v] == "9") {
                    document.getElementById("logo").src="images/bluemark.png";
                    document.getElementById("logo").alt="BlueMark";
                    document.getElementById("STATUS:BOARD").innerText = "BlueMark db203can";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://download.bluemark.io/db200.pdf'>db200 series manual</a></li><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                    document.body.style.background = "#fafafa";
                } else if (json[v] == "11") {
                    document.getElementById("logo").src="images/holybro.jpg";
                    document.getElementById("logo").alt="Holybro";
                    document.getElementById("STATUS:BOARD").innerText = "Holybro RemoteID";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                } else if (json[v] == "12") {
                    document.getElementById("logo").src="images/CUAV.jpg";
                    document.getElementById("logo").alt="CUAV";
                    document.getElementById("STATUS:BOARD").innerText = "CUAV RemoteID";
                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                } else {
                    document.getElementById("logo").src="images/logo.jpg";
                    document.getElementById("logo").alt="ArduPilot";

                    if (json[v] == "1") {
                        document.getElementById("STATUS:BOARD").innerText = "ESP32S3_DEV";
                    } else if (json[v] == "2") {
                        document.getElementById("STATUS:BOARD").innerText = "ESP32C3_DEV";
                    } else if (json[v] == "5") {
                        document.getElementById("STATUS:BOARD").innerText = "JW_TBD";
                    } else if (json[v] == "6") {
                        document.getElementById("STATUS:BOARD").innerText = "mRo-RID";
                    } else {
                        document.getElementById("STATUS:BOARD").innerText = "unknown:" + json[v];
                    }

                    document.getElementById("documentation").innerHTML = "<ul><li><a href='https://ardupilot.org/ardupilot/index.html'>ArduPilot Project</a></li><li><a href='https://github.com/ArduPilot/ArduRemoteID'>ArduRemoteID Project</a></li><li><a href='https://ardupilot.org/plane/docs/common-remoteid.html'>ArduPilot RemoteID Documentation</a></li><li><a href='https://www.opendroneid.org/'>OpenDroneID Website</a></li></ul>";
                }
            }
            run_once = 1;
        }
    }
}

/*
  fetch a URL, calling a callback
*/
function ajax_get_callback(url, callback) {
    var xhr = createCORSRequest("GET", url);
    xhr.onload = function() {
        callback(xhr.responseText);
    }
    xhr.send();
}

/*
  fetch a URL, calling a callback for binary data
*/
function ajax_get_callback_binary(url, callback) {
    var xhr = createCORSRequest("GET", url);
    xhr.onload = function() {
        console.log("got response length " + xhr.response.byteLength);
        callback(xhr.response);
    }
    xhr.responseType = "arraybuffer";
    xhr.send();
}

/*
  poll a URL, calling a callback
*/
function ajax_poll(url, callback, refresh_ms=1000) {
    function again() {
        setTimeout(function() { ajax_poll(url, callback, refresh_ms); }, refresh_ms);
    }
    var xhr = createCORSRequest("GET", url);
    xhr.onload = function() {
        if (callback(xhr.responseText)) {
            again();
        }
    }
    xhr.onerror = function() {
        again();
    }
    xhr.timeout = 3000;
    xhr.ontimeout = function() {
        again();
    }
    xhr.send();
}


/*
  poll a json file and fill document IDs at the given rate
*/
function ajax_json_poll(url, callback, refresh_ms=1000) {
    function do_callback(responseText) {
        try {
            var json = JSON.parse(responseText);
            return callback(json);
        } catch(e) {
            return true;
        }
        /* on bad json keep going */
        return true;
    }
    ajax_poll(url, do_callback, refresh_ms);
}

/*
  poll a json file and fill document IDs at the given rate
*/
function ajax_json_poll_fill(url, refresh_ms=1000) {
    function callback(json) {
        page_fill_json_html(json);
        return true;
    }
    ajax_json_poll(url, callback, refresh_ms);
}


/*
  set a message in a div by id, with given color
*/
function set_message_color(id, color, message) {
    var element = document.getElementById(id);
    if (element) {
        element.innerHTML = '<b style="color:' + color + '">' + message + '</b>';
    }
}

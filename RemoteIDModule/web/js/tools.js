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

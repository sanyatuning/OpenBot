

const limit = 100;
let startX;
let startY;
let socket;

function debugEvent(e) {
    console.log(e.type, e);
}

const canvas = document.getElementById('canvas');
const ctx = canvas.getContext("2d");

const softSend = throttle(send, 100);

canvas.addEventListener('touchstart', start);
canvas.addEventListener('mousedown', start);

canvas.addEventListener('touchmove', move);
canvas.addEventListener('mousemove', move);

canvas.addEventListener('touchend', stop);
canvas.addEventListener('mouseup', stop);

startWS();

function start(e) {
    startX = clientX(e);
    startY = clientY(e);
    console.log(e.type, startX, startY)
    ctx.beginPath();
    ctx.arc(startX / canvas.clientWidth * canvas.width, startY / canvas.clientHeight * canvas.height, 200, 0, 2 * Math.PI);
    ctx.stroke();
    e.preventDefault();
}

function move(e) {
    const x = minmax(startX - clientX(e), limit / 4);
    const y = minmax(startY - clientY(e), limit);
    softSend(
        x < 0 ? y : (y < 0 ? y + x : y - x),
        x > 0 ? y : (y > 0 ? y + x : y - x),
    );
}

function stop(e) {
    send(0, 0);
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    e.preventDefault();
}

function send(L, R) {
    console.log({L, R});
    socket.send(`c${L},${R}`)
}

function clientX(e) {
    return e.touches ? e.touches[0].clientX : e.clientX;
}
function clientY(e) {
    return e.touches ? e.touches[0].clientY : e.clientY;
}

function minmax(v, limit) {
    return Math.round(Math.max(-limit, Math.min(limit, v)));
}

function throttle(func, limit) {
    let inThrottle
    return (...args) => {
        if (!inThrottle) {
            func(...args)
            inThrottle = setTimeout(() => inThrottle = false, limit)
        }
    }
}

function startWS() {

    socket = new WebSocket('ws://192.168.0.160/ws')
    socket.onopen = function () {
        console.log("[open] Connection established");
        console.log("Sending to server");
        socket.send("My name is John");
    };

    socket.onmessage = function (event) {
        console.log(`[message] Data received from server: ${event.data}`);
    };

    socket.onclose = function (event) {
        if (event.wasClean) {
            console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
        } else {
            // e.g. server process killed or network down
            // event.code is usually 1006 in this case
            console.log('[close] Connection died');
        }
        setTimeout(startWS, 2000);
    };

    socket.onerror = function (error) {
        console.log(`[error] ${error.message}`);
    };
}

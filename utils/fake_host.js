const dgram = require('dgram');
const readline = require('readline');
const {promisify} = require('util');

const GN_PACKETID_CONNECT = 33865;
const GN_PACKETID_ACK = 8478;
const GN_PACKETID_GAMESTATE = 48347;

const GN_VERSION = 1;
const GAME_PORT = 13437;

const game_state_struct = [
	["seq_num", "u32"],
	["score", "u32"],
	["health", "f32"],
	["angle_accuracy", "f32"],
	["time", "u32"],
	["attack_accum", "f32"],
	["stick_x", "s8"],
	["stick_y", "s8"],
	["in_game", "u8"],
	["mm_screen", "u8"],
	["mm_song", "u8"],
	["mm_character", "u8"]
];
const number_sizes = {
	"u32": 4,
	"s32": 4,
	"u16": 2,
	"s16": 2,
	"u8": 1,
	"s8": 1,
	"f32": 4
};
const number_funcs = {
	"u32": "Uint32",
	"s32": "Int32",
	"u16": "Uint16",
	"s16": "Int16",
	"u8": "Uint8",
	"s8": "Int8",
	"f32": "Float32"
};

let game_state_size = 0;
for(let [name, type] of game_state_struct) {
	game_state_size = (Math.ceil(game_state_size / number_sizes[type])+1) * number_sizes[type];
}
console.log("Game state size: " + game_state_size);

let our_game_state = {
	seq_num: 1,
	in_game: 0,
	mm_screen: 2,
	mm_song: 1,
	mm_character: 1
};

let rl = readline.createInterface({
	input: process.stdin,
	output: process.stdout
});

const sock = dgram.createSocket('udp4');
let remote_address = null;
let remote_port = null;

sock.on('error', (err) => {
	console.error(err);
});

sock.on('message', (msg, rinfo) => {
	remote_address = rinfo.address;
	remote_port = rinfo.port;
	let dv = new DataView(msg.buffer);
	let type = dv.getUint16(0, true);
	if(hosting && initialized && (type == GN_PACKETID_CONNECT)) {
		let dv2 = new DataView(new ArrayBuffer(2));
		dv2.setUint16(0, GN_PACKETID_ACK, true);
		sock.send(new Uint8Array(dv2.buffer), remote_port, remote_address);
		console.log("Got connect packet from " + remote_address + ":" + remote_port);
		has_displayed = false;
		resend();
	} else if(initialized && type == GN_PACKETID_GAMESTATE) {
		disp_dv = dv;
		redisplay();
	}
});

sock.on('listening', () => {
	const address = sock.address();
	console.log(`server listening ${address.address}:${address.port}`);
});

let disp_dv;

function resend() {
	let dv = new DataView(new ArrayBuffer(2 + game_state_size));
	let n = 0;
	dv.setUint16(0, GN_PACKETID_GAMESTATE, true);
	for(let [name, type] of game_state_struct) {
		n = Math.ceil(n / number_sizes[type]) * number_sizes[type];
		dv["set"+number_funcs[type]](n+2, our_game_state[name], true);
	}
	our_game_state.seq_num++;
	sock.send(new Uint8Array(dv.buffer), remote_port, remote_address);
	return dv;
}

let initialized = false;
let hosting = false;
let has_displayed = false;

function display(dv) {
	let n = 0;
	for(let [name, type] of game_state_struct) {
		n = Math.ceil(n / number_sizes[type]) * number_sizes[type];
		let value = dv["get"+number_funcs[type]](n+2, true);
		n += number_sizes[type];
		console.log(name.padEnd(30, " ") + " = " + value);
	}
}
function redisplay() {
	if(!disp_dv) return;
	if(!has_displayed) {
		process.stdout.write("\n");
		display(disp_dv);
		has_displayed = true;
		return;
	}
	process.stdout.write("\033[s\r\033[" + game_state_struct.length + "A");
	display(disp_dv);
	process.stdout.write("\033[u");
}

rl.on('line', (input) => {
	has_displayed = false;
	if(!initialized) {
		if(input == "host") {
			hosting = true;
			initialized = true;
			sock.bind(GAME_PORT);
		}
	} else {
		if(input.startsWith("set ")) {
			let [cmd, name, value] = input.split(" ");
			our_game_state[name] = +value || 0;
			display(resend());
			console.log();
		}
	}
})

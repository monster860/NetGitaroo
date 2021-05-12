const dgram = require('dgram');
const readline = require('readline');
const {promisify} = require('util');

let rl = readline.createInterface({
	input: process.stdin,
	output: process.stdout
});
rl.question("IP: ", (s) => {
	rl.close();
	let ip = s;
	let port = 13437;
	let s1 = dgram.createSocket("udp4");
	let s2 = dgram.createSocket("udp4");
	let remote_ip ;
	let remote_port;
	s1.bind();
	s2.bind(13437);
	s1.on("message", (msg, rinfo) => {
		if(remote_ip) {
			s2.send(msg, remote_port, remote_ip);
		}
	});
	s2.on("message", (msg, rinfo) => {
		remote_ip = rinfo.address;
		remote_port = rinfo.port;
		s1.send(msg, port, ip);
	})
});
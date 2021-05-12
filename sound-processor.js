// Okay so
// the purpose of this is to reduce the packet size of some of the win sounds
// so that they don't use as much IOP ram, because it turns out that the networking
// libraries instantly malloc ~100 KB of IOP ram and that's enough for the IOP to run out of memory
// loading the buffers for the sounds on multiplayer stages which is a very big sad-face
async function reduce_packet_size(blob, lba) {
	// http://boring.shoutwiki.com/wiki/.IMC
	let abs_off = lba*2048;
	let replacements = [];
	let num_sections = new DataView(await blob.slice(abs_off,abs_off+4).arrayBuffer()).getUint32(0, true);
	let section_dv = new DataView(await blob.slice(abs_off+4, abs_off+4+num_sections*32).arrayBuffer());
	for(let i = 0; i < num_sections; i++) {
		let name = "";
		for(let j = 0; j < 16; j++) {
			let ch = section_dv.getUint8(i*32 + j);
			if(ch == 0) break;
			name += String.fromCharCode(ch);
		}
		if(name.startsWith("Win_P")) {
			//console.log(name);
			let s_abs_off = abs_off + section_dv.getUint32(i*32+16, true);
			let header_dv = new DataView(await blob.slice(s_abs_off, s_abs_off+16).arrayBuffer());
			if(header_dv.getUint32(0, true) != 1) throw new Error("Wrong number of channels");
			let interleave = header_dv.getUint32(8, true);
			let num_packets = header_dv.getUint32(12, true);
			//console.log(interleave + ", " + num_packets);
			if((interleave % 3) != 0) throw new Error("Packet interlieave of " + interleave + " not divisible by 3");
			interleave /= 3;
			num_packets *= 3;
			header_dv.setUint32(8, interleave, true);
			header_dv.setUint32(12, num_packets, true);
			replacements.push([s_abs_off, new Blob([header_dv.buffer])]);
		}
	}
	return replacements;
}
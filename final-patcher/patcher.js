let isomime = "application/x-iso9660-image";

let elf_offset = -0xFFF80;

const valid_elf_filenames = {
	"SLUS_202.94;1": {
		initial_heapstart: 0x311280,
		heapstart_uppers: [
			0x100068,
			0x100C20
		],
		heapstart_lowers: [
			0x100070,
			0x100C28
		],
		heapstart_fulls: [
			0x247DE4
		],
		bin_size_target: 0x100000,
		int_patches: [
			// Redirect the printf to the actual printf spot
			[0x142588, 0x0804BA88],
			[0x14258C, 0x00000000],
			// Call the preloader
			/*[0x100908, 0x08099183],
			[0x100920, 0x7BBF0050],
			[0x100924, 0x03E00008],
			[0x100928, 0x27BD00E0]*/
			[0x1093D0, 0x08099183],
			[0x1093DC, 0x7BBF0020],
			[0x1093E0, 0x03E00008],
			[0x1093E4, 0x27BD0430]
		],
		other_patches: [
			[0x264660, new Blob([`cdrom0:\\MODULES\\NETGITRO.BIN\0`])]
		],
		preloader_patch: [
			
			                                                                        0x26, 0x00, 0x04, 0x3C,
			0x60, 0x46, 0x84, 0x24, 0xEC, 0xC0, 0x04, 0x0C, 0x01, 0x00, 0xA5, 0x24, 0x21, 0x80, 0x40, 0x00,
			0x31, 0x00, 0x05, 0x3C, 0x80, 0x12, 0xA5, 0x24, 0x34, 0x12, 0x06, 0x3C, 0x78, 0x56, 0xC6, 0x24,
			0x14, 0xC2, 0x04, 0x0C, 0x21, 0x20, 0x00, 0x02, 0x5C, 0xC1, 0x04, 0x0C, 0x21, 0x20, 0x00, 0x02,
			0xA8, 0xB2, 0x04, 0x0C, 0x00, 0x00, 0x04, 0x24, 0xA8, 0xB2, 0x04, 0x0C, 0x02, 0x00, 0x04, 0x24,
			0xA0, 0x44, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xF6, 0x24, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00,
			/*0x63, 0x64, 0x72, 0x6F, 0x6D, 0x30, 0x3A, 0x5C, 0x4D, 0x4F, 0x44, 0x55, 0x4C, 0x45, 0x53, 0x5C,
			0x4E, 0x45, 0x54, 0x47, 0x49, 0x54, 0x52, 0x4F, 0x2E, 0x42, 0x49, 0x4E, 0x3B, 0x31, 0x00, 0x00*/
			/*                                                                        0x25, 0x00, 0x04, 0x3C,
			0x40, 0x19, 0x84, 0x24, 0xEC, 0xC0, 0x04, 0x0C, 0x01, 0x00, 0xA5, 0x24, 0x21, 0x80, 0x40, 0x00,
			0x00, 0x00, 0x05, 0x24, 0x00, 0x00, 0x06, 0x24, 0x9A, 0xC1, 0x04, 0x0C, 0x21, 0x20, 0x00, 0x02,
			0x31, 0x00, 0x05, 0x3C, 0x80, 0x12, 0xA5, 0x24, 0x00, 0x00, 0x06, 0x3C, 0x58, 0x03, 0xC6, 0x24,
			0x14, 0xC2, 0x04, 0x0C, 0x21, 0x20, 0x00, 0x02, 0x5C, 0xC1, 0x04, 0x0C, 0x21, 0x20, 0x00, 0x02,
			0xA8, 0xB2, 0x04, 0x0C, 0x00, 0x00, 0x04, 0x24, 0xA8, 0xB2, 0x04, 0x0C, 0x02, 0x00, 0x04, 0x24,
			0xA0, 0x44, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x44, 0x02, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, */
		],
		preloader_binsize_upper_offset: 0x1C,
		preloader_binsize_lower_offset: 0x20,
		preloader_location: 0x26460C,
		bin: bin_us,
		image_patches: [
			{
				xgm: "PROJECTS/STDATA/US/VS_SEL/VS_SEL.XGM;1",
				imx: "MARUCHI.IMX",
				overlay: maruchi_patch
			}
		]
	}
};
async function convert() {
	try {
		let [
			dev9_irx_blob,
			netman_irx_blob,
			smap_irx_blob
		] = await Promise.all([
			fetch(dev9_irx).then(r => r.blob()),
			fetch(netman_irx).then(r => r.blob()),
			fetch(smap_irx).then(r => r.blob()),
		])
		let replacements = [];
		let addendums = [];
		let infile = document.getElementById("fileinput").files[0];
		let curr_iso_size = infile.size;

		let dev9_irx_lba, netman_irx_lba, smap_irx_lba, bin_lba;
		[dev9_irx_lba, curr_iso_size] = add_addendum(addendums, dev9_irx_blob, curr_iso_size);
		[netman_irx_lba, curr_iso_size] = add_addendum(addendums, netman_irx_blob, curr_iso_size);
		[smap_irx_lba, curr_iso_size] = add_addendum(addendums, smap_irx_blob, curr_iso_size);

		let pvd = new DataView(await infile.slice(0x8000, 0x8800).arrayBuffer());
		// root file = 156
		let root_lba = pvd.getUint32(158, true);
		let root_dir = await read_directory(infile, root_lba);
		let modules_dir_entry;
		let elf_filename;
		let elf_entry;
		for(let file of root_dir.files) {
			if(file.filename == "MODULES" && file.is_directory) {
				modules_dir_entry = file;
			} else if(file.filename == "SYSTEM.CNF;1") {
				elf_filename = get_elf_filename(await infile.slice(file.lba*2048, file.lba*2048+file.size).text());
			}
		}
		if(!elf_filename) throw new Error("Not a valid PS2 disc");
		if(!modules_dir_entry) throw new Error("MODULES folder missing");
		console.log(elf_filename);
		if(!valid_elf_filenames.hasOwnProperty(elf_filename)) {
			throw new Error("Unsupported game");
		}
		for(let file of root_dir.files) {
			if(file.filename == elf_filename && !file.is_directory) {
				elf_entry = file;
			}
		}
		if(!elf_entry) throw new Error("Elf file missing");
		let elf_properties = valid_elf_filenames[elf_filename];
		let bin_blob = await (await fetch(elf_properties.bin)).blob();
		[bin_lba, curr_iso_size] = add_addendum(addendums, bin_blob, curr_iso_size);
		let files_to_add = [
			["NETGITRO.BIN;1", bin_blob.size, bin_lba],
			["PS2DEV9.IRX;1", dev9_irx_blob.size, dev9_irx_lba],
			["NETMAN.IRX;1", netman_irx_blob.size, netman_irx_lba],
			["SMAP.IRX;1", smap_irx_blob.size, smap_irx_lba],
		]
		let modules_dir = await read_directory(infile, modules_dir_entry.lba);
		for(let item of files_to_add) {
			let entry = make_dir_entry(item[0], item[1], item[2]);
			replacements.push([modules_dir_entry.lba*2048 + modules_dir.end, new Blob([entry])]);
			modules_dir.end += entry.byteLength;
		}
		let new_length_buf = new ArrayBuffer(8);
		let new_length_dv = new DataView(new_length_buf);
		new_length_dv.setUint32(0, modules_dir.end, true);
		new_length_dv.setUint32(4, modules_dir.end, false);
		replacements.push([modules_dir_entry.entry_loc + root_lba*2048 + 10, new Blob([new_length_buf])]);
		replacements.push([modules_dir.files[0].entry_loc + modules_dir_entry.lba*2048 + 10, new Blob([new_length_buf])]);
		let new_volumesize_buf = new ArrayBuffer(8);
		let new_volumesize_dv = new DataView(new_volumesize_buf);
		new_volumesize_dv.setUint32(0, Math.ceil(curr_iso_size/2048), true);
		new_volumesize_dv.setUint32(4, Math.ceil(curr_iso_size/2048), false);
		replacements.push([0x8050, new Blob([new_volumesize_buf])]);

		let teo = elf_offset + elf_entry.lba * 2048;
		let heapstart = elf_properties.initial_heapstart;
		console.log((bin_blob.size).toString(16));
		heapstart += bin_blob.size;
		heapstart = (heapstart + 0x7F) & ~0x7F
		let heapstart_upper = (heapstart >> 16) & 0xFFFF;
		let heapstart_lower = (heapstart & 0xFFFF);
		if(heapstart_lower > 0x7FFF) heapstart_upper++;
		for(let item of elf_properties.heapstart_uppers) {
			replacements.push([teo+item, new Blob([buf_uint16(heapstart_upper)])]);
		}
		for(let item of elf_properties.heapstart_lowers) {
			replacements.push([teo+item, new Blob([buf_uint16(heapstart_lower)])]);
		}
		for(let item of elf_properties.heapstart_fulls) {
			replacements.push([teo+item, new Blob([buf_uint32(heapstart)])]);
		}
		for(let [loc, num] of elf_properties.int_patches) {
			replacements.push([teo+loc, new Blob([buf_uint32(num)])]);
		}
		for(let [loc, blob] of elf_properties.other_patches) {
			replacements.push([teo+loc, blob]);
		}
		let binsize_upper = (bin_blob.size >> 16) & 0xFFFF;
		let binsize_lower = (bin_blob.size & 0xFFFF);
		if(binsize_lower > 0x7FFF) binsize_upper++;

		let preloader = new Uint8Array(elf_properties.preloader_patch);
		let preloader_dv = new DataView(preloader.buffer);
		preloader_dv.setUint16(elf_properties.preloader_binsize_upper_offset, binsize_upper, true);
		preloader_dv.setUint16(elf_properties.preloader_binsize_lower_offset, binsize_lower, true);
		replacements.push([teo+elf_properties.preloader_location, new Blob([preloader])]);

		for(let image_patch of elf_properties.image_patches) {
			let imx_file;
			if(image_patch.xgm) {
				let xgm_file = await find_file(infile, image_patch.xgm, root_dir);	
				imx_file = await find_in_xgm(infile, xgm_file.lba, image_patch.imx);
			} else {
				let f = await find_file(infile, image_patch.imx, root_dir);
				imx_file = {offset: f.lba*2048, length: f.size};
			}
			let imx = read_imx(await infile.slice(imx_file.offset, imx_file.offset+imx_file.length).arrayBuffer());
			document.body.appendChild(imx.canvas);
			let overlay = new Image();
			let promise = new Promise((resolve, reject) => {
				overlay.onload = resolve;
				overlay.onerror = reject;
			})
			overlay.src = image_patch.overlay;
			await promise;
			imx.canvas.getContext("2d").drawImage(overlay, 0, 0);
			replacements.push(get_imx_patch(imx, imx_file.offset));
		}

		console.log(replacements);
		console.log(addendums);

		replacements.sort((a,b) => {
			return a[0] - b[0];
		});

		
		let parts = [];
		parts.push(infile.slice(0, replacements[0][0]));
		for(let i = 0; i < replacements.length-1; i++) {
			if(replacements[i][0] + replacements[i][1].size > replacements[i+1][0]) throw new Error("Overlapping replacements");
			parts.push(replacements[i][1]);
			if(replacements[i][0] + replacements[i][1].size != replacements[i+1][0])
				parts.push(infile.slice(replacements[i][0] + replacements[i][1].size, replacements[i+1][0]));
		}
		parts.push(replacements[replacements.length-1][1]);
		parts.push(infile.slice(replacements[replacements.length-1][0] + replacements[replacements.length-1][1].size));
		parts.push(...addendums);
		console.log(parts);
		let outfile = new Blob(parts, {type:isomime});
		console.log(outfile.size);
		saveAs(outfile, elf_filename.substring(0, elf_filename.length-2) + ".netgitaroo.iso");
	} catch(e) {
		console.error(e);
		document.getElementById("error").textContent = e;
	}
}
function buf_uint16(n) {
	let buf = new ArrayBuffer(2);
	let dv = new DataView(buf);
	dv.setUint16(0, n, true);
	return buf;
}
function buf_uint32(n) {
	let buf = new ArrayBuffer(4);
	let dv = new DataView(buf);
	dv.setUint32(0, n, true);
	return buf;
}

function make_dir_entry(filename, size, lba) {
	let date = new Date();
	let buf = new ArrayBuffer(((33 + filename.length) + 16) & ~1); // for some reason ps2 has 0 bytes here dunno why but it doesnt work without it I guess
	let dv = new DataView(buf);
	dv.setUint8(0, buf.byteLength);
	dv.setUint8(1, 0);
	dv.setUint32(2, lba, true);
	dv.setUint32(6, lba, false);
	dv.setUint32(10, size, true);
	dv.setUint32(14, size, false);
	dv.setUint8(18, date.getFullYear() - 1900);
	dv.setUint8(19, date.getMonth()+1);
	dv.setUint8(20, date.getDate());
	dv.setUint8(21, date.getHours());
	dv.setUint8(22, date.getMinutes());
	dv.setUint8(23, date.getSeconds());
	dv.setInt8(24, -Math.round(date.getTimezoneOffset() / 15));
	dv.setUint8(25, 0);
	dv.setUint8(26, 0);
	dv.setUint8(27, 0);
	dv.setUint16(28, 0, true);
	dv.setUint16(30, 0, false);
	dv.setUint8(32, filename.length);
	for(let i = 0; i < filename.length; i++) {
		dv.setUint8(33+i, filename.charCodeAt(i));
	}
	return buf;
}
function add_addendum(addendums_list, blob, curr_iso_size) {
	let ret_lba = 0;
	if(up_to_sector(curr_iso_size) != curr_iso_size) {
		addendums_list.push(new Blob([new ArrayBuffer(up_to_sector(curr_iso_size) - curr_iso_size)]));
		curr_iso_size = up_to_sector(curr_iso_size);
	}
	ret_lba = Math.floor(curr_iso_size / 2048);
	addendums_list.push(blob);
	curr_iso_size += blob.size;
	if(up_to_sector(curr_iso_size) != curr_iso_size) {
		addendums_list.push(new Blob([new ArrayBuffer(up_to_sector(curr_iso_size) - curr_iso_size)]));
		curr_iso_size = up_to_sector(curr_iso_size);
	}
	return [ret_lba, curr_iso_size];
}
function get_elf_filename(system_cnf) {
	let lines = system_cnf.split(/\r?\n/g);
	if(lines[0].startsWith("BOOT2 = cdrom0:\\")) return lines[0].substring(16);
	else throw new Error("Invalid system.cnf\n" + system_cnf);
}
async function read_directory(blob, dir_lba) {
	let dir = new DataView(await blob.slice(dir_lba*2048, dir_lba*2048+2048).arrayBuffer());
	let i = 0;
	let files = [];
	while(i < dir.byteLength && dir.getUint8(i) != 0) {
		let record_length = dir.getUint8(i);
		let filename_length = dir.getUint8(i+32);
		let filename = "";
		for(let j = 0; j < filename_length; j++) {
			filename += String.fromCharCode(dir.getUint8(i+33+j));
		}
		let file = {
			lba: dir.getUint32(i+2, true),
			size: dir.getUint32(i+10, true),
			is_directory: !!(dir.getUint8(i+25) & 2),
			filename,
			entry_loc: i
		};
		files.push(file);
		console.log(file.filename + " (" + file.size + " bytes)");
		i += record_length;
	}
	return {
		files: files,
		end: i
	}
}

async function find_file(blob, path, dir_meta) {
	let parts = path.split(/[\/\\]/g);
	let file;
	for(let i = 0; i < parts.length; i++) {
		file = null;
		for(let f of dir_meta.files) {
			if(f.filename == parts[i]) {
				file = f;
				break;
			}
		}
		if(!file) throw new Error(path + " not found (on part " + parts[i] + ")");
		if(i != parts.length-1) dir_meta = await read_directory(blob, file.lba);
	}
	return file;
}

async function find_in_xgm(blob, lba, name) {
	let off = lba*2048;
	let num_textures = new DataView(await blob.slice(off,off+4).arrayBuffer()).getUint32(0, true);
	off += 8;
	for(let i = 0; i < num_textures; i++) {
		let nulled_name = name + "\0";
		let compared_name = await blob.slice(off+256, off+256+nulled_name.length).text();
		console.log(compared_name);
		let file_size = new DataView(await blob.slice(off+0x114,off+0x118).arrayBuffer()).getUint32(0, true);
		console.log(file_size);
		if(compared_name == nulled_name) {
			return {
				offset: off+0x130,
				length: file_size
			};
		} else {
			off += file_size + 0x130;
		}
	}
	throw new Error("Failed to find " + name + " in XGM file");
}

function up_to_sector(loc) {
	return Math.ceil(loc / 2048) * 2048;
}
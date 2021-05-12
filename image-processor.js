function read_imx(buf) {
	// see http://boring.shoutwiki.com/wiki/.IMX for details on format
	let dv = new DataView(buf);
	console.log(dv.byteLength);
	if(dv.getUint32(0, true) != 0x00584d49);
	let width = dv.getUint32(0x14, true);
	let height = dv.getUint32(0x18, true);
	let canvas = document.createElement("canvas");
	canvas.width = width;
	canvas.height = height;
	let ctx = canvas.getContext("2d");
	let imagedata = ctx.getImageData(0, 0, width, height);
	let format = dv.getUint32(0x1c, true);
	let imagedata_start = 0x28;
	let palette;
	if(format <= 1) {
		let palette_bytes = dv.getUint32(0x24, true);
		palette = new Uint8Array(buf, 0x28, palette_bytes);
		imagedata_start = 0x30 + palette_bytes;
	}
	for(let i = 0; i < width*height; i++) {
		let off;
		let idx;
		if(format == 0) {
			let byte = dv.getUint8(imagedata_start + (i>>1));
			if(i & 1) idx = (byte >> 4) & 0xF;
			else idx = byte & 0xF
		} else {
			off = imagedata_start + i*format;
			if(format == 1) {
				idx = dv.getUint8(off);
			}
		}
		if(format <= 1) {
			imagedata.data[i*4] = palette[idx*4];
			imagedata.data[i*4+1] = palette[idx*4+1];
			imagedata.data[i*4+2] = palette[idx*4+2];
			imagedata.data[i*4+3] = Math.round(palette[idx*4+3] / 0x80 * 0xFF);
		} else if(format == 3) {
			imagedata.data[i*4] = dv.getUint8(off);
			imagedata.data[i*4+1] = dv.getUint8(off+1);
			imagedata.data[i*4+2] = dv.getUint8(off+2);
			imagedata.data[i*4+3] = 0xFF;
		} else if(format == 4) {
			imagedata.data[i*4] = dv.getUint8(off);
			imagedata.data[i*4+1] = dv.getUint8(off+1);
			imagedata.data[i*4+2] = dv.getUint8(off+2);
			imagedata.data[i*4+3] = Math.round(dv.getUint8(off+2) / 0x80 * 0xFF);
		}
	}
	ctx.putImageData(imagedata, 0, 0);
	return {
		canvas,
		palette,
		format,
		imagedata_start
	};
}

function get_imx_patch(imx, file_offset) {
	let imagedata = imx.canvas.getContext("2d").getImageData(0, 0, imx.canvas.width, imx.canvas.height);
	let data = new Uint8Array(imx.canvas.width * imx.canvas.height * (imx.format == 0 ? 0.5 : imx.format));
	for(let i = 0; i < imx.canvas.width*imx.canvas.height; i++) {
		if(imx.format >= 3) {
			let off = i*imx.format;
			data[off] = imagedata.data[i*4];
			data[off+1] = imagedata.data[i*4+1];
			data[off+2] = imagedata.data[i*4+2];
			if(imx.format >= 4) data[off+3] = Math.round(imagedata.data[i*4+3] / 0xFF * 0x80);
		} else {
			let ia = imagedata.data[i*4+3] / 0xFF;
			let ir = imagedata.data[i*4] * ia;
			let ig = imagedata.data[i*4+1] * ia;
			let ib = imagedata.data[i*4+2] * ia;
			let best_index = 0;
			let best_distance = Infinity;
			for(let j = 0; j < imx.palette.length>>2; j++) {
				let pa = imx.palette[j*4+3] / 0x80;
				let pr = imx.palette[j*4] * pa;
				let pg = imx.palette[j*4+1] * pa;
				let pb = imx.palette[j*4+2] * pa;
				let distance = (pa*0xFF-ia*0xFF)**2 + (pr-ir)**2 + (pg-ig)**2 + (pb-ib)**2;
				if(distance < best_distance) {
					best_distance = distance;
					best_index = j;
				}
			}
			if(imx.format == 0) {
				let di = i>>1;
				best_index = best_index & 0xF;
				if(i & 1) data[di] = (data[di] & 0xF) | (best_index << 4);
				else data[di] = (data[di] & 0xF0) | best_index;
			} else {
				data[i] = best_index;
			}
		}
	}
	return [file_offset+imx.imagedata_start, new Blob([data])];
}

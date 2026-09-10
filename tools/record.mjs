// Records frames from the demo running in PPSSPPHeadless, without a display.
//
//   node tools/record.mjs <elf> <outdir> [frames=960] [step=2] [skip=0] [port=45690]
//
// PPSSPPHeadless 1.20 cannot save screenshots on its own, so this drives its
// WebSocket debugger: a breakpoint on the sceDisplaySetFrameBuf stub fires
// once per presented frame, the registers say which VRAM buffer went to the
// screen, and memory.read pulls that buffer out. Every <step>th frame is
// written as <outdir>/frame_NNNNN.png, so step=2 gives 30 fps from a 60 Hz
// demo. The JIT ignores breakpoints added at runtime, hence the interpreter.
//
// Needs: PPSSPPHeadless on PATH, psp-nm in the pspdev container (see
// tools/make-video.sh, which supplies the stub address through the STUB env).
import { spawn } from 'node:child_process';
import { writeFileSync, mkdirSync } from 'node:fs';
import zlib from 'node:zlib';

const [elf, outdir, framesArg, stepArg, skipArg, portArg] = process.argv.slice(2);
const frames = Number(framesArg ?? 960), step = Number(stepArg ?? 2), skip = Number(skipArg ?? 0), port = Number(portArg ?? 45690);
const stub = parseInt(process.env.STUB ?? '', 16);
if (!elf || !outdir || !stub) { console.error('usage: STUB=<hex> node tools/record.mjs <elf> <outdir> [frames] [step] [skip] [port]'); process.exit(2); }
mkdirSync(outdir, { recursive: true });

const sleep = ms => new Promise(r => setTimeout(r, ms));
const child = spawn('PPSSPPHeadless', [elf, ...(process.env.TRACE ? ['-l'] : []), '--graphics=software', '-i', `--debugger=${port}`, '--timeout=100000'], { stdio: ['ignore', 'pipe', 'pipe'] });
let log = ''; child.stdout.on('data', d => log += d); child.stderr.on('data', d => log += d);
const die = msg => { console.error(msg + '\n' + log.split('\n').filter(l => !/^I Restored|^D /.test(l)).slice(-40).join('\n')); child.kill('SIGKILL'); process.exit(1); };

let ws;
for (let i = 0; i < 50 && !ws; i++) {
  await sleep(200);
  try { ws = await new Promise((res, rej) => { const w = new WebSocket(`ws://127.0.0.1:${port}/debugger`); w.onopen = () => res(w); w.onerror = e => rej(e); }); } catch { ws = null; }
}
if (!ws) die('no debugger connection');
const events = [];
ws.onmessage = m => events.push(JSON.parse(m.data));
const send = o => ws.send(JSON.stringify(o));
const waitFor = async (pred, ms = 30000) => { const t = Date.now(); while (Date.now() - t < ms) { const i = events.findIndex(pred); if (i >= 0) return events.splice(i, 1)[0]; await sleep(5); } return null; };

const W = 480, H = 272;
const png = rgba => {
  const raw = Buffer.alloc((W * 4 + 1) * H);
  for (let y = 0; y < H; y++) { raw[y * (W * 4 + 1)] = 0; rgba.copy(raw, y * (W * 4 + 1) + 1, y * W * 4, (y + 1) * W * 4); }
  const chunk = (type, data) => { const len = Buffer.alloc(4); len.writeUInt32BE(data.length); const td = Buffer.concat([Buffer.from(type), data]); const crc = Buffer.alloc(4); crc.writeUInt32BE(zlib.crc32(td) >>> 0); return Buffer.concat([len, td, crc]); };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(W, 0); ihdr.writeUInt32BE(H, 4); ihdr[8] = 8; ihdr[9] = 6;
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw, { level: 6 })), chunk('IEND', Buffer.alloc(0))]);
};
const decode = (src, stride, fmt) => {
  const bpp = fmt === 3 ? 4 : 2, rgba = Buffer.alloc(W * H * 4);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const o = (y * W + x) * 4, i = (y * stride + x) * bpp;
    let R, G, B;
    if (fmt === 3) { R = src[i]; G = src[i + 1]; B = src[i + 2]; }
    else {
      const v = src[i] | (src[i + 1] << 8);
      if (fmt === 0) { R = (v & 31) * 255 / 31; G = ((v >> 5) & 63) * 255 / 63; B = ((v >> 11) & 31) * 255 / 31; }
      else if (fmt === 1) { R = (v & 31) * 255 / 31; G = ((v >> 5) & 31) * 255 / 31; B = ((v >> 10) & 31) * 255 / 31; }
      else { R = (v & 15) * 17; G = ((v >> 4) & 15) * 17; B = ((v >> 8) & 15) * 17; }
    }
    rgba[o] = R; rgba[o + 1] = G; rgba[o + 2] = B; rgba[o + 3] = 255;
  }
  return rgba;
};

send({ event: 'cpu.breakpoint.add', address: stub, enabled: true });
await waitFor(e => e.event === 'cpu.breakpoint.add');
send({ event: 'cpu.resume' });

let written = 0;
const t0 = Date.now();
for (let n = 0; n < skip + frames; n++) {
  // Wait for the breakpoint. The stepping notice can go missing, so after a
  // short wait ask the CPU state directly instead of trusting the event.
  let stopped = false;
  for (let waited = 0; !stopped && waited < 60000; ) {
    if (await waitFor(e => e.event === 'cpu.stepping', 200)) { stopped = true; break; }
    waited += 200;
    send({ event: 'cpu.status' });
    const st = await waitFor(e => e.event === 'cpu.status', 5000);
    if (st && st.stepping) stopped = true;
  }
  if (!stopped) {
    send({ event: 'cpu.status' }); console.error(JSON.stringify(await waitFor(e => e.event === 'cpu.status', 5000)));
    send({ event: 'hle.thread.list' }); console.error(JSON.stringify(await waitFor(e => e.event === 'hle.thread.list', 5000)));
    send({ event: 'cpu.stepping' }); await waitFor(e => e.event === 'cpu.stepping', 5000);
    send({ event: 'cpu.getAllRegs' }); const rg = await waitFor(e => e.event === 'cpu.getAllRegs', 5000);
    if (rg) { const g = rg.categories.find(c => c.name === 'GPR'); console.error('pc', rg.pc?.toString(16), 'ra', g.uintValues[g.registerNames.indexOf('ra')].toString(16), 'sp', g.uintValues[g.registerNames.indexOf('sp')].toString(16)); }
    die(`frame ${n}: breakpoint never hit`);
  }
  events.length = 0;
  if (n >= skip && (n - skip) % step === 0) {
    send({ event: 'cpu.getAllRegs' });
    const regs = await waitFor(e => e.event === 'cpu.getAllRegs');
    const gpr = regs.categories.find(c => c.name === 'GPR');
    const r = name => gpr.uintValues[gpr.registerNames.indexOf(name)];
    const top = r('a0'), stride = r('a1'), fmt = r('a2');
    send({ event: 'memory.read', address: top, size: stride * H * (fmt === 3 ? 4 : 2) });
    const mem = await waitFor(e => e.event === 'memory.read', 30000);
    if (!mem) die(`frame ${n}: memory read failed`);
    writeFileSync(`${outdir}/frame_${String(written).padStart(5, '0')}.png`, png(decode(Buffer.from(mem.base64, 'base64'), stride, fmt)));
    written++;
    if (written % 50 === 0) console.error(`${written} frames, ${((Date.now() - t0) / 1000).toFixed(0)} s`);
  }
  send({ event: 'cpu.resume' });
}
console.log(JSON.stringify({ written, seconds: (Date.now() - t0) / 1000 }));
ws.close(); child.kill('SIGKILL'); process.exit(0);

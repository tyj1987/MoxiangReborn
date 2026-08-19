const fs = require('fs');
const path = require('path');

const root = 'd:\\墨香全套源代码（源码+资源+客户端+服务端+教程）\\墨香【源码】';

function countDir(dir) {
  let files = 0;
  let kb = 0;
  let lines = 0;
  try {
    const items = fs.readdirSync(dir, { withFileTypes: true });
    for (const item of items) {
      const fp = path.join(dir, item.name);
      if (item.isDirectory()) {
        const sub = countDir(fp);
        files += sub.files;
        kb += sub.kb;
        lines += sub.lines;
      } else if (item.name.endsWith('.cpp') || item.name.endsWith('.h') || item.name.endsWith('.inl')) {
        const stat = fs.statSync(fp);
        kb += stat.size / 1024;
        files++;
        try {
          const content = fs.readFileSync(fp, 'utf8');
          lines += content.split('\n').length;
        } catch(e) {}
      }
    }
  } catch(e) {}
  return { files, kb: Math.round(kb), lines };
}

const modules = [
  '[Client]MH',
  '4DYUCHIGX_RENDER',
  '4DyuchiGXGeometry',
  '4DYUCHIGXEXECUTIVE',
  '4DyuchiGRX_myself97',
  '4DyuchiNET_Latest',
  '4DyuchiGRX_common',
  '[CC]Ability',
  '[CC]BattleSystem',
  '[CC]Quest',
  '[CC]Skill',
  '[CC]ServerModule',
  '[CC]Suryun',
  '[Lib]BaseNetwork',
  '[Lib]YHLibrary',
  '[Lib]HSEL',
  'SoundLib',
  '4DyuchiFileStorage',
  '4DyuchiFilePack',
  '[Lib]MHConsole',
  '[Lib]ZipArchive',
  '[Lib]DBThread',
  '[Lib]dx81'
];

let total = { files: 0, kb: 0, lines: 0 };
for (const mod of modules) {
  const dir = path.join(root, mod);
  if (fs.existsSync(dir)) {
    const r = countDir(dir);
    total.files += r.files;
    total.kb += r.kb;
    total.lines += r.lines;
    console.log(`${mod}: ${r.files} files, ${r.kb} KB, ${r.lines} lines`);
  } else {
    console.log(`${mod}: NOT FOUND`);
  }
}
console.log(`\n=== TOTAL: ${total.files} files, ${total.kb} KB, ${total.lines} lines ===`);

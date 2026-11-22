// アカウントの一覧を描く（Webview の中で動く）。中身は extension.js から postMessage で届く。
const vscode = acquireVsCodeApi();
const root = document.getElementById('root');
let data = { accounts: [] };

// 要素を作る。children は文字か要素の配列（文字は textContent なので、名前に < などがあっても崩れない）。
function el(tag, className, children = []) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  for (const child of children) node.append(child);
  return node;
}

// 横いっぱいのボタン（切り替える・保存）。押したら extension.js に { type, id } を送る。
function action(text, type, id) {
  const node = el('button', 'action', [text]);
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}

// 小さなボタン（↑・↓・✕）。title はマウスを乗せたときに出る説明。
function button(text, title, type, id) {
  const node = el('button', 'icon', [text]);
  node.title = title;
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}

// 「17:10 に復活（あと 47分）」。週の枠は日付も付ける。
function resetText(resetsAt, withDate) {
  // 16:59:59.68 のような時刻を、次の分（17:00）に切り上げる
  const reset = new Date(Math.ceil(Date.parse(resetsAt) / 60000) * 60000);
  const minutes = Math.max(1, Math.floor((reset - Date.now()) / 60000));
  const left = minutes >= 1440 ? `${Math.floor(minutes / 1440)}日${Math.floor((minutes % 1440) / 60)}時間`
    : minutes >= 60 ? `${Math.floor(minutes / 60)}時間${minutes % 60}分` : `${minutes}分`;
  const day = withDate ? reset.toLocaleDateString('ja-JP', { month: 'numeric', day: 'numeric', weekday: 'short' }) + ' ' : '';
  const time = reset.toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit' });
  return `${day}${time} に復活（あと ${left}）`;
}

// 1つの枠（5時間・週）を、見出し・バー・復活の時刻の3段にする。
function limitBlock(label, limit, withDate) {
  if (!limit || !limit.resets_at) return el('div', 'limit', [el('div', 'limit-head', [el('span', '', [label]), el('span', 'dim', ['使用量なし'])])]);
  const reset = Date.parse(limit.resets_at) <= Date.now();
  const used = reset ? 0 : Math.round(limit.utilization);
  const left = 100 - used;
  const level = left <= 10 ? 'danger' : left <= 30 ? 'warn' : '';
  const fill = el('div', `fill ${level}`);
  fill.style.width = `${used}%`;
  return el('div', 'limit', [
    el('div', 'limit-head', [el('span', '', [label]), el('span', `left ${level}`, [`残り ${left}%`])]),
    el('div', 'bar', [fill]),
    el('div', 'dim', [reset ? '復活済み' : `${used}% 使用・${resetText(limit.resets_at, withDate)}`]),
  ]);
}

const NOTES = {
  invalid: '保存した鍵が無効（ログインし直して、保存し直す）',
  busy: '混んでいて取れなかった（2分後に聞き直す）',
  expired: '鍵の期限切れで聞けない（切り替えると取れる）',
  error: '取れなかった（通信できない など）',
};

function usageBlock(usage) {
  if (!usage) return [el('div', 'dim', ['使用量: まだ取れていない'])];
  const blocks = [];
  if (usage.fiveHour || usage.sevenDay) {
    blocks.push(limitBlock('5時間の枠', usage.fiveHour, false), limitBlock('週の枠', usage.sevenDay, true));
  }
  if (NOTES[usage.status]) blocks.push(el('div', `note ${usage.status === 'invalid' ? 'danger' : ''}`, [NOTES[usage.status]]));
  if (usage.fetchedAt) {
    const at = new Date(usage.fetchedAt).toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    blocks.push(el('div', 'fetched', [`${at} に取った値`]));
  }
  return blocks;
}

function card(account) {
  const head = el('div', 'card-head', [
    el('span', `dot ${account.isCurrent ? 'on' : ''}`, [account.isCurrent ? '●' : '○']),
    el('div', 'who', [el('div', 'name', [account.name]), el('div', 'dim', [account.email])]),
    button('↑', '上へ', 'up', account.id),
    button('↓', '下へ', 'down', account.id),
    button('✕', '保存を消す（ログアウトはしない）', 'remove', account.id),
  ]);
  const node = el('div', `card ${account.isCurrent ? 'is-current' : ''}`, [head]);
  node.append(...usageBlock(account.usage));
  if (account.isCurrent) node.append(el('div', 'current', ['使用中']));
  if (!account.isCurrent) node.append(action('このアカウントに切り替える', 'switch', account.id));
  return node;
}

function render() {
  const children = data.accounts.map(card);
  // 今のアカウントが保存されていなければ、一番上に保存のボタンを出す
  if (data.unsaved) children.unshift(el('div', 'card', [el('div', 'dim', [`${data.unsaved} は保存されていません。`]), action('今のアカウントを保存', 'save')]));
  if (children.length === 0) children.push(el('div', 'dim', ['Claude で /login してから、上の「+」で今のアカウントを保存してください。']));
  root.replaceChildren(...children);
}

window.addEventListener('message', (event) => {
  data = event.data;
  render();
});
// 「あと〇分」を進めるため、30秒ごとに描き直す（値は extension.js が聞き直して送ってくる）
setInterval(render, 30 * 1000);
vscode.postMessage({ type: 'ready' });

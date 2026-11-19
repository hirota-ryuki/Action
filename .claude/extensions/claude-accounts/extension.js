// Claude アカウントの切り替え。
// ログイン情報（~/.claude/.credentials.json）と、だれでログインしているか（~/.claude.json の oauthAccount）を
// アカウントごとに ~/.claude/account-profiles/ へ保存しておき、押されたら入れ替える。
const vscode = require('vscode');
const fs = require('fs');
const os = require('os');
const path = require('path');

const HOME = os.homedir();
const CREDENTIALS_PATH = path.join(HOME, '.claude', '.credentials.json');
const CONFIG_PATH = path.join(HOME, '.claude.json');
const PROFILES_DIR = path.join(HOME, '.claude', 'account-profiles');

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

// 今ログインしているアカウント（~/.claude.json の oauthAccount）。ログインしていなければ null。
function currentAccount() {
  try {
    return readJson(CONFIG_PATH).oauthAccount || null;
  } catch {
    return null;
  }
}

// 保存したアカウントの一覧。1つのファイルに { name, oauthAccount, credentials } が入っている。
function loadProfiles() {
  if (!fs.existsSync(PROFILES_DIR)) return [];
  return fs.readdirSync(PROFILES_DIR)
    .filter((f) => f.endsWith('.json'))
    .map((f) => ({ file: path.join(PROFILES_DIR, f), ...readJson(path.join(PROFILES_DIR, f)) }))
    .sort((a, b) => a.name.localeCompare(b.name));
}

// 書きかけのファイルを Claude に読まれないよう、別の名前に書いてから置き換える。
function writeJson(file, value) {
  const temp = file + '.tmp';
  fs.writeFileSync(temp, JSON.stringify(value, null, 2), 'utf8');
  fs.renameSync(temp, file);
}

function findProfile(accountUuid) {
  return loadProfiles().find((p) => p.oauthAccount.accountUuid === accountUuid);
}

// 今のログイン情報を、そのアカウントの保存ファイルに書く（ないときは作る）。
function saveCurrent(name) {
  const account = currentAccount();
  const saved = findProfile(account.accountUuid);
  const file = saved ? saved.file : path.join(PROFILES_DIR, account.accountUuid + '.json');
  fs.mkdirSync(PROFILES_DIR, { recursive: true });
  writeJson(file, { name, oauthAccount: account, credentials: readJson(CREDENTIALS_PATH) });
}

// 保存したアカウントのログイン情報を、Claude が読む2つのファイルに書き戻す。
function applyProfile(profile) {
  writeJson(CREDENTIALS_PATH, profile.credentials);
  const config = readJson(CONFIG_PATH);
  config.oauthAccount = profile.oauthAccount;
  writeJson(CONFIG_PATH, config);
}

// 並び順と使用量は、VSCode がこのPCに覚えておく（context.globalState）。
let state;

// 並び順（accountUuid を上から並べた配列）のとおりに並べ替える。並び順にないものは後ろ（名前の順のまま）。
function sortByOrder(profiles) {
  const order = state.get('order', []);
  const rank = (p) => {
    const i = order.indexOf(p.oauthAccount.accountUuid);
    return i < 0 ? order.length : i;
  };
  return profiles.sort((a, b) => rank(a) - rank(b));
}

const USAGE_URL = 'https://api.anthropic.com/api/oauth/usage';
const CURRENT_GAP_MS = 30 * 1000;      // 使用中のアカウントを、続けて聞かない間（Claude の会話が進むたびに聞く）
const OTHER_GAP_MS = 5 * 60 * 1000;    // ほかのアカウントを、続けて聞かない間
const BUSY_WAIT_MS = 2 * 60 * 1000;    // 429（混んでいる）のあと、聞かずに待つ間
const lastAsked = {};                  // アカウントの番号 → 最後に聞いた時刻
const busyUntil = {};                  // アカウントの番号 → この時刻までは聞かない
let updating = false;                  // 聞いている最中か
let again = null;                      // 聞いている最中に頼まれたら、終わったあとにもう一度聞く（null・false・true は force）

// 1つのアカウントの使用量を聞く。返すのは { fiveHour, sevenDay } か、取れなかった理由（status）。
async function fetchUsage(credentials) {
  const oauth = credentials.claudeAiOauth;
  if (!oauth || oauth.expiresAt < Date.now()) return { status: 'expired' };
  const response = await fetch(USAGE_URL, {
    headers: { Authorization: `Bearer ${oauth.accessToken}`, 'anthropic-beta': 'oauth-2025-04-20' },
  });
  // 401 は、鍵がサーバーで無効にされている（/logout したときなど）。429 は、聞く回数が多すぎる
  if (response.status === 401) return { status: 'invalid' };
  if (response.status === 429) return { status: 'busy' };
  if (!response.ok) return { status: 'error' };
  const body = await response.json();
  return { status: 'ok', fiveHour: body.five_hour, sevenDay: body.seven_day, fetchedAt: Date.now() };
}

// 間があいたアカウントの使用量を聞き直す。force なら間を待たない（429 のあとは待つ）。
async function updateUsage(view, force) {
  if (updating) {
    again = again || force;
    return;
  }
  updating = true;
  try {
    const current = currentAccount();
    const usage = state.get('usage', {});
    for (const p of loadProfiles()) {
      const id = p.oauthAccount.accountUuid;
      const isCurrent = current && id === current.accountUuid;
      const gap = isCurrent ? CURRENT_GAP_MS : OTHER_GAP_MS;
      if (Date.now() < (busyUntil[id] || 0)) continue;
      if (!force && Date.now() - (lastAsked[id] || 0) < gap) continue;
      lastAsked[id] = Date.now();
      let result;
      try {
        result = await fetchUsage(isCurrent ? readJson(CREDENTIALS_PATH) : p.credentials);
      } catch {
        result = { status: 'error' };   // 通信できない
      }
      if (result.status === 'busy') busyUntil[id] = Date.now() + BUSY_WAIT_MS;
      // 取れなかったときは、前に取れた値を残して理由だけ替える（理由は毎回替わるので、前の理由は残らない）
      usage[id] = result.status === 'ok' ? result : { ...usage[id], status: result.status };
    }
    await state.update('usage', usage);
  } finally {
    updating = false;
  }
  view.render();
  if (again !== null) {
    const nextForce = again;
    again = null;
    updateUsage(view, nextForce);
  }
}

// 保存し直したアカウントの、前に取った使用量を消す（新しい鍵で取り直すため）。
async function forgetUsage(accountUuid) {
  const usage = state.get('usage', {});
  delete usage[accountUuid];
  await state.update('usage', usage);
  delete lastAsked[accountUuid];
  delete busyUntil[accountUuid];
}

// 画面に渡す中身。アカウントを並び順に並べ、使用量を添える。
function viewData() {
  const current = currentAccount();
  const usage = state.get('usage', {});
  const profiles = loadProfiles();
  sortByOrder(profiles);
  const accounts = profiles.map((p) => ({
    id: p.oauthAccount.accountUuid,
    name: p.name,
    email: p.oauthAccount.emailAddress,
    isCurrent: !!current && p.oauthAccount.accountUuid === current.accountUuid,
    usage: usage[p.oauthAccount.accountUuid] || null,
  }));
  const data = { accounts };
  // 今のアカウントが保存されていなければ、保存を勧める
  if (current && !accounts.some((a) => a.isCurrent)) data.unsaved = current.emailAddress;
  return data;
}

// アクティビティバーに出す画面（Webview）。描くのは media/view.js で、ここは中身を渡すだけ。
class AccountsView {
  constructor(extensionUri) {
    this.extensionUri = extensionUri;
    this.view = null;
  }

  resolveWebviewView(webviewView) {
    this.view = webviewView;
    const webview = webviewView.webview;
    const media = vscode.Uri.joinPath(this.extensionUri, 'media');
    webview.options = { enableScripts: true, localResourceRoots: [media] };
    const nonce = String(Math.random()).slice(2);
    const script = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.css'));
    webview.html = `<!DOCTYPE html><html lang="ja"><head><meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';">
<link rel="stylesheet" href="${style}"></head>
<body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`;
    webview.onDidReceiveMessage((message) => onMessage(this, message));
    webviewView.onDidChangeVisibility(() => {
      if (webviewView.visible) updateUsage(this, false);
    });
  }

  render() {
    if (this.view) this.view.webview.postMessage(viewData());
  }

  // 保存・切り替え・消したあと。すぐ描き直し、使用量もすぐ聞き直す。
  refresh() {
    this.render();
    updateUsage(this, true);
  }
}

async function onSave(view) {
  const account = currentAccount();
  if (!account || !fs.existsSync(CREDENTIALS_PATH)) {
    vscode.window.showWarningMessage('Claude にログインしていません。Claude で /login してから保存してください。');
    return;
  }
  const saved = findProfile(account.accountUuid);
  const name = await vscode.window.showInputBox({
    prompt: `${account.emailAddress} を一覧に出す名前`,
    value: saved ? saved.name : account.emailAddress,
  });
  if (!name) return;
  saveCurrent(name);
  await forgetUsage(account.accountUuid);
  view.refresh();
}

async function onSwitch(view, profile) {
  const current = currentAccount();
  if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;
  // 今のアカウントの最新のログイン情報を先に取っておく（使っている間に Claude が更新しているため）。
  const saved = current && findProfile(current.accountUuid);
  if (saved) {
    saveCurrent(saved.name);
  } else if (current) {
    const answer = await vscode.window.showWarningMessage(
      `${current.emailAddress} は保存されていません。切り替えると、戻すときにもう一度ログインが要ります。`,
      { modal: true }, '切り替える');
    if (answer !== '切り替える') return;
  }
  applyProfile(profile);
  view.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
}

async function onRemove(view, profile) {
  const answer = await vscode.window.showWarningMessage(
    `${profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(profile.file);
  view.refresh();
}

// 押したアカウントを、1つ上（step = -1）か1つ下（step = 1）と入れ替える。
async function onMove(view, profile, step) {
  const ids = sortByOrder(loadProfiles()).map((p) => p.oauthAccount.accountUuid);
  const from = ids.indexOf(profile.oauthAccount.accountUuid);
  const to = from + step;
  if (to < 0 || to >= ids.length) return;
  [ids[from], ids[to]] = [ids[to], ids[from]];
  await state.update('order', ids);
  view.render();
}

// 画面から届いた知らせ。{ type: 'ready' | 'save' | 'switch' | 'up' | 'down' | 'remove', id }
function onMessage(view, message) {
  if (message.type === 'ready') {
    view.render();
    updateUsage(view, false);
    return;
  }
  if (message.type === 'save') return onSave(view);
  const profile = findProfile(message.id);
  if (!profile) return;
  if (message.type === 'switch') return onSwitch(view, profile);
  if (message.type === 'remove') return onRemove(view, profile);
  if (message.type === 'up') return onMove(view, profile, -1);
  if (message.type === 'down') return onMove(view, profile, 1);
}

// Claude の会話のファイル（~/.claude/projects の下）が書かれたら、少し待って onChange を呼ぶ。
function watchClaude(onChange) {
  const dir = path.join(HOME, '.claude', 'projects');
  if (!fs.existsSync(dir)) return { dispose() {} };
  let timer;
  const watcher = fs.watch(dir, { recursive: true }, () => {
    clearTimeout(timer);
    timer = setTimeout(onChange, 5000);   // 書き終わるのを待つ
  });
  return { dispose: () => { clearTimeout(timer); watcher.close(); } };
}

function activate(context) {
  state = context.globalState;
  const view = new AccountsView(context.extensionUri);
  // 画面が見えているときだけ、間があいたアカウントを聞き直す
  const update = () => {
    if (view.view && view.view.visible) updateUsage(view, false);
  };
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider('claudeAccounts.list', view, { webviewOptions: { retainContextWhenHidden: true } }),
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(view)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => view.refresh()),
    watchClaude(update),
  );
  // 会話が進んでいなくても、1分ごとに聞き直す（間があいたアカウントだけ）
  const timer = setInterval(update, 60 * 1000);
  context.subscriptions.push({ dispose: () => clearInterval(timer) });
}

function deactivate() {}

module.exports = { activate, deactivate };

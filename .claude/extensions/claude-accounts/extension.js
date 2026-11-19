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
const USAGE_INTERVAL_MS = 5 * 60 * 1000;
let usageFetchedAt = 0;

// アカウントの使用量（5時間の枠と週の枠）を取る。鍵の期限が切れていて取れないときは null。
async function fetchUsage(credentials) {
  const oauth = credentials.claudeAiOauth;
  if (!oauth || oauth.expiresAt < Date.now()) return null;
  const response = await fetch(USAGE_URL, {
    headers: { Authorization: `Bearer ${oauth.accessToken}`, 'anthropic-beta': 'oauth-2025-04-20' },
  });
  // 401 は、鍵がサーバーで無効にされている（/logout したときなど）
  if (response.status === 401) return { invalid: true, fetchedAt: Date.now() };
  if (!response.ok) return null;
  const body = await response.json();
  return { fiveHour: body.five_hour, sevenDay: body.seven_day, fetchedAt: Date.now() };
}

// 全部のアカウントの使用量を取り直す。取れなかったアカウントは、前に取った値のまま。
async function updateUsage(provider) {
  usageFetchedAt = Date.now();
  const current = currentAccount();
  const usage = state.get('usage', {});
  for (const p of loadProfiles()) {
    const isCurrent = current && p.oauthAccount.accountUuid === current.accountUuid;
    try {
      const fetched = await fetchUsage(isCurrent ? readJson(CREDENTIALS_PATH) : p.credentials);
      if (fetched) usage[p.oauthAccount.accountUuid] = fetched;
    } catch {
      // 通信できないときは、前に取った値を出す
    }
  }
  await state.update('usage', usage);
  provider.changed.fire();
}

// 「残り 96%・17:00 に復活（あと 4時間42分）」のような1行の文にする。
function formatLimit(label, limit, withDate) {
  if (!limit || !limit.resets_at) return `${label}: 使用量なし`;
  if (Date.parse(limit.resets_at) <= Date.now()) return `${label}: 残り 100%（復活済み）`;
  // 16:59:59.68 のような時刻を、次の分（17:00）に切り上げる
  const reset = new Date(Math.ceil(Date.parse(limit.resets_at) / 60000) * 60000);
  const minutes = Math.max(1, Math.floor((reset - Date.now()) / 60000));
  const left = minutes >= 1440 ? `${Math.floor(minutes / 1440)}日${Math.floor((minutes % 1440) / 60)}時間`
    : minutes >= 60 ? `${Math.floor(minutes / 60)}時間${minutes % 60}分` : `${minutes}分`;
  const day = withDate ? reset.toLocaleDateString('ja-JP', { month: 'numeric', day: 'numeric', weekday: 'short' }) + ' ' : '';
  const time = reset.toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit' });
  return `${label}: 残り ${100 - Math.round(limit.utilization)}%・${day}${time} に復活（あと ${left}）`;
}

// アカウントの行の下に出す、使用量の行。
function usageItems(profile) {
  const usage = state.get('usage', {})[profile.oauthAccount.accountUuid];
  if (!usage) return [new vscode.TreeItem('使用量: まだ取れていない（このアカウントに切り替えると取れる）')];
  if (usage.invalid) return [new vscode.TreeItem('保存した鍵が無効（ログインし直して、保存し直す）')];
  const items = [formatLimit('5時間', usage.fiveHour, false), formatLimit('週', usage.sevenDay, true)]
    .map((text) => new vscode.TreeItem(text));
  if (Date.now() - usage.fetchedAt > USAGE_INTERVAL_MS * 2) {
    const at = new Date(usage.fetchedAt).toLocaleString('ja-JP', { month: 'numeric', day: 'numeric', hour: '2-digit', minute: '2-digit' });
    items.push(new vscode.TreeItem(`（${at} に取った値。鍵の期限切れなどで取り直せていない）`));
  }
  return items;
}

// アクティビティバーに出す一覧。今のアカウントには印を付ける。
class AccountsProvider {
  constructor() {
    this.changed = new vscode.EventEmitter();
    this.onDidChangeTreeData = this.changed.event;
  }

  refresh() {
    usageFetchedAt = 0;
    this.changed.fire();
  }

  getTreeItem(item) {
    return item;
  }

  getChildren(element) {
    if (element) return usageItems(element.profile);
    if (Date.now() - usageFetchedAt > USAGE_INTERVAL_MS) updateUsage(this);
    const current = currentAccount();
    const profiles = loadProfiles();
    sortByOrder(profiles);
    return profiles.map((p) => {
      const isCurrent = current && p.oauthAccount.accountUuid === current.accountUuid;
      const item = new vscode.TreeItem(p.name);
      item.description = p.oauthAccount.emailAddress + (isCurrent ? '（使用中）' : '');
      item.iconPath = new vscode.ThemeIcon(isCurrent ? 'pass-filled' : 'account');
      item.command = { command: 'claudeAccounts.switch', title: '切り替える', arguments: [p] };
      item.profile = p;
      item.contextValue = 'account';
      item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
      return item;
    });
  }
}

async function onSave(provider) {
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
  provider.refresh();
}

async function onSwitch(provider, profile) {
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
  provider.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
}

async function onRemove(provider, item) {
  const answer = await vscode.window.showWarningMessage(
    `${item.profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(item.profile.file);
  provider.refresh();
}

// 押した行を、1つ上（step = -1）か1つ下（step = 1）と入れ替える。
async function onMove(provider, item, step) {
  const ids = sortByOrder(loadProfiles()).map((p) => p.oauthAccount.accountUuid);
  const from = ids.indexOf(item.profile.oauthAccount.accountUuid);
  const to = from + step;
  if (to < 0 || to >= ids.length) return;
  [ids[from], ids[to]] = [ids[to], ids[from]];
  await state.update('order', ids);
  provider.changed.fire();
}

function activate(context) {
  state = context.globalState;
  const provider = new AccountsProvider();
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('claudeAccounts.list', provider),
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(provider)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => provider.refresh()),
    vscode.commands.registerCommand('claudeAccounts.switch', (p) => onSwitch(provider, p)),
    vscode.commands.registerCommand('claudeAccounts.remove', (item) => onRemove(provider, item)),
    vscode.commands.registerCommand('claudeAccounts.moveUp', (item) => onMove(provider, item, -1)),
    vscode.commands.registerCommand('claudeAccounts.moveDown', (item) => onMove(provider, item, 1)),
  );
  // 「あと〇分」を進めるため、1分ごとに描き直す（5分たっていれば使用量も取り直す）
  const timer = setInterval(() => provider.changed.fire(), 60 * 1000);
  context.subscriptions.push({ dispose: () => clearInterval(timer) });
}

function deactivate() {}

module.exports = { activate, deactivate };

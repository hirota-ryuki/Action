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

// アクティビティバーに出す一覧。今のアカウントには印を付ける。
class AccountsProvider {
  constructor() {
    this.changed = new vscode.EventEmitter();
    this.onDidChangeTreeData = this.changed.event;
  }

  refresh() {
    this.changed.fire();
  }

  getTreeItem(item) {
    return item;
  }

  getChildren() {
    const current = currentAccount();
    return loadProfiles().map((p) => {
      const isCurrent = current && p.oauthAccount.accountUuid === current.accountUuid;
      const item = new vscode.TreeItem(p.name);
      item.description = p.oauthAccount.emailAddress + (isCurrent ? '（使用中）' : '');
      item.iconPath = new vscode.ThemeIcon(isCurrent ? 'pass-filled' : 'account');
      item.command = { command: 'claudeAccounts.switch', title: '切り替える', arguments: [p] };
      item.profile = p;
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

function activate(context) {
  const provider = new AccountsProvider();
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('claudeAccounts.list', provider),
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(provider)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => provider.refresh()),
    vscode.commands.registerCommand('claudeAccounts.switch', (p) => onSwitch(provider, p)),
    vscode.commands.registerCommand('claudeAccounts.remove', (item) => onRemove(provider, item)),
  );
}

function deactivate() {}

module.exports = { activate, deactivate };

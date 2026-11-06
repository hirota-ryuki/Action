# VSCode の拡張用の保存領域（%APPDATA%\Code\User\globalStorage\state.vscdb）を読み書きする部品。
# claude-sessions.ps1 から . （ドット）で読み込んで使う。
#
# state.vscdb は SQLite のファイルで、ItemTable(key, value) に「拡張のID → その拡張の globalState（JSON）」が入っている。
# Claude Code 拡張は、アーカイブした会話の ID を globalState の hiddenSessionIds（配列）に、
# アーカイブを戻した時刻を sessionUnarchivedAt（{ 会話ID: ミリ秒 }）に入れている。
#
# SQLite は Windows に入っている winsqlite3.dll を使う（追加のインストールはいらない）。
#
# 注意: VSCode が開いている間は、拡張が globalState をメモリに持っていて、変えるたびに丸ごと書き戻す。
#       そのため書き込み（Set-ClaudeExtensionState）は VSCode を閉じているときだけ行う。読むのはいつでもよい。

$VscodeStateDb = Join-Path $env:APPDATA 'Code\User\globalStorage\state.vscdb'
$ClaudeExtKey  = 'anthropic.claude-code'

if (-not ('ClaudeSessions.VsState' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections;
using System.Collections.Generic;
using System.Management.Automation;
using System.Runtime.InteropServices;

namespace ClaudeSessions {
    public static class VsState {
        const string Dll = "winsqlite3.dll";
        const int SQLITE_OK = 0, SQLITE_ROW = 100, SQLITE_DONE = 101;
        const int OPEN_READONLY = 0x1, OPEN_READWRITE = 0x2;

        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern int sqlite3_open_v2(byte[] filename, out IntPtr db, int flags, IntPtr vfs);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern int sqlite3_close(IntPtr db);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern int sqlite3_busy_timeout(IntPtr db, int ms);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        static extern int sqlite3_prepare16_v2(IntPtr db, string sql, int nBytes, out IntPtr stmt, IntPtr tail);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        static extern int sqlite3_bind_text16(IntPtr stmt, int index, string text, int nBytes, IntPtr destructor);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern int sqlite3_step(IntPtr stmt);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr sqlite3_column_text16(IntPtr stmt, int col);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern int sqlite3_finalize(IntPtr stmt);
        [DllImport(Dll, CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr sqlite3_errmsg16(IntPtr db);

        // SQLITE_TRANSIENT: 渡した文字列を SQLite 側にコピーさせる
        static readonly IntPtr Transient = new IntPtr(-1);

        static IntPtr Open(string path, bool write) {
            IntPtr db;
            byte[] name = System.Text.Encoding.UTF8.GetBytes(path + "\0");
            int rc = sqlite3_open_v2(name, out db, write ? OPEN_READWRITE : OPEN_READONLY, IntPtr.Zero);
            if (rc != SQLITE_OK) {
                string msg = db != IntPtr.Zero ? Marshal.PtrToStringUni(sqlite3_errmsg16(db)) : rc.ToString();
                if (db != IntPtr.Zero) sqlite3_close(db);
                throw new Exception("state.vscdb を開けません: " + msg);
            }
            sqlite3_busy_timeout(db, 5000);
            return db;
        }

        static void Check(IntPtr db, int rc) {
            if (rc != SQLITE_OK) throw new Exception("SQLite: " + Marshal.PtrToStringUni(sqlite3_errmsg16(db)));
        }

        // PowerShell が値にかぶせる包み（PSObject）を外す。外さないと JSON にできない。
        public static object Plain(object v) {
            PSObject p = v as PSObject;
            if (p != null) v = p.BaseObject;
            if (v == null || v is string || v is ValueType) return v;
            IDictionary d = v as IDictionary;
            if (d != null) {
                Dictionary<string, object> r = new Dictionary<string, object>();
                foreach (DictionaryEntry e in d) r[Convert.ToString(e.Key)] = Plain(e.Value);
                return r;
            }
            IEnumerable list = v as IEnumerable;
            if (list != null) {
                List<object> r = new List<object>();
                foreach (object x in list) r.Add(Plain(x));
                return r.ToArray();
            }
            return v;
        }

        // key に一致する（大文字小文字は区別しない）行の key と value を返す。なければ null。
        public static string[] Get(string path, string key) {
            IntPtr db = Open(path, false);
            try {
                IntPtr st;
                Check(db, sqlite3_prepare16_v2(db, "SELECT key, value FROM ItemTable WHERE lower(key) = lower(?1) LIMIT 1", -1, out st, IntPtr.Zero));
                try {
                    Check(db, sqlite3_bind_text16(st, 1, key, -1, Transient));
                    if (sqlite3_step(st) != SQLITE_ROW) return null;
                    return new string[] {
                        Marshal.PtrToStringUni(sqlite3_column_text16(st, 0)),
                        Marshal.PtrToStringUni(sqlite3_column_text16(st, 1))
                    };
                } finally { sqlite3_finalize(st); }
            } finally { sqlite3_close(db); }
        }

        // key の value を書き換える（行は Get で見つかったものだけを対象にする）。
        public static void Set(string path, string key, string value) {
            IntPtr db = Open(path, true);
            try {
                IntPtr st;
                Check(db, sqlite3_prepare16_v2(db, "UPDATE ItemTable SET value = ?2 WHERE key = ?1", -1, out st, IntPtr.Zero));
                try {
                    Check(db, sqlite3_bind_text16(st, 1, key, -1, Transient));
                    Check(db, sqlite3_bind_text16(st, 2, value, -1, Transient));
                    int rc = sqlite3_step(st);
                    if (rc != SQLITE_DONE) throw new Exception("SQLite: " + Marshal.PtrToStringUni(sqlite3_errmsg16(db)));
                } finally { sqlite3_finalize(st); }
            } finally { sqlite3_close(db); }
        }
    }
}
'@
}

Add-Type -AssemblyName System.Web.Extensions
function New-JsonSerializer {
    $s = New-Object System.Web.Script.Serialization.JavaScriptSerializer
    $s.MaxJsonLength = [int]::MaxValue
    return $s
}

# Claude 拡張の globalState を読む。返り値: @{ Key = 実際のキー名; State = Dictionary } または $null
function Get-ClaudeExtensionState {
    if (-not (Test-Path $VscodeStateDb)) { return $null }
    $row = [ClaudeSessions.VsState]::Get($VscodeStateDb, $ClaudeExtKey)
    if (-not $row) { return $null }
    $state = (New-JsonSerializer).DeserializeObject($row[1])
    if ($state -isnot [System.Collections.Generic.Dictionary[string, object]]) { return $null }
    return @{ Key = $row[0]; State = $state }
}

# Claude 拡張の globalState を書き戻す。VSCode を閉じているときだけ呼ぶこと。
function Set-ClaudeExtensionState($ext) {
    $json = (New-JsonSerializer).Serialize([ClaudeSessions.VsState]::Plain($ext.State))
    [ClaudeSessions.VsState]::Set($VscodeStateDb, $ext.Key, $json)
}

# VSCode（Code.exe）が動いているか
function Test-VscodeRunning {
    return [bool](Get-Process -Name 'Code' -ErrorAction SilentlyContinue)
}

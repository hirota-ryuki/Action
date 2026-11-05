# Store this repository's Claude Code conversations in OneDrive so that the
# same conversations (tabs) can be resumed on every PC that clones the repo.
#
# Claude Code keeps conversations in  <config>\projects\<encoded repo path>\ .
# This script replaces that folder with a junction to
#   <OneDrive>\12_ClaudeSessions\<repo folder name>\
# moving any conversations that are already there.
#
# It runs automatically from the SessionStart hook in .claude/settings.json.
# It can also be run by hand:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .claude\scripts\link-sessions.ps1

param([string]$RepoDir = $env:CLAUDE_PROJECT_DIR)

$ErrorActionPreference = 'Stop'
$tag = '[session-sync]'

if (-not $RepoDir) { $RepoDir = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$RepoDir = $RepoDir.TrimEnd('\', '/')

$oneDrive = if ($env:OneDriveCommercial) { $env:OneDriveCommercial }
            elseif ($env:OneDrive) { $env:OneDrive }
            else { $null }
if (-not $oneDrive) {
    Write-Output "$tag OneDrive was not found on this PC. Conversations stay on this PC only."
    exit 0
}

$configDir = if ($env:CLAUDE_CONFIG_DIR) { $env:CLAUDE_CONFIG_DIR } else { Join-Path $env:USERPROFILE '.claude' }
$encoded   = $RepoDir -replace '[^A-Za-z0-9]', '-'
$local     = Join-Path $configDir "projects\$encoded"
$repoName  = Split-Path $RepoDir -Leaf
$shared    = Join-Path $oneDrive "12_ClaudeSessions\$repoName"

function Copy-Newer([string]$from, [string]$to) {
    robocopy $from $to /E /XO /R:2 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE): $from -> $to" }
}

$item = Get-Item -LiteralPath $local -Force -ErrorAction SilentlyContinue
if ($item -and $item.LinkType -eq 'Junction') { exit 0 }   # already linked

New-Item -ItemType Directory -Force -Path $shared | Out-Null

$backup = $null
if ($item) {
    Copy-Newer $local $shared
    $backup = "$local.local-backup-" + (Get-Date -Format 'yyyyMMdd-HHmmss')
    try {
        Rename-Item -LiteralPath $local -NewName (Split-Path $backup -Leaf)
    } catch {
        Write-Output "$tag Could not switch the conversation folder because it is in use. Close every Claude tab in VSCode, then run: powershell -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
        exit 0
    }
}

# A running session may recreate the folder between the rename and the link.
for ($i = 0; $i -lt 3; $i++) {
    if (Test-Path -LiteralPath $local) {
        Copy-Newer $local $shared
        Remove-Item -LiteralPath $local -Recurse -Force
    }
    try { New-Item -ItemType Junction -Path $local -Target $shared | Out-Null; break }
    catch { if ($i -eq 2) { throw } }
}

# Pick up anything written to the old folder during the switch.
if ($backup) { Copy-Newer $backup $shared }

Write-Output "$tag Conversations for this repository are now stored in OneDrive ($shared). Old local copy: $backup"

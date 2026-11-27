$ErrorActionPreference = "Stop"

$Repo = "AchillesOrg/Kestral"
$InstallDir = if ($env:KESTRAL_INSTALL_DIR) { $env:KESTRAL_INSTALL_DIR } else { Join-Path $env:LOCALAPPDATA "Programs\Kestral" }
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

$Release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest"
$Tag = $Release.tag_name
$Url = "https://github.com/$Repo/releases/download/$Tag/kestral-windows-x64.exe"
$Tmp = Join-Path $InstallDir "kestral.tmp.exe"
$Dest = Join-Path $InstallDir "kestral.exe"

Write-Host "Downloading $Url..."
Invoke-WebRequest -Uri $Url -OutFile $Tmp
Move-Item -Force $Tmp $Dest

$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
if (-not ($UserPath -split ";" | Where-Object { $_ -eq $InstallDir })) {
  [Environment]::SetEnvironmentVariable("Path", "$InstallDir;$UserPath", "User")
  $env:Path = "$InstallDir;$env:Path"
}

Write-Host "Installed Kestral to $Dest"
& $Dest --version

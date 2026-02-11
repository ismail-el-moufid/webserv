Write-Host "Setting up Git configuration..."

# Initialize Git if needed
if (-not (Test-Path ".git")) {
    Write-Host "Initializing new git repository..."
    git init
}

# Define remotes
$remotes = [ordered]@{
    "origin"  = "https://github.com/Abdelkader37/Web-serv.git"
    "remote2" = "https://github.com/ismail-el-moufid/webserv.git"
    "remote3" = "https://github.com/example/repo3.git"
}

# remove any existing remotes
git remote | ForEach-Object { git remote remove $_ }

# Add origin remote
git remote add origin $remotes["origin"]

# Configure 'push' to update ALL remotes when pushing to origin
Write-Host "Configuring push behavior..."

# Reset push URLs for origin to clear any previous state
git config --unset-all remote.origin.pushurl 2>$null

git remote set-url --push origin $remotes["origin"]
git remote set-url --add --push origin $remotes["remote2"]
git remote set-url --add --push origin $remotes["remote3"]

Write-Host "Push configuration acting on 'origin' will now push to all 3."

# Configure 'pull'
Write-Host "Configuring pull behavior..."

# Ensure we start fresh for fetch URLs to avoid duplicates on re-run
git config --unset-all remote.origin.url 2>$null

git config --add remote.origin.url $remotes["origin"]

Write-Host "Fetching from origin and setting upstream..."
git fetch origin
git branch --set-upstream-to=origin/master master

if (Test-Path ".git") {
    New-Item -ItemType File -Path ".git/setup_done" -Force | Out-Null
}

Write-Host "Setup complete!"

git pull
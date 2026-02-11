#!/bin/sh

echo "Setting up Git configuration..."

# Initialize Git if needed
if [ ! -d ".git" ]; then
    echo "Initializing new git repository..."
    git init
fi

# Define remotes
REMOTE_ORIGIN="https://github.com/Abdelkader37/Web-serv.git"
REMOTE_2="https://github.com/ismail-el-moufid/webserv.git"
REMOTE_3="https://github.com/example/repo3.git"

# Remove any existing remotes
for remote in $(git remote); do
    git remote remove "$remote"
done

# Add origin remote
git remote add origin "$REMOTE_ORIGIN"

# Configure 'push' to update ALL remotes when pushing to origin
echo "Configuring push behavior..."

# Reset push URLs for origin to clear any previous state
git config --unset-all remote.origin.pushurl 2>/dev/null

git remote set-url --push origin "$REMOTE_ORIGIN"
git remote set-url --add --push origin "$REMOTE_2"
git remote set-url --add --push origin "$REMOTE_3"

echo "Push configuration acting on 'origin' will now push to all 3."

# Configure 'pull'
echo "Configuring pull behavior..."

# Ensure we start fresh for fetch URLs to avoid duplicates on re-run
git config --unset-all remote.origin.url 2>/dev/null

git config --add remote.origin.url "$REMOTE_ORIGIN"

echo "Fetching from origin and setting upstream..."
git fetch origin
git branch --set-upstream-to=origin/master master

touch .git/setup_done
echo "Setup complete!"

git pull

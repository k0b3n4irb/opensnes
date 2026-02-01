# /submodules - Manage Git Submodules Lifecycle

Check for upstream updates, sync forks, and update submodules.

## Usage
```
/submodules status     # Show status of all submodules (commits behind/ahead)
/submodules check      # Check for upstream updates
/submodules sync       # Fetch upstream, merge, push to origin
/submodules sync <name> # Sync specific submodule (wla-dx, cproc, qbe)
```

## Submodule Configuration

| Submodule | Origin (fork) | Upstream (original) | Branch |
|-----------|---------------|---------------------|--------|
| compiler/wla-dx | k0b3n4irb/wla-dx | vhelin/wla-dx | master |
| compiler/cproc | k0b3n4irb/cproc | ~mcf/cproc (sr.ht) | master |
| compiler/qbe | k0b3n4irb/qbe | c9x.me/git/qbe | master |

## Implementation

### Status
```bash
cd $OPENSNES_HOME
echo "=== Submodule Status ==="
git submodule foreach 'echo "--- $name ---" && git fetch origin && git fetch upstream 2>/dev/null && git status -sb'
```

### Check for updates
```bash
cd $OPENSNES_HOME
git submodule foreach '
  echo "=== $name ==="
  git fetch upstream 2>/dev/null || echo "No upstream configured"
  LOCAL=$(git rev-parse HEAD)
  UPSTREAM=$(git rev-parse upstream/master 2>/dev/null || git rev-parse upstream/main 2>/dev/null)
  if [ "$LOCAL" = "$UPSTREAM" ]; then
    echo "Up to date with upstream"
  else
    echo "Behind upstream by:"
    git log --oneline HEAD..upstream/master 2>/dev/null || git log --oneline HEAD..upstream/main 2>/dev/null
  fi
'
```

### Sync (fetch upstream, merge, push to origin)
```bash
cd $OPENSNES_HOME/compiler/$SUBMODULE
git fetch upstream
git checkout master
git merge upstream/master
git push origin master
cd $OPENSNES_HOME
git add compiler/$SUBMODULE
echo "Submodule $SUBMODULE updated. Commit the change in main repo when ready."
```

## Workflow

1. **Check**: Run `/submodules check` periodically to see if upstream has updates
2. **Review**: Look at the commits to understand what changed
3. **Sync**: Run `/submodules sync <name>` to merge upstream into your fork
4. **Test**: Build and test the project with `make clean && make`
5. **Commit**: Commit the submodule update in the main opensnes repo

## Troubleshooting

### Merge conflicts
If upstream changes conflict with your modifications:
```bash
cd compiler/<submodule>
git fetch upstream
git merge upstream/master
# Resolve conflicts manually
git add .
git commit
git push origin master
```

### Reset to upstream
If you want to discard local changes and match upstream:
```bash
cd compiler/<submodule>
git fetch upstream
git reset --hard upstream/master
git push origin master --force
```

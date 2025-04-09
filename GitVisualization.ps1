function Show-GitInternals {
    # Clear the screen for better presentation
    Clear-Host
    
    Write-Host "============== BRANCH STRUCTURE ==============" -ForegroundColor Cyan
    git log --graph --pretty=format:"%C(auto)%h%d %C(reset)%s %C(bold blue)(%an, %ar)" --all -15
    
    Write-Host "`n============== CURRENT BRANCH INFO ==============" -ForegroundColor Cyan
    $CURRENT_BRANCH = git branch --show-current
    $CURRENT_COMMIT = git rev-parse HEAD
    Write-Host "Current branch: $CURRENT_BRANCH" -ForegroundColor Green
    Write-Host "HEAD commit: $CURRENT_COMMIT" -ForegroundColor Green
    
    Write-Host "`n============== COMMIT OBJECT ==============" -ForegroundColor Cyan
    git cat-file -p $CURRENT_COMMIT
    
    Write-Host "`n============== TREE OBJECT ==============" -ForegroundColor Cyan
    $TREE_HASH = git show --format=%T -s HEAD
    Write-Host "Tree hash: $TREE_HASH" -ForegroundColor Green
    git cat-file -p $TREE_HASH
    
    Write-Host "`n============== SAMPLE BLOB OBJECT ==============" -ForegroundColor Cyan
    $BLOBS = git ls-tree -r HEAD | Where-Object { $_ -match "blob" }
    if ($BLOBS) {
        $SAMPLE_BLOB = ($BLOBS | Select-Object -First 1) -split "\s+"
        $BLOB_HASH = $SAMPLE_BLOB[2]
        $BLOB_PATH = $SAMPLE_BLOB[3]
        Write-Host "Blob hash for $BLOB_PATH : $BLOB_HASH" -ForegroundColor Green
        git cat-file -p $BLOB_HASH | Select-Object -First 10
    } else {
        Write-Host "No blob objects found in repository." -ForegroundColor Yellow
    }
    
    Write-Host "`n============== RECENT DIFF ==============" -ForegroundColor Cyan
    if (git rev-parse HEAD~1 2>$null) {
        git diff --color-words HEAD~1 HEAD | Select-Object -First 20
    } else {
        Write-Host "Repository has only one commit, cannot show diff." -ForegroundColor Yellow
    }
    
    Write-Host "`n============== BRANCH RELATIONSHIPS ==============" -ForegroundColor Cyan
    git branch -a -v
}

# Execute the function
Show-GitInternals
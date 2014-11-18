Write-Host 'checking runtime of modules ...';

foreach ($item in Get-ChildItem $args[0] -Recurse -Include *.node){
    $output = dumpbin /DEPENDENTS $item.FullName;
    if ($output -like '*VCRUNTIME140.dll*') {
        Write-Host "[$item] vcruntime140 found" -ForegroundColor Green
    } else {
        Write-Host "[$item] vcruntime140 NOT FOUND" -ForegroundColor Red
        Write-Host $output
    }
}


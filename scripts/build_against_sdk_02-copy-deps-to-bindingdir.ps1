$msg_prefix='====================== '

Try{

    $deps = Get-ChildItem -Path $env:MAPNIK_SDK\libs -Filter *.dll | % { $_.FullName }

	##COPY DEPENDENCIES TO BINDING DIR
	Write-Output "$msg_prefix copying dependencies to binding dir:"
	foreach($dep in $deps){
    	Write-Output $dep
        Copy-Item $dep $env:NODEMAPNIK_BINDING_DIR -ErrorAction Stop
	}

	###COPY FONTS AND INPUT PLUGINS TO BINDING DIR
    $srcDir="$env:MAPNIK_SDK\libs\mapnik"
    $destDir="$env:NODEMAPNIK_BINDING_DIR\mapnik"
	Write-Output "$msg_prefix copying fonts and input plugins to binding dir:"
	Write-Output "$srcDir --> $destDir"
    Copy-Item -Path $srcDir -Destination $destDir -Force -Recurse

    ##COPY GDAL AND PROJ TO BINDING DIR
    $srcDir="$env:MAPNIK_SDK\share"
    $destDir="$env:NODEMAPNIK_BINDING_DIR\share"
	Write-Output "$msg_prefix copying gdal and proj to binding dir:"
	Write-Output "$srcDir --> $destDir"
    Copy-Item -Path $srcDir -Destination $destDir -Force -Recurse

}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

$msg_prefix='====================== '

Try{

    ##dependencies
    $deps = @(
        "$env:MAPNIK_LIB_DIR\mapnik.dll",
        "$env:MAPNIK_LIB_DIR\icuuc48.dll",
        "$env:MAPNIK_LIB_DIR\icuin48.dll",
        "$env:MAPNIK_LIB_DIR\cairo.dll"
    )

    ##add libxml to deps, if not compiled static
    if($env:LIB_XML_STATIC -ne '1'){
        $deps += "$env:MAPNIK_LIB_DIR\libxml2.dll"
    }

	###COPY INPUT PLUGINS TO BINDING DIR
	Write-Output "$msg_prefix copying input plugins to binding dir: $env:N_MAPNIK_BINDING_DIR"
	#Copy-Item $env:MAPNIK_PLUGIN_DIR\*.* $env:N_MAPNIK_BINDING_DIR
    ###be more verbose
    ForEach ($src in $(Get-ChildItem $env:MAPNIK_PLUGIN_DIR)){
        Write-Output $src.FullName
        Copy-Item $src.FullName $env:N_MAPNIK_BINDING_DIR\
    }

	##COPY DEPENDENCIES TO BINDING DIR
	Write-Output "$msg_prefix copying dependencies to binding dir: $env:N_MAPNIK_BINDING_DIR"
	foreach($dep in $deps){
    	Write-Output $dep
        Copy-Item $dep $env:N_MAPNIK_BINDING_DIR
	}
}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

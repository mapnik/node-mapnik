$msg_prefix='====================== '

Try{

    Write-Output "$msg_prefix MAPNIK_VERSION: $env:MAPNIK_VERSION"
    $mapnik220 = False
    if($env:MAPNIK_VERSION -eq "2.2.0"){
        $mapnik220=$True
    }

    if($mapnik220){
        $mapnik_remote='http://mapnik.s3.amazonaws.com/dist/v2.2.0/mapnik-win-sdk-v2.2.0.zip'
    } else {
        $mapnik_remote='https://mapnik.s3.amazonaws.com/dist/dev/mapnik-win-sdk-v2.3.0-f59dc36a19.zip'
    }
    $mapnik_local="$env:DL_DIR\mapnik-sdk$env:MAPNIK_VERSION.zip"

    $cairohdrs_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/cairo-hdrs.7z'
    $cairohdrs_local="$env:DL_DIR\cairo-hdrs.7z"
    $cairohdrs_dir=$env:MAPNIK_DIR + '\include'

    $protobin_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/protobuf-bin.7z'
    $protobin_local="$env:DL_DIR\protobuf-bin.7z"
    $protobin_dir=$env:BASE_DIR

    $protosrc_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/protobuf-src.7z'
    $protosrc_local="$env:DL_DIR\protobuf-src.7z"

    ###CREATE DOWNLOAD DIR
    if(!(Test-Path -Path $env:DL_DIR )){
    	Write-Output "$msg_prefix creating download directory: $env:DL_DIR"
	    New-Item -ItemType directory -Path $env:DL_DIR
    } else {
    	Write-Output "$msg_prefix download directory exists already: $env:DL_DIR"
    }

	###DOWNLOAD DEPENDENCIES
	Write-Output "$msg_prefix downloading dependencies $msg_prefix"
	Write-Output "$msg_prefix downloading mapnik sdk: $mapnik_remote"
	(new-object net.webclient).DownloadFile($mapnik_remote, $mapnik_local)
    if($mapnik220){
    	Write-Output "$msg_prefix downloading cairohdrs: $cairohdrs_remote"
	    (new-object net.webclient).DownloadFile($cairohdrs_remote, $cairohdrs_local)
    }
	Write-Output "$msg_prefix downloading protoc-bin: $protobin_remote"
	(new-object net.webclient).DownloadFile($protobin_remote, $protobin_local)
	Write-Output "$msg_prefix downloading protobuf src: $protosrc_remote"
	(new-object net.webclient).DownloadFile($protosrc_remote, $protosrc_local)
	Write-Output "$msg_prefix dependencies downloaded $msg_prefix"

	###EXTRACT DEPENDENCIES
	##7z does not have a silent mode -> pipe to FIND to reduce ouput
	##| FIND /V "ing  "
	Write-Output "$msg_prefix extracting mapnik sdk: C:\"
	invoke-expression "7z -y x $mapnik_local -oC:\ | FIND /V `"ing  `""
    if($mapnik220){
	    Write-Output "$msg_prefix extracting cairo hdrs: $cairohdrs_dir"
	    invoke-expression "7z -y e $cairohdrs_local -o$cairohdrs_dir | FIND /V `"ing  `""
    }
	Write-Output "$msg_prefix extracting protoc.exe: $protobin_dir"
	invoke-expression "7z -y e $protobin_local -o$protobin_dir | FIND /V `"ing  `""
	Write-Output "$msg_prefix extracting protobuf src: $env:BASE_DIR"
	invoke-expression "7z -y x $protosrc_local -o$env:BASE_DIR | FIND /V `"ing  `""

	Write-Output "$msg_prefix deleting nodist dir: $env:NODIST_DIR"
    If (Test-Path($env:NODIST_DIR)){
        #there's a bug in powershell for this command to work
        #Remove-Item $env:NODIST_DIR -Recurse -Force -ErrorAction Stop
        Get-ChildItem -Path $env:NODIST_DIR -Recurse | Remove-Item -Recurse -Force
        Remove-Item $env:NODIST_DIR -Recurse -Force
    }
}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

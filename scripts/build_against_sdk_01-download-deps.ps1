$msg_prefix='====================== '

Try{
    $mapnik_remote='http://mapnik.s3.amazonaws.com/dist/v2.2.0/mapnik-win-sdk-v2.2.0.zip'
    $mapnik_local="$env:DL_DIR\mapnik-sdk2.2.0.zip"

    $cairohdrs_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/cairo-hdrs.7z'
    $cairohdrs_local="$env:DL_DIR\cairo-hdrs.7z"
    $cairohdrs_dir=$mapnik_dir + '\include'

    $protobin_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/protobuf-bin.7z'
    $protobin_local="$env:DL_DIR\protobuf-bin.7z"
    $protobin_dir=$env:BASE_DIR

    $protosrc_remote='https://raw.github.com/BergWerkGIS/node-mapnik-build-deps/master/protobuf-src.7z'
    $protosrc_local="$env:DL_DIR\protobuf-src.7z"

    ###CREATE DOWNLOAD DIR
    if(!(Test-Path -Path $env:DL_DIR )){
    	Write-Output "$msg_prefix creating download directory"
	    New-Item -ItemType directory -Path $env:DL_DIR
    } else {
    	Write-Output "$msg_prefix download directory exists already"
    }

	###DOWNLOAD DEPENDENCIES
	Write-Output "$msg_prefix downloading dependencies $msg_prefix"
	Write-Output "$msg_prefix downloading mapnik sdk"
	(new-object net.webclient).DownloadFile($mapnik_remote, $mapnik_local)
	Write-Output "$msg_prefix downloading cairohdrs"
	(new-object net.webclient).DownloadFile($cairohdrs_remote, $cairohdrs_local)
	Write-Output "$msg_prefix downloading protoc-bin"
	(new-object net.webclient).DownloadFile($protobin_remote, $protobin_local)
	Write-Output "$msg_prefix downloading protobuf src"
	(new-object net.webclient).DownloadFile($protosrc_remote, $protosrc_local)
	Write-Output "$msg_prefix dependencies downloaded $msg_prefix"

	###EXTRACT DEPENDENCIES
	##7z does not have a silent mode -> pipe to FIND to reduce ouput
	##| FIND /V "ing  "
	Write-Output "$msg_prefix extracting mapnik sdk"
	invoke-expression "7z -y x $mapnik_local -oC:\ | FIND /V `"ing  `""
	Write-Output "$msg_prefix extracting cairo hdrs"
	invoke-expression "7z -y e $cairohdrs_local -o$cairohdrs_dir | FIND /V `"ing  `""
	Write-Output "$msg_prefix extracting protoc.exe"
	invoke-expression "7z -y e $protobin_local -o$protobin_dir | FIND /V `"ing  `""
	Write-Output "$msg_prefix extracting protobuf src"
	invoke-expression "7z -y x $protosrc_local -o$env:BASE_DIR | FIND /V `"ing  `""

}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

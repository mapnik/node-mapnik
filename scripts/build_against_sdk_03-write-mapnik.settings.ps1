$msg_prefix='====================== '

Try{
    $file="$env:N_MAPNIK_BINDING_DIR\mapnik_settings.js"
"var path = require('path');
module.exports.paths = {
    'fonts': path.join(__dirname, 'mapnik/fonts'),
    'input_plugins': path.join(__dirname, 'mapnik/input')
};
module.exports.env = {
    'ICU_DATA': path.join(__dirname, 'share/icu'),
    'GDAL_DATA': path.join(__dirname, 'share/gdal'),
    'PROJ_LIB': path.join(__dirname, 'share/proj')
};" | Out-File -Encoding UTF8 $file
}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

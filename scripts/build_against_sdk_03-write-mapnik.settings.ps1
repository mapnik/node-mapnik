$msg_prefix='====================== '

Try{
    $file="$env:NODEMAPNIK_BINDING_DIR\mapnik_settings.js"
    Write-Output "writing settings to --> $file"
"var path = require('path');
module.exports.paths = {
    'fonts': path.join(__dirname, 'mapnik/fonts'),
    'input_plugins': path.join(__dirname, 'mapnik/input'),
    'mapnik_index': path.join(__dirname, 'mapnik/bin/mapnik-index'),
    'shape_index': path.join(__dirname, 'mapnik/bin/shapeindex')
};
module.exports.env = {
    'ICU_DATA': path.join(__dirname, 'share/icu'),
    'GDAL_DATA': path.join(__dirname, 'share/gdal'),
    'PROJ_LIB': path.join(__dirname, 'share/proj'),
    'PATH': __dirname
};" | Out-File -Encoding UTF8 $file
}
Catch {
	Write-Output "`n`n$msg_prefix`n!!!!EXCEPTION!!!`n$msg_prefix`n`n"
	Write-Output "$_.Exception.Message`n"
	Exit 1
}

## new mapnik.Image(width, height)

Create a new image object that can be rendered to.

## Image.width()

Returns the width of the image in pixels

## Image.height()

Returns the height of the image in pixels

## Image.encode(format, callback)

Encode an image into a given format, like `'png'` and call callback with `(err, result)`.

## Image.encodeSync(format)

Encode an image into a given format, like `'png'` and return buffer of data.

## Image.compare(image,options)

Available in >= 1.4.7.

Compare the pixels of one image to the pixels of another. Returns the number of pixels that are different. So, if the images are identical then it returns `0`. And if the images share no common pixels it returns the total number of pixels in an image which is equivalent to `im.width()*im.height()`.

Arguments:

* `image`: An image instance to compare to.

Options:

* `threshold`: A value that should be 0 or greater to determine if the pixels match. Defaults to 16 which means that `rgba(0,0,0,0)` would be considered the same as `rgba(15,15,15,0)`.

* `alpha`: Boolean that can be set to `false` so that alpha is ignored in the comparison. Default is `true` which means that alpha is considered in the pixel comparison along with the rgb channels.
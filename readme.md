# Jpak compression format

**Not stable or properly tested! Do not use to store any data you care about**

This is a compression format designed for:

 - similar compression ratio as tar + xz
 - random access of files
 - parallel packing and unpacking

Note that parallel operations are not yet implemented.

## Measurements

Article about results is [online here](http://nibblestew.blogspot.fi/2017/01/beating-compression-performance-of-xz.html).

Compressed file sizes for different archivers including some not
listed in the article above. Archives listed in decreasing order of size.

 - Zip 164 MB
 - 7zip 150 MB
 - jpa-10M 102 MB
 - jpa-10M 93 MB
 - tar.xz 92 MB
 - jpa-100M 91 MB
 - 7zip solid 91 MB
 - jpa-1000M 91 MB

The last three are very close to the same size, jpa-1000M is only
around 100 kB smaller than 7zip solid.

All archivers used LZMA compression with default settings.

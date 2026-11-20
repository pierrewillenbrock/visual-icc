# Visual ICC

This is a tool for generating ICC files using visual methods, like comparing differently patterned/colored swatches. 
**This is not a calibration tool and does not replace using a hardware measuring device if a true calibration is needed.**

This works to create a useable gamma setting (and pleasing white-balance at some time in the future), though.

Currently, this only allows to modify the transfer functions because those can be derived from
brightest-most and darkest-most components, mixed in various amounts and how a solid color compares to them.

I don't know about methods to figure out other color space properties, like the
primary points relative to each other, or the white point, but if there are
some, those can be added here as well.


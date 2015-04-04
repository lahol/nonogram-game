# Nonogram-game #

Play nonograms on your computer.

Nonogram-game supports a human-readable input-file format, so you can create
new puzzles by yourself using just a simple text editor.

Nonogram-game lets you tick off and lock squares using the left or
the right mouse button, respectively. Large puzzles (i.e. not fitting completely
in the window) may be scrolled. Hints also may be ticked off.

Scrolling is done with the mouse wheel or shift + mouse wheel for vertical or
horizontal direction.

## Installation ##

Just run
 
    $ make
    # make install

This will install the program into `/usr/bin`. If you want to install it in another
location just use

    $ make PREFIX=/your/app/dir install

## License ##

This program is licensed under a MIT license. See LICENSE.

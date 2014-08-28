good-robot
==========

Better solution to "toy robot" coding exercise.

Three-quarter-baked version in Ruby.

Testing
-------

    % ( good_robot.rb test_input1.txt 2>&1 ) > test_output.txt
    % diff test_output.txt test_output1_rb.txt

Note that input syntax and output messages are slightly different for C++ and Ruby versions.

Synopsis
--------

    % good_robot

or

    % good_robot <input-file> [ <input-file>, ... ]

Accepts commands (from stdin or named input files):

    table <xmin> <ymin> <xmax> <ymax>
    create <new-robot-name>
    [ <robot-name> ] place <x> <y> <direction>
    [ <robot-name> ] move
    [ <robot-name> ] left
    [ <robot-name> ] right
    [ <robot-name> ] report
    [ <robot-name> ] remove
    quit
    help

Commands are *case-insensitive*. Robot names are *case-sensitive*.

Arguments (for "table" and "place") can be comma- or space-delimited.

Starts with a table at [ ( 0, 0 ), ( 10, 10 ) ] but "table" resizes this.

Starts with two robots called "Robbie" and "Arthur", not on the table.

place/move/left/right/report/remove act on all robots or just the named one.

Robots cannot be moved past the table boundaries, nor onto an occupied position.

The table can however be resized on the fly so that a Robot can suddenly find
itself outside the boundaries. Please don't do this as it upsets the
Robot's world view :-)

Flow
----

Ruby version needs describing...

Classes
-------

Ruby version needs describing...

Extensibility/pluggability concerns
-----------------------------------
- different driver e.g. commands come from some MMO game
- 3D
- more complicated edge constraints (edges could be rows of no-go positions? representable as motionless pseudo-robots i.e. non-command-listening constraint-generating objects)
- more-than-one-square occupancy (that could tie in with the above edge objects)

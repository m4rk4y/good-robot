good-robot
==========

Better solution to "toy robot" coding exercise (still in C++ though)

Synopsis
--------
Accepts commands (from stdin or named input files):

    table <xmin> <ymin> <xmax> <ymax>
    create <new-robot-name>
    [ <robot-name>: ] place
    [ <robot-name>: ] move
    [ <robot-name>: ] left
    [ <robot-name>: ] right
    [ <robot-name>: ] report
    [ <robot-name>: ] remove
    quit
    help

Commands are case-insensitive. Robot names are case-sensitive.

Starts with a table at [ ( 0, 0 ), ( 10, 10 ) ] but "table" resizes this.

Starts with two robots called "Robbie" and "Arthur", not on the table.

place/move/left/right/report/remove act on all robots or just the named one.

Robots cannot be moved past the table boundaries, nor onto an occupied position.

The table can however be resized on the fly so that a Robot is outside its
boundaries. Please don't do this :-)

Flow
----
(1) main loop:

    while read-command
        if help
            help
        elsif quit
            quit
        elsif create
            create-robot
                register-as-command-listener
                register-as-constrainer
        else
            broadcast-command-to-all-listeners
                listeners.each
                    relay-command-to-listening-object
        endif
    endwhile

(2) command:

    constrainers.each
        check-this-proposal
    if ok
        do it
    endif

Classes
-------

CommandStream: reads files or stdin until EOF and produces command lines

Command: what a command line gets turned into

CommandFactory: constructs Commands

CommandListener: intermediary, constructed by GameObject in order to relay Commands to the GameObject

GameObject: interface for objects which wish to be notified of commands and/or constraint-verdict requests

Robot: implementation of GameObject, which responds to Commands while observing Constraints

RobotFactory: constructs Robots

Table: implementation of GameObject, which responds to (very few) Commands and provides a constraint-request verdict

Interpreter: main controlling object which
 - uses CommandStream to read lines
 - creates Commands
 - tells Broadcaster to broadcast the Commands

Broadcaster: broadcasts Commands to CommandListeners

Constraint: checks proposed moves etc; constructed by GameObject in order to relay constraint-verdict requests to the GameObject

ConstraintFactory: constructs Constraints

Various Exception classes.

Extensibility/pluggability concerns
-----------------------------------
- different driver e.g. commands come from some MMO game
- 3D
- more complicated edge constraints (edges could be rows of no-go positions? representable as motionless pseudo-robots i.e. non-command-listening constraint-generating objects)
- more-than-one-square occupancy (that could tie in with the above edge objects)

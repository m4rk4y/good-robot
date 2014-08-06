good-robot
==========

Better solution to "toy robot" coding exercise (still in C++ though)

Proposal
--------

Classes:
    Reader
    Command
    CommandListener
    Constraint
    ConstraintListener

(perhaps CommandListener and ConstraintListener can have a common superclass
Listener?)

Reader reads command endlessly until EOF
Sends each command to every registered command-listener
Each command-listener (robot) responds to command appropriately, observing constraints
Constraint requests are broadcast to constraint-listeners

Set of commands map from each instruction (simple solution sets up hard-coded
map; better would be to read from setup file so can e.g. do multiple
languages)

Considerations...

Each command says what it would like to do: how best to encapsulate message
between command and robot, how to direct response, how to design set-of-
commands so that robot doesn't need to know what class of command it is, just
what the command is requesting

Different commands: class hierarchy? Could be best for extensibility but a bit
overkill for simple example.

Multiple robots need to avoid collisions. These and the table-edge are
examples of constraints. Need a uniform interface which takes Proposal and
returns verdict.

How to avoid inter-robot false positive? E.g. if robot1 is on (1,1) and wants
to move to (2,1) while robot2 is on (2,1) and wants to move to (3,1), then if
they are supposed to move simultaneously then this should be ok, but if we try
to move robot1 first then it will fail. Depends on form of instructions: if
each instruction is separate and tagged for a specific robot, then we can
reasonably process them one at a time, but if say we extend to one instruction
encapsulating multiple robot-moves then it's a harder problem.

Extensibility/pluggability concerns:
 - different driver e.g. commands come from some MMO game
 - 3D
 - more complicated edge constraints (edges could be rows of no-go positions? representable as motionless pseudo-robots i.e. non-command-listening constraint-generating objects)
 - more-than-one-square occupancy (that could tie in with the above edge objects)

Simple version:
  - hard-code list of commands
  - no command class hierarchy
  - robots talk directly to one another in order to avoid collision
  - hard-code edges of table

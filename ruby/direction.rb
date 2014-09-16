module Direction
    INVALID = -1
    NORTH = 0
    EAST = 1
    SOUTH = 2
    WEST = 3
    def Direction.to_s ( arg )
        { INVALID => :Invalid,
          NORTH   => :North,
          EAST    => :East,
          SOUTH   => :South,
          WEST    => :West
        }[arg]
    end
    # We want to have a way to raise an exception for invalid values.
    # Easiest with case rather than this hash, which simply returns nil.
    def Direction.from_s ( arg )
        { "N" => NORTH,
          "E" => EAST,
          "S" => SOUTH,
          "W" => WEST
        }[arg]
    end
    def Direction.left ( arg )
        { INVALID => INVALID,
          NORTH   => WEST,
          EAST    => NORTH,
          SOUTH   => EAST,
          WEST    => SOUTH
        }[arg]
    end
    def Direction.right ( arg )
        { INVALID => INVALID,
          NORTH   => EAST,
          EAST    => SOUTH,
          SOUTH   => WEST,
          WEST    => NORTH
        }[arg]
    end
    def Direction.moveX ( arg )
        { INVALID => 0,
          NORTH   => 0,
          EAST    => 1,
          SOUTH   => 0,
          WEST    => -1
        }[arg]
    end
    def Direction.moveY ( arg )
        { INVALID => 0,
          NORTH   => 1,
          EAST    => 0,
          SOUTH   => -1,
          WEST    => 0
        }[arg]
    end
    def Direction.move ( direction, xpos, ypos )
        # Now we do this with two { Direction, increment } hashes.
        case direction
        when Direction::NORTH
            ypos += 1
        when Direction::EAST
            xpos += 1
        when Direction::SOUTH
            ypos -= 1
        when Direction::WEST
            xpos -= 1
        end
    [ xpos, ypos ]
    end
end

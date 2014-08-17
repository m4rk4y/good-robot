module Direction
    INVALID = -1
    NORTH = 0
    EAST = 1
    SOUTH = 2
    WEST = 3
    # want to have a way to raise an exception for invalid values
    # Easiest with case rather than this hash
    def Direction.to_s ( arg )
        { INVALID => "Invalid",
          NORTH   => "North",
          EAST    => "East",
          SOUTH   => "South",
          WEST    => "West"
        }[arg]
    end
    def Direction.from_s ( arg )
        { "N" => NORTH,
          "E" => EAST,
          "S" => SOUTH,
          "W" => WEST
        }[arg]
    end
end

module Constraint
    @@constrainers = nil
    def addConstrainer ( object, function )
        @@constrainers = {} unless @@constrainers
        @@constrainers[object] = function
    end
    def checkConstraints( *args )
        # returns true if all the constrainers say ok
        # should filter out self at this point though
        @@constrainers.all?{ | key, value | key.send( value, self, args ) }
    end
end

class Game
    attr_accessor :forwardedCommands, :specificCommands, :table, :robots
    def initialize
        # can't help thinking this is too early
        @forwardedCommands = []
        @specificCommands = []
        @table = nil
        @robots = {}
    end

    def interpret( line )
        # Line can be either <verb> [ <args> ]
        # or <known-robot> <verb> [ <args> ]
        #
        # <verb> must be a known command and possibly one that goes via the
        # generic forwarding.
        #
        # This isn't using a Broadcaster/Listener mechanism yet.
        #
        verb, rest = line.split( /[ :]+/, 2 )
        if robots.keys.include?( verb )
            robot = robots[verb]
            verb, rest = rest.split( /[ ]+/, 2 )
        end
        qualifiers = rest.split( /[ ]/ ) if rest
        if forwardedCommands.include?( verb )
            doSomething( func:verb, robot:robot, args:qualifiers )
        elsif ( specificCommands.include?( verb ) && respond_to?( verb ) )
            send verb, robot:robot, args:qualifiers
        else
            puts "Don't know how to #{verb}"
        end
    end

    def create( robot: nil, **other_args )
        other_args[:args].each { |name| robots[name] = Robot.new( name ) }
    end

    def table( **other_args )
        @table.set( *other_args[:args] )
    end

    def doSomething( func: nil, robot: nil, **other_args )
        if robot
            robot.send( func, *other_args[:args] )
        else
            robots.values.each { |robot| robot.send( func, *other_args[:args] ) }
        end
    end

    def report( **other_args )
        @table.report
        robots.values.each { |robot| robot.report }
    end

    def help( **other_args )
        puts "Valid commands are:"
        # Yeah I could concatenate the arrays and do one join, but what's the point?
        puts forwardedCommands.join "\n"
        puts specificCommands.join "\n"
    end

    def quit( **other_args )
        puts "Bye!"
        exit
    end
end

class Table
    include Constraint
    attr_reader :xmin, :ymin, :xmax, :ymax
    def initialize( xmin, ymin, xmax, ymax )
        set( xmin, ymin, xmax, ymax )
        addConstrainer( self, :constrain )
    end
    def constrain( *args )
        actor = args[0]
        new_xpos = args[1][0].to_i
        new_ypos = args[1][1].to_i
        ( self === actor ||
          # can I say a <= b < c?
          ( @xmin <= new_xpos && new_xpos < @xmax && @ymin <= new_ypos && new_ypos < @ymax )
        )
    end
    def set ( xmin, ymin, xmax, ymax )
        # Harsh... nothing catches this yet.
        if xmin >= xmax || ymin >= ymax
            raise ArgumentError,
                "Invalid table co-ordinates [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
        end
        @xmin = xmin
        @ymin = ymin
        @xmax = xmax
        @ymax = ymax
    end
    def report
        puts "Table is at [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
    end
end

class Robot
    include Constraint
    attr_reader :name, :xpos, :ypos, :direction, :on_table
    def initialize( name )
        @name = name
        @xpos = -1
        @ypos = -1
        @direction = Direction::INVALID
        @on_table = false
        addConstrainer( self, :constrain )
    end
    def constrain( *args )
        actor = args[0]
        new_xpos = args[1][0].to_i
        new_ypos = args[1][1].to_i
        ( self === actor || ! @on_table || @xpos != new_xpos || @ypos != new_ypos )
    end
    def place( new_xpos, new_ypos, direction )
        if checkConstraints( new_xpos, new_ypos )
            @xpos = new_xpos.to_i
            @ypos = new_ypos.to_i
            @direction = Direction.from_s direction.upcase
            @on_table = true
        else
            puts "Cannot place Robot #{name} at ( #{new_xpos}, #{new_ypos} )"
        end
    end
    def move
        if ! @on_table
            puts "Cannot move Robot #{name} when not on table"
            return
        end
        new_xpos = @xpos
        new_ypos = @ypos
        case @direction
        when Direction::NORTH
            new_ypos = new_ypos + 1
        when Direction::EAST
            new_xpos = new_xpos + 1
        when Direction::SOUTH
            new_ypos = new_ypos - 1
        when Direction::WEST
            new_xpos = new_xpos - 1
        end
        if checkConstraints( new_xpos, new_ypos )
            @xpos = new_xpos
            @ypos = new_ypos
        else
            puts "Cannot move Robot #{name} to ( #{new_xpos}, #{new_ypos} )"
        end
    end
    def left
        @direction = case @direction
                     when Direction::NORTH
                         Direction::WEST
                     when Direction::EAST
                         Direction::NORTH
                     when Direction::SOUTH
                         Direction::EAST
                     when Direction::WEST
                         Direction::SOUTH
                     else # leave Invalid unchanged
                     end
    end
    def right
        @direction = case @direction
                     when Direction::NORTH
                         Direction::EAST
                     when Direction::EAST
                         Direction::SOUTH
                     when Direction::SOUTH
                         Direction::WEST
                     when Direction::WEST
                         Direction::NORTH
                     else # leave Invalid unchanged
                     end
    end
    def remove
        @on_table = false
    end
    def report
        if @on_table
            puts "#{name} is at ( #{xpos}, #{ypos} ) facing #{Direction.to_s direction}"
        else
            puts "#{name} is not on the table"
        end
    end
end

# Start here.

game = Game.new
# I really don't like this split...
game.specificCommands = [ "create", "table", "report", "help", "quit" ]
game.forwardedCommands = [ "place", "move", "left", "right", "remove" ]
game.table = Table.new( 0, 0, 10, 10 )
[ "Robbie", "Arthur" ].each { |name| game.robots[name] = Robot.new( name ) }
# Ugly, but I want STDIN to be interactive.
if ARGV.empty?
    game.help
    while true
        printf "? "
        line = gets.chomp
        game.interpret ( line ) unless line.empty?
    end
else
    ARGF.readlines.each { |line| interpret line }
end
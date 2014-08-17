module Direction
    INVALID = -1
    NORTH = 0
    EAST = 1
    SOUTH = 2
    WEST = 3
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
        @@constrainers.all?{ | key, value | key.send( value, args ) }
    end
end

class Game
    attr_accessor :validCommands, :forwarded, :table, :robots
    def initialize
        # can't help thinking this is too early
        @validCommands = []
        @forwarded = []
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
        verb, rest = line.split( /[ :]+/, 2 )
        if robots.keys.include?( verb )
            robot = robots[verb]
            verb, rest = rest.split( /[ ]+/, 2 )
        end
        if validCommands.include?( verb )
            qualifiers = rest.split( /[ ]/ ) if rest
            if forwarded.include?( verb )
                doSomething( func:verb, robot:robot, args:qualifiers )
            elsif respond_to?( verb )
                send verb, robot:robot, args:qualifiers
            else
                puts "Don't know how to #{verb}"    # twice? Surely not
            end
        else
            puts "Don't know how to #{verb}"        # twice? Surely not
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

#     def place( robot: nil, **other_args )
#         if robot
#             robot.place( *other_args[:args] )
#         else
#             robots.values.each { |robot| robot.place( *other_args[:args] ) }
#         end
#     end
#
#     def move( robot: nil, **other_args )
#         if robot
#             robot.move( *other_args[:args] )
#         else
#             robots.values.each { |robot| robot.move( *other_args[:args] ) }
#         end
#     end
#
#     def left( robot: nil, **other_args )
#         if robot
#             robot.left( *other_args[:args] )
#         else
#             robots.values.each { |robot| robot.left( *other_args[:args] ) }
#         end
#     end
#
#     def right( robot: nil, **other_args )
#         if robot
#             robot.right( *other_args[:args] )
#         else
#             robots.values.each { |robot| robot.right( *other_args[:args] ) }
#         end
#     end
#
#     def remove( robot: nil, **other_args )
#         if robot
#             robot.remove( *other_args[:args] )
#         else
#             robots.values.each { |robot| robot.remove( *other_args[:args] ) }
#         end
#     end

    def report( **other_args )
        @table.report
        robots.values.each { |robot| robot.report }
    end

    def help( **other_args )
        puts "Valid commands are:"
        puts validCommands.join "\n"
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
        puts "create a table"
        set( xmin, ymin, xmax, ymax )
        addConstrainer( self, :constrain )
    end
    def constrain( args )
        puts "Checking #{args} for table"
        true
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
        puts "create a robot called #{name}"
        @name = name
        @xpos = -1
        @ypos = -1
        @direction = Direction::INVALID
        @on_table = false
        addConstrainer( self, :constrain )
    end
    def constrain( args )
        puts "Checking #{args} for Robot #{name}"
        true
    end
    def place( xpos, ypos, direction )
        # Ensure not off table, ensure not colliding with another robot
        @xpos = xpos
        @ypos = ypos
        @direction = Direction.from_s direction.upcase
        @on_table = true
    end
    def move
        puts "dummy move"
        # Ensure not off table, ensure not colliding with another robot
        # checker should not care if checking against self
        if checkConstraints( 42, 43 )
            puts "check ok"
        else
            puts "Check failed"
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
                     else # ought not to change Invalid, but for now...
                         Direction::NORTH
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
                     else # ought not to change Invalid, but for now...
                         Direction::NORTH
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
game.validCommands = [ "create", "table", "place", "move", "left", "right", "report", "remove", "help", "quit" ]
# Horrible, these are the generic doSomething ones...
game.forwarded = [ "place", "move", "left", "right", "remove" ]
game.table = Table.new( 0, 0, 10, 10 )
[ "Robbie", "Arthur" ].each { |name| game.robots[name] = Robot.new( name ) }
# Ugly, but I want STDIN to be interactive.
if ARGV.empty?
    game.help
    while line = gets
        game.interpret ( line.chomp )
    end
else
    ARGF.readlines.each { |line| interpret line }
end
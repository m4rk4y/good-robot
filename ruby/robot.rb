require_relative "constraint.rb"
require_relative "direction.rb"

class Robot
    include Constraint
    attr_reader :name, :xpos, :ypos, :direction, :on_table
    def initialize( name )
        @name = name
        @xpos, @ypos = -1, -1
        @direction = Direction::INVALID
        @on_table = false
        addConstrainer( self, :constrain )
        report
    end
    def constrain( *args )
        # Named arguments would help here.
        actor = args[0]
        new_xpos, new_ypos = args[1][0].to_i, args[1][1].to_i
        ! @on_table || @xpos != new_xpos || @ypos != new_ypos
    end
    def place( new_xpos, new_ypos, direction )
        new_direction = Direction.from_s direction.upcase
        if new_direction
            if checkConstraints( new_xpos, new_ypos )
                @xpos, @ypos = new_xpos.to_i, new_ypos.to_i
                @direction = Direction.from_s direction.upcase
                @on_table = true
                report
            else
                "Cannot place Robot #{name} at ( #{new_xpos}, #{new_ypos} )"
            end
        else
            "Invalid direction #{direction}"
        end
    end
    def move
        if @on_table
            new_xpos = @xpos + Direction.moveX( @direction )
            new_ypos = @ypos + Direction.moveY( @direction )
            if checkConstraints( new_xpos, new_ypos )
                @xpos, @ypos = new_xpos, new_ypos
                report
            else
                "Cannot move Robot #{name} to ( #{new_xpos}, #{new_ypos} )"
            end
        else
            "Cannot move Robot #{name} when not on table"
        end
    end
    def left
        if @on_table
            @direction = Direction.left ( @direction )
            report
        else
            "Cannot turn Robot #{name} left when not on table"
        end
    end
    def right
        if @on_table
            @direction = Direction.right ( @direction )
            report
        else
            "Cannot turn Robot #{name} right when not on table"
        end
    end
    def remove
        @on_table = false
        report
    end
    def report
        if @on_table
            "#{name} is at ( #{xpos}, #{ypos} ) facing #{Direction.to_s direction}"
        else
            "#{name} is not on the table"
        end
    end
end

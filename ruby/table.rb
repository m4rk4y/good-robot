require_relative "constraint.rb"

class Table
    include Constraint
    attr_reader :xmin, :ymin, :xmax, :ymax
    def initialize( xmin, ymin, xmax, ymax )
        set( xmin, ymin, xmax, ymax )
        addConstrainer( self, :constrain )
    end
    def constrain( *args )
        actor = args[0]
        new_xpos, new_ypos = args[1][0].to_i, args[1][1].to_i
        ( @xmin ... @xmax ).include?( new_xpos ) && ( @ymin ... @ymax ).include?( new_ypos )
    end
    def set ( xmin, ymin, xmax, ymax )
        xmin, xmax, ymin, ymax = xmin.to_i, xmax.to_i, ymin.to_i, ymax.to_i
        if ( xmin < xmax && ymin < ymax )
            @xmin, @ymin, @xmax, @ymax = xmin, ymin, xmax, ymax
            report
        else
            "Invalid table co-ordinates [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
        end
    end
    def report
        "Table is at [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
    end
end

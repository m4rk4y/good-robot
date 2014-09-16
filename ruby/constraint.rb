module Constraint
    @@constrainers = nil
    def addConstrainer ( object, function )
        @@constrainers ||= {}
        @@constrainers[object] = function
    end
    def checkConstraints( *args )
        # Returns true if all the constrainers say ok.
        # We filter out self at this point.
        @@constrainers.all?{ | key, value | key == self || key.send( value, self, args ) }
    end
end

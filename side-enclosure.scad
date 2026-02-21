// Parameters
length = 244;      
width = 68;
height = 79;
wall = 2;       // Wall thickness
fillet_radius = 7;
ribbon_height = 10;
pass_through_leg_width = 20;

// Shape with cable pass through
difference() {

    // Hollow shape, but missing cable pass through
    difference() {
        // Non-hollow shape
        hull() {   
            // Far end area 
            translate([0, length-wall, 0]) cube([width, wall, height]);

            // Base area
            cube([width, length, 1]); 

            // Nice fillet edge at front
            translate([0, fillet_radius, height-fillet_radius]) {
                rotate([0, 90, 0]) 
                cylinder(width, fillet_radius, fillet_radius);
            }
        }

        translate([wall, wall*2, -0.01])
        cube([width-wall*2, length, height-wall]);
    }

    // Ribbon cable pass through 
    translate([-wall, pass_through_leg_width, -0.01])
    cube([wall*3, length-pass_through_leg_width*2, ribbon_height]);

}
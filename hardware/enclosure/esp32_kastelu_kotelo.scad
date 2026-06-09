// ESP32 kastelujarjestelman PCB-kotelo
// PCB koko: 105 x 70 mm

$fn = 48;

pcb_x = 105;
pcb_y = 70;

clearance = 3;
wall = 2.4;
bottom = 2.4;
lid_thickness = 2.4;

inner_x = pcb_x + clearance * 2;
inner_y = pcb_y + clearance * 2;
inner_z = 35;

outer_x = inner_x + wall * 2;
outer_y = inner_y + wall * 2;
outer_z = inner_z + bottom;

usb_cut_w = 16;
usb_cut_h = 10;
side_slot_w = 34;
side_slot_h = 12;

part = "both";

module rounded_box(size=[10,10,10], r=2) {
    hull() {
        for (x=[r, size[0]-r])
        for (y=[r, size[1]-r])
        for (z=[r, size[2]-r])
            translate([x,y,z]) sphere(r=r);
    }
}

module base_shell() {

    difference() {

        // Ulkokuori
        hull() {

            // Pohjan pyöristetyt kulmat
            translate([2.5, 2.5, 2.5])
                sphere(r=2.5);

            translate([outer_x-2.5, 2.5, 2.5])
                sphere(r=2.5);

            translate([2.5, outer_y-2.5, 2.5])
                sphere(r=2.5);

            translate([outer_x-2.5, outer_y-2.5, 2.5])
                sphere(r=2.5);

            // Yläosa suorana
            translate([0,0,outer_z-0.1])
                cube([outer_x, outer_y, 0.1]);
        }

        // Sisäontelo
        translate([wall, wall, bottom])
            cube([inner_x, inner_y, inner_z + 1]);

        // USB-aukko
        translate([outer_x-wall-0.1, outer_y/2 - usb_cut_w/2, bottom + 8])
            cube([wall+0.3, usb_cut_w, usb_cut_h]);

        // Sivuaukko
        translate([-0.1, outer_y/2 - side_slot_w/2, bottom + 8])
            cube([wall+0.3, side_slot_w, side_slot_h]);

        // Johtoaukko
        translate([outer_x/2 - 11, outer_y-wall-0.1, bottom + 9])
            cube([22, wall+0.3, 10]);
    }
}

module base() {
    base_shell();
}

module lid() {

    union() {

        // Kannen peruslevy
        cube([outer_x, outer_y, lid_thickness]);

        // Kohoteksti
        translate([outer_x/2, outer_y/2 + 8, lid_thickness - 0.1])
            linear_extrude(height = 0.9)
                text("Kasteluterminaattori",
                     size = 7,
                     halign = "center",
                     valign = "center",
                     font = "Liberation Sans");

        // 3000-teksti
        translate([outer_x/2, outer_y/2 - 8, lid_thickness - 0.1])
            linear_extrude(height = 0.9)
                text("3000",
                     size = 12,
                     halign = "center",
                     valign = "center",
                     font = "Liberation Sans");

        // Sisähuuli
        translate([wall+1.0, wall+1.0, -2.5])
    difference() {

        cube([inner_x-2.0, inner_y-2.0, 2.7]);

        translate([2.0,2.0,-0.1])
        cube([inner_x-6.0, inner_y-6.0, 2.9]);
}
    }
}

if (part == "base") {
    base();
} else if (part == "lid") {
    lid();
} else {
    base();
    translate([outer_x + 10, 0, 0])
        lid();
}

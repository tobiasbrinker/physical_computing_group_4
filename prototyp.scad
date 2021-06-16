    difference() {
    //Hauptzylinder
    cylinder(h=10, r=20, center=true);
    // Ausbuchtung Waage Element
    translate([0,0,4.9])cylinder(h=3, r=13, center=true);
    // Mikroloch
    translate([16.5,4,5])cylinder(h=5, r=0.5, center=true);
    // Lautsprecher 1 Loch
    translate([0,16.5,5])cylinder(h=5, r=1, center=true);
    // Lautsprecher 2 Loch
    translate([0,-16.5,5])cylinder(h=5, r=1, center=true);
    // Ausbuchtung USB
    translate([-16.5,0,-2])cube([8,6,2], center=true);
    // Innen hohl
    cylinder(h=7, r=18, center=true);
    // Herzfrequenzmesser
    translate([13,10,5])cylinder(h=1, r=2, center=true);
}

// Arduino
translate([-5,0,-3])cube([7,5.4,1], center=true);

// Knopf
translate([16.5,0,5])cylinder(h=1, r=2, center=true);

//Diode Knopf
color("red")translate([14,0,4.5]) sphere(1);
color("red")translate([14.5,2,4.5]) sphere(1);
color("red")translate([14.5,-2,4.5]) sphere(1);

//Diode Heartbeat
color("blue")translate([10,10,4.5]) sphere(1);
color("blue")translate([11,12,4.5]) sphere(1);
color("blue")translate([13,13,4.5]) sphere(1);

//Diode Waage
color("green")translate([0,14.5,4.5]) sphere(1);
color("green")translate([0,-14.5,4.5]) sphere(1);




// "Waage" oberes Element
translate([10,30,15])cylinder(h=1, r=13);
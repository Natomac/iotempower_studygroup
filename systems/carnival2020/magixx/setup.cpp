//led(blue, ONBOARDLED).invert().on();

button(lower, D0, "pressed", "released").debounce(4).invert();
button(left, D3, "pressed", "released").debounce(4).invert();
button(right, D6, "pressed", "released").debounce(4).invert();

//led(red, D7);
//led(yellow, D8);


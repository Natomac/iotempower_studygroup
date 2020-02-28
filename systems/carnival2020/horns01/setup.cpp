const int strip_len = 10;

rgb_strip(strip1, strip_len, WS2812B, D1, GRB).report_change(false);
rgb_strip(strip2, strip_len, WS2812B, D2, GRB).report_change(false);

const int frame_count_base = 100;

// vars for tie
enum anim_type {anim_none, anim_dots, anim_rainbow,
            anim_scroll, anim_fade_out, anim_lightning,
            anim_pump_in, anim_pump_out, anim_meter};
anim_type anim_current = anim_none;
int anim_frames = 0;
uint32_t idle_counter = 0;

CRGB color_front, color_back, color_blended;
CHSV color_hsv_front, color_hsv_back;

int pump_threshold;

// connect strips into 2x10 matrix
rgb_matrix(matrix, 2, strip_len)
   .with(IN(strip1),0,0,Down_Right)
   .with(IN(strip2),1,0,Down_Right);

animator(anim, IN(matrix))
    .with_fps(50)
    .with_frame_builder( [&] {
        switch(anim_current) {
            int virtual_count;
            int virtual_pos;
            int x, y;
            int l;
			case anim_dots:
                virtual_count = 6 + 2 * strip_len;
                virtual_pos = ((frame_count_base - anim_frames) 
                                    * virtual_count + frame_count_base/2)/ frame_count_base;
                x=0; y=strip_len-1;
                for(int lednum=0; lednum < 2*strip_len; lednum++) { // go through all leds
                    int currentpos = lednum + 3;
                    if(currentpos == virtual_pos) IN(matrix).set_pixel(x, y, color_blended);
                    else if(currentpos == virtual_pos+1) IN(matrix).set_pixel(x, y, color_front);
                    else if(currentpos == virtual_pos+2) IN(matrix).set_pixel(x, y, color_blended);
                    else IN(matrix).set_pixel(x, y, color_back);
                    if(lednum < strip_len-1) y--;
                    else if(lednum >= strip_len) y++;
                    else x++; // leave y be equal
                }
				break;
			case anim_rainbow:
                IN(matrix).rainbow_column(0, 0, -1, -1, anim_frames % 256, 16); // TODO: think, revert anim_frames?
				break;
			case anim_scroll:
				IN(matrix).scroll_down(true, 0, 0, -1, -1);
				break;
            case anim_fade_out:
				IN(matrix).fade(8);
                break;
            case anim_lightning:
                if(anim_frames > frame_count_base / 10) {
                    l = (frame_count_base - anim_frames) * 8 * strip_len / frame_count_base; 
                    if(l > strip_len) l = strip_len;
                    for(int i=0; i<l; i++) {
                        int r = urandom(0,2);
                        IN(matrix).set_pixel(r,i,CRGB::White);
                        IN(matrix).set_pixel(1-r,i,CRGB::Black);
                    }
                } else {
                    anim_current = anim_fade_out;
                    IN(matrix).black();
                }
                break;
            case anim_meter:
                    l = ((frame_count_base - anim_frames) + strip_len - 1) / strip_len; 
                    if(l > strip_len) l = strip_len;
                    for(int i=0; i<strip_len; i++) {
                        if(l>i){
                            IN(matrix).set_pixel(0,i,CRGB::Red);
                            IN(matrix).set_pixel(1,i,CRGB::Red);
                        }
                        
                        else {
                            IN(matrix).set_pixel(0,i,CRGB::White);
                            IN(matrix).set_pixel(1,i,CRGB::White);
                    }
                }
                break;
            case anim_pump_in:
                    if(anim_frames < pump_threshold) {
                        anim_current = anim_pump_out;
                        anim_frames = frame_count_base;
                    } else {
                        ulog("pump: %f", ((double)anim_frames-pump_threshold)/(frame_count_base-pump_threshold)); // TODO: remove debug
                        color_blended = blend(color_hsv_front, color_hsv_back,
                            (anim_frames-pump_threshold)*255/(frame_count_base-pump_threshold));
                        IN(matrix).set_color(color_blended);
                    }
                break;
            case anim_pump_out:
                    if(anim_frames < pump_threshold) {
                        anim_current = anim_pump_in;
                        anim_frames = frame_count_base;
                    } else {
                        color_blended = blend(color_hsv_back, color_hsv_front,
                            (anim_frames-pump_threshold)*255/(frame_count_base-pump_threshold));
                        IN(matrix).set_color(color_blended);
                    }
                break;
		}
        if(anim_frames > 0)
            anim_frames --;
        else { // anim_frames == 0
            if(anim_current != anim_fade_out && anim_current != anim_none) {
                anim_frames = frame_count_base;
                anim_current = anim_fade_out;
            } else anim_current = anim_none;
        }
        if(anim_current == anim_none) {
            idle_counter ++;
        } else {
            idle_counter = 0;
        }
    } )
    .with_show( [&] { 
        if(anim_current != anim_none)
            IN(matrix).show(); 
    } )
    .with_command_handler( "dots", [&] (Ustring& command) {
        CRGB c;
        color_front = CRGB::Gold;
        color_back = CRGB::Black;
        if(IN(matrix).read_color(command, c)) {
            // front color given
            color_front = c;
            command.strip_param();
            if(IN(matrix).read_color(command, c)) {
                // back color given
                color_back = c;
            }
        }
        // compute blended color
        CHSV blended, front, back;
        front = rgb2hsv_approximate(color_front);
        back = rgb2hsv_approximate(color_back);
        blended = blend(front, back, 160);
        color_blended = blended; // auto convert from hsv to rgb

        anim_current = anim_dots;
        anim_frames = frame_count_base;
    } )
    .with_command_handler( "rainbow", [&] (Ustring& command) {
        anim_current = anim_rainbow;
        anim_frames = 10*frame_count_base;
    } )
    .with_command_handler( "scroll", [&] (Ustring& command) {
        anim_current = anim_scroll;
        anim_frames = frame_count_base;
    } )
    .with_command_handler( "fade_out", [&] (Ustring& command) {
        anim_current = anim_fade_out;
        anim_frames = frame_count_base;
    } )
    .with_command_handler( "lightning", [&] (Ustring& command) {
        anim_current = anim_lightning;
        anim_frames = frame_count_base;
    } )
    .with_command_handler( "meter", [&] (Ustring& command) {
        anim_current = anim_meter;
        anim_frames = frame_count_base;
    } )
    .with_command_handler( "pump", [&] (Ustring& command) {
        anim_current = anim_pump_in;
        anim_frames = frame_count_base;
        pump_threshold = 0;
        if(command.length()>0) pump_threshold = command.as_int();
        if(pump_threshold<1) pump_threshold = 1;
        if(pump_threshold>+frame_count_base) pump_threshold=frame_count_base-1;
        color_front = CRGB::Gold;
        color_back = CRGB::Black;
        command.strip_param();
        CHSV front, back;
        CRGB c;
        color_front = CRGB::Gold;
        color_back = CRGB::Black;
        if(IN(matrix).read_color(command, c)) {
            // front color given
            color_front = c;
            command.strip_param();
            if(IN(matrix).read_color(command, c)) {
                // back color given
                color_back = c;
            }
        }
        color_hsv_front = rgb2hsv_approximate(color_front);
        color_hsv_back = rgb2hsv_approximate(color_back);
    } );


void check_idle() {
    if(idle_counter > IN(anim).get_fps() * 15) {
        anim_current = anim_rainbow;
        anim_frames = 10*frame_count_base;
    }
    do_later( 10000, check_idle );
}

void start() {
    check_idle();
}

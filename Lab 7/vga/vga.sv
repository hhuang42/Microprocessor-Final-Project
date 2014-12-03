// vga.sv
// 20 October 2011 Karl_Wang & David_Harris@hmc.edu
// Modified 31 October 2014 Henry_Huang@hmc.edu
// VGA driver with character generator

module vga(input  logic       clk,
           input  logic       spi_clk,
           input  logic       spi_in,
           input  logic       spi_fsync,
           output logic       vgaclk,						// 25 MHz VGA clock
           output logic       hsync, vsync, sync_b,	// to monitor & DAC
           output logic [7:0] r, g, b);					// to video DAC
 
  logic [9:0]  x, y;
  logic [7:0]  r_int, g_int, b_int;
  logic [31:0] spi_word;
  logic [9:0]  x_cursor;
  logic [9:0]  y_cursor;
  logic        finished;
  logic [2:0]  buttons;
	
  // Use a PLL to create the 25.175 MHz VGA pixel clock 
  // 25.175 Mhz clk period = 39.772 ns
  // Screen is 800 clocks wide by 525 tall, but only 640 x 480 used for display
  // HSync = 1/(39.772 ns * 800) = 31.470 KHz
  // Vsync = 31.474 KHz / 525 = 59.94 Hz (~60 Hz refresh rate)
  pll	vgapll(.inclk0(clk),	.c0(vgaclk)); 

  // generate monitor timing signals
  vgaController vgaCont(vgaclk, hsync, vsync, sync_b,  
                        r_int, g_int, b_int, r, g, b, x, y);
	

  
  spi_frm_slave slave(spi_clk, spi_in, spi_fsync, spi_word, finished);
  
  mouse_reader reader(vsync, spi_word, finished, x_cursor, y_cursor, buttons);
  
  // user-defined module to determine pixel color
  videoGen videoGen(vsync, x, y, x_cursor, y_cursor, 
                    buttons, r_int, g_int, b_int);
endmodule

module vgaController #(parameter HMAX   = 10'd800,
                                 VMAX   = 10'd525, 
											HSTART = 10'd152,
											WIDTH  = 10'd640,
											VSTART = 10'd37,
											HEIGHT = 10'd480)
						  (input  logic       vgaclk, 
               output logic       hsync, vsync, sync_b,
							 input  logic [7:0] r_int, g_int, b_int,
							 output logic [7:0] r, g, b,
							 output logic [9:0] x, y);

  logic [9:0] hcnt, vcnt;
  logic       oldhsync;
  logic       valid;
  
  // counters for horizontal and vertical positions
  always @(posedge vgaclk) begin
    if (hcnt >= HMAX) hcnt = 0;
    else hcnt++;
	 if (hsync & ~oldhsync) begin // start of hsync; advance to next row
	   if (vcnt >= VMAX) vcnt = 0;
      else vcnt++;
    end
    oldhsync = hsync;
  end
  
  // compute sync signals (active low)
  assign hsync = ~(hcnt >= 10'd8 & hcnt < 10'd104); // horizontal sync
  assign vsync = ~(vcnt >= 2 & vcnt < 4); // vertical sync
  assign sync_b = hsync | vsync;

  // determine x and y positions
  assign x = hcnt - HSTART;
  assign y = vcnt - VSTART;
  
  // force outputs to black when outside the legal display area
  assign valid = (hcnt >= HSTART & hcnt < HSTART+WIDTH &
                  vcnt >= VSTART & vcnt < VSTART+HEIGHT);
  assign {r,g,b} = valid ? {r_int,g_int,b_int} : 24'b0;
endmodule

module videoGen(input  logic       clk,
                input  logic [9:0] x, y, x_cursor, y_cursor,
                input  logic [2:0] buttons,
           		  output logic [7:0] r_int, g_int, b_int);

  logic [7:0] ch;
  logic [23:0] background_rgb;
  
  background(clk, x, y, background_rgb);
  draw_cursor(x, y, x_cursor, y_cursor, buttons,
              background_rgb, {r_int, g_int, b_int});
endmodule

// draw a cursor centered at {x_cursor, y_cursor} that changes
// color based on the buttons pressed
module draw_cursor(input  logic [9:0]  x, y, x_cursor, y_cursor,
                   input  logic [2:0]  buttons,
                   input  logic [23:0] background_rgb,
                   output logic [23:0] rgb);
  logic [23:0] cursor_color;
  logic        in_cursor;
  logic        in_hitbox;
  in_ellipse cursor(x, y, x_cursor, y_cursor, 140,1,0, in_cursor);
  in_disk hitbox(x, y, x_cursor, y_cursor, 8, in_hitbox);
  
  
  assign cursor_color = {buttons[2]? 8'h80 : 8'hFF,
                         buttons[1]? 8'h80 : 8'hFF,
                         buttons[0]? 8'h80 : 8'hFF};
  
  assign rgb = in_hitbox ? 24'hFFFFFF : 
               in_cursor ? {8'((9'(cursor_color[23:16])
                                +background_rgb[23:16])>>1),
                            8'((9'(cursor_color[15: 8])
                                +background_rgb[15: 8])>>1),
                            8'((9'(cursor_color[ 7: 0])
                                +background_rgb[ 7: 0])>>1)}: 
               background_rgb;
                   
endmodule

// Calculate if a point lies in an disk centered at 
// {cent_x, cent_y} with radius sqrt(rad_squared).
module in_disk  (input  logic [9:0]  x, y, cent_x, cent_y,
                 input  logic [21:0] rad_squared,
                 output logic        is_in);
  logic signed [10:0] d_x, d_y;
  logic [21:0] dist_squared;
  always_comb begin
    d_x = (x-cent_x);
    d_y = (y-cent_y);
    dist_squared = d_x*d_x + d_y*d_y;
    is_in = (dist_squared < rad_squared);
  end
endmodule

// Calculate if a point lies in an ellipse centered at 
// {cent_x, cent_y} with x_radius (sqrt(rad_squared) >> x_reduce)
// and y_radius (sqrt(rad_squared) >> y_reduce).
module in_ellipse  (input  logic [9:0]  x, y, cent_x, cent_y,
                    input  logic [31:0] rad_squared,
                    input  logic [1:0]  x_reduce, y_reduce,
                    output logic        is_in);
  logic signed [15:0] d_x, d_y;
  logic [31:0] dist_squared;
  always_comb begin
    d_x = (x-cent_x) << x_reduce;
    d_y = (y-cent_y) << y_reduce;
    dist_squared = d_x*d_x + d_y*d_y;
    is_in = (dist_squared < rad_squared);
  end
endmodule

// creates an interesting gradient as a background based on
// the timing and position.
module background(input  logic       clk,
                  input  logic [9:0] x_screen, y_screen,
                  output logic [23:0] rgb);
logic [9:0] diff, sum, x, y;
logic [19:0] product; 
logic [7:0] r, g, b, x_wave, y_wave, intensity;
logic [10:0] counter;

always_ff @(posedge clk) begin
  counter <= counter + 1;
end

always_comb begin
  x = x_screen;
  y = y_screen;
  
  // diagonal gradient functions that move with time
  diff = 10'(x - y + (counter >> 0));
  sum  = 10'(x+y - (counter >> 0));
  product = diff*sum;
  
  // the moving curved gradients
  intensity = 8'(product[11:4] + (counter << 2)) >> 3;
  
  // the shifting "blinds" 
  x_wave = (8'((x<<2)-counter)>>>5) + (8'(-(x<<2)-counter)>>>5);
  y_wave = (8'((y<<2)-counter)>>>6) + (8'(-(y<<2)-counter)>>>6);
  
  // have each color play separate roles in each pattern
  r = 8'h70 + x_wave + y_wave;
  g = 8'h80 + ~intensity;
  b = 8'hC0 + intensity + x_wave + y_wave;
  rgb = {r,g,b};
end

endmodule

// reads input from a frame enabled SPI to word_input
// and signals the end of the transmission with finished
module spi_frm_slave #(parameter WIDTH_POWER = 5, WIDTH = 2**WIDTH_POWER)
                      (input  logic               spi_clk,
                       input  logic               serial_input,
                       input  logic               fsync,
                       output logic [(WIDTH-1):0] word_input,
                       output logic               finished);
  logic [(WIDTH_POWER-1):0] counter = 0;
  
  always_ff @ (posedge spi_clk) begin
    word_input <= finished ? word_input : 
                            {word_input[(WIDTH-2):0], serial_input};
    counter <= finished ? 1 : counter + 1;
    finished <= ~fsync ? '0 : finished ? '1 : counter == 0;
  end
  
endmodule

// converts the values from word_input to recreate the
// mouse state in the form of position and buttons.
module mouse_reader  (input  logic        sync_clk,
                      input  logic [31:0] spi_word,
                      input  logic        finished,
                      output logic [9:0]  x_pixel, y_pixel,
                      output logic [2:0]  button_state);
  logic [22:0] complete_word;
  always_ff @ (posedge finished) begin
    complete_word <= spi_word[22:0];
  end
  always_ff @ (posedge sync_clk) begin
    {button_state, x_pixel, y_pixel} = complete_word;
  end
endmodule
  



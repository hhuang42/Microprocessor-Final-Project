// vga.sv
// 20 October 2011 Karl_Wang & David_Harris@hmc.edu
// VGA driver with character generator

module vga_DS(input  logic       clk,
			  input logic sck, sdi,
			  output logic       vgaclk,						// 25 MHz VGA clock
			  output logic       hsync, vsync, sync_b,	// to monitor & DAC
			  output logic [7:0] r, g, b, // to video DAC
			  output logic [7:0] led);					
 
  logic [9:0] x, y;
  logic [15:0] xpos, ypos;
  logic [7:0] r_int, g_int, b_int;
	
  // Use a PLL to create the 25.175 MHz VGA pixel clock 
  // 25.175 Mhz clk period = 39.772 ns
  // Screen is 800 clocks wide by 525 tall, but only 640 x 480 used for display
  // HSync = 1/(39.772 ns * 800) = 31.470 KHz
  // Vsync = 31.474 KHz / 525 = 59.94 Hz (~60 Hz refresh rate)
  pll	vgapll(.inclk0(clk),	.c0(vgaclk)); 

  // generate monitor timing signals
  vgaController vgaCont(vgaclk, hsync, vsync, sync_b,  
                        r_int, g_int, b_int, r, g, b, x, y);
	
  // user-defined module to determine pixel color
  videoGen videoGen(x, y, ~vsync, vgaclk, r_int, g_int, b_int);
  spi_slave_receive_only spireceive(sck, sdi, xpos, ypos);
  assign led[6:0] = ypos[6:0];
  assign led[7] = sdi;
  
endmodule

/*module writeled(input logic clk,
					 input logic [15:0] xpos,
					 output logic [7:0] led);
	always_comb begin
		led[0] = 0;
		led[1] = xpos[1];
		led[2] = xpos[2];
		led[3] = xpos[3];
		led[4] = xpos[4];
		led[5] = xpos[5];
		led[6] = xpos[6];
	end
endmodule
*/
				
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

module videoGen(input  logic [9:0] x, y,
                input  logic game_clk,
                input  logic pxl_clk,
           		  output logic [7:0] r_int, g_int, b_int);

 logic [23:0] intermediate_color [32:0];
 logic [50:0] circlerom [50:0];
 logic [21:0] counter = 0;
 
 assign intermediate_color[0] = 24'h808080;
 

  // given y position, choose a character to display
  // then look up the pixel value from the character ROM
  // and display it in red or blue
 

 //assign ch = y[8:3]+8'd48;
  //chargenrom chargenromb(ch, x[2:0], y[2:0], pixel);  
  always_ff@(posedge game_clk) begin
	counter <= counter - 1;
  end
 
 

 genvar index;
 generate
 for (index=0; index < 8; index=index+1)
   begin: gen_code_label
		octagonv2 test(x, y, 100+{index,7'b0}+counter[7:4], 100+{index,4'b0}+counter[7:5],counter[5:0], intermediate_color[index], 
					  24'hFFD000+counter, intermediate_color[index+1]); 
   end
 endgenerate
 draw_health hp(x,y, counter, intermediate_color[8], intermediate_color[9]);
 draw_score score(x,y, pxl_clk, counter, intermediate_color[9], {r_int, g_int, b_int});
 
endmodule



module octagon(input logic [9:0] x_pixel_orig, y_pixel_orig, x_cent_orig, y_cent_orig,
					input logic [9:0] ring_align_radius_offset, 
				  input logic [23:0] background, shape_color,
				  output logic [23:0] output_color);
logic [10:0] x_pixel, y_pixel, x_cent, y_cent;
logic [10:0] sum_pixel, diff_pixel, sum_cent, diff_cent;
logic in_octagon, in_outline, in_outer_ring, in_inner_ring;
logic [9:0] ring_align_radius;
logic [9:0] ring_diag_radius;
assign ring_align_radius = ring_align_radius_offset + 25;

assign ring_diag_radius = ring_align_radius + ring_align_radius[9:2]
                        + ring_align_radius[9:3] + ring_align_radius[9:5]+1;

assign x_pixel = x_pixel_orig + 100;
assign y_pixel = y_pixel_orig + 100;
assign x_cent = x_cent_orig + 100;
assign y_cent = y_cent_orig + 100;
assign sum_pixel = x_pixel + y_pixel;
assign diff_pixel = x_pixel - y_pixel + 1000;
assign sum_cent = x_cent + y_cent;
assign diff_cent = x_cent - y_cent + 1000;
assign in_octagon = (x_pixel < x_cent + 25) && (x_pixel > x_cent - 25) &&
						  (y_pixel < y_cent + 25) && (y_pixel > y_cent - 25) &&
						  (sum_pixel < sum_cent + 35) && (sum_pixel > sum_cent - 35) &&
			  			  (diff_pixel < diff_cent + 35) && (diff_pixel > diff_cent - 35);
assign in_outline = (x_pixel < x_cent + 30) && (x_pixel > x_cent - 30) &&
						  (y_pixel < y_cent + 30) && (y_pixel > y_cent - 30) &&
						  (sum_pixel < sum_cent + 42) && (sum_pixel > sum_cent - 42) &&
			  			  (diff_pixel < diff_cent + 42) && (diff_pixel > diff_cent - 42);

assign in_inner_ring =    (x_pixel < x_cent + ring_align_radius) && (x_pixel > x_cent - ring_align_radius) &&
								  (y_pixel < y_cent + ring_align_radius) && (y_pixel > y_cent - ring_align_radius) &&
								  (sum_pixel < sum_cent + ring_diag_radius) && (sum_pixel > sum_cent - ring_diag_radius) &&
								  (diff_pixel < diff_cent + ring_diag_radius) && (diff_pixel > diff_cent - ring_diag_radius);
								  
assign in_outer_ring =    (x_pixel < x_cent + ring_align_radius + 5) && (x_pixel > x_cent - ring_align_radius - 5) &&
								  (y_pixel < y_cent + ring_align_radius + 5) && (y_pixel > y_cent - ring_align_radius - 5) &&
								  (sum_pixel < sum_cent + ring_diag_radius + 7) && (sum_pixel > sum_cent - ring_diag_radius - 7) &&
								  (diff_pixel < diff_cent + ring_diag_radius + 7) && (diff_pixel > diff_cent - ring_diag_radius - 7);
						  
							
always_comb
	begin 
	if (in_octagon || (in_outer_ring && ~(in_inner_ring)))
		output_color = shape_color;
	else if (in_outline)
		output_color = 24'hFFFFFF;
	else
		output_color = background;
end
endmodule

module octagonv2(input logic [9:0] x_pixel_orig, y_pixel_orig, x_cent_orig, y_cent_orig,
					input logic [9:0] ring_align_radius_offset, 
				  input logic [23:0] background, shape_color,
				  output logic [23:0] output_color);
logic [9:0] x_pixel, y_pixel, x_cent, y_cent;
logic [10:0] sum_pixel, diff_pixel, sum_cent, diff_cent;
logic [9:0] ring_align_radius;
logic [9:0] min_NS, min_EW, min_NESW, min_NWSE, max_card, max_diag, max_diag_norm, max;
assign ring_align_radius = ring_align_radius_offset + 25;

assign x_pixel = x_pixel_orig;
assign y_pixel = y_pixel_orig;
assign x_cent = x_cent_orig;
assign y_cent = y_cent_orig;
assign sum_pixel = x_pixel + y_pixel;
assign diff_pixel = x_pixel + y_cent;
assign sum_cent = x_cent + y_cent;
assign diff_cent = x_cent + y_pixel;
assign min_NS = y_pixel > y_cent ? y_pixel - y_cent : y_cent - y_pixel;
assign min_EW = x_pixel > x_cent ? x_pixel - x_cent : x_cent - x_pixel;
assign min_NWSE = diff_pixel > diff_cent ? diff_pixel - diff_cent : diff_cent - diff_pixel;
assign min_NESW = sum_pixel > sum_cent ? sum_pixel - sum_cent : sum_cent - sum_pixel;
assign max_card = min_NS > min_EW ? min_NS : min_EW;
assign max_diag = min_NESW > min_NWSE ? min_NESW : min_NWSE;
assign max_diag_norm = max_diag - max_diag[9:2] - max_diag[9:5];
assign max = max_card > max_diag_norm ? max_card : max_diag_norm;
						  
							
always_comb
	begin 
	if (max < 25 || (max > ring_align_radius && max < ring_align_radius + 5))
		output_color = shape_color;
	else if (max < 30)
		output_color = 24'hFFFFFF;
	else
		output_color = background;
end
endmodule

/*module chargenrom(input  logic [7:0] ch,
                  input  logic [2:0] xoff, yoff,
						output logic       pixel);
						
  logic [5:0] charrom[2047:0]; // character generator ROM
  logic [7:0] line;            // a line read from the ROM
  logic [7:0] mouseline;
 
 // initialize ROM with characters from text file 
  initial 
	 $readmemb("charrom.txt", charrom);
	 
  // index into ROM to find line of character
  assign line = {charrom[yoff+{ch, 3'b000}]};
   
  //reverse order of bits
  assign pixel = line[3'd7-xoff];
  
endmodule
*/

module store_data(input logic[31:0] octagondata,
						output logic [31:0] datalist[7:0]);
endmodule

module to_synch_time(input logic [31:0] timerdata,
							output logic [9:0] mousexpos, mouseypos);
endmodule

module draw_health #(parameter WIDTH_SCALING = 0,
                               HEIGHT = 10,
                               BASE_Y = 20,
                               BASE_X = 50
                               )
                   (input logic [9:0] x, y,
                    input logic [7:0] health,
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic in_full_bar;
  logic health_area;
  assign in_full_bar = (x >= BASE_X && x < BASE_X + 255 &&
                        y >= BASE_Y && y < BASE_Y + HEIGHT);
  assign health_area = (x < BASE_X + (health >> WIDTH_SCALING));
  assign output_color = in_full_bar? health_area? 24'hFFFFFF : 24'h000000 : 
                                     background;
endmodule

module draw_score #(parameter DIGIT_WIDTH_BIT = 8,
                              DECIMAL_DIGIT_COUNT = 7,
                              HEIGHT = 8,
                              BASE_Y = 20,
                              BASE_X = 550
                              )
                   (input logic [9:0] x, y,
                    input logic pxl_clk,
                    input logic [21:0] score,
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic in_score_area;
  logic [7:0] ch;
  logic [9:0] delta_y, delta_x;
  logic [3:0] decimal_digits [6:0];
  genvar index;
  assign delta_y = (y-BASE_Y);
  assign delta_x = (x-BASE_X);
  to_decimal decimal_score(pxl_clk, score, decimal_digits);
  assign ch = decimal_digits[6-delta_x[8:3]]+8'd48;
  chargenrom chargenromb(ch, delta_x[2:0], delta_y[2:0], pixel);  
  assign in_score_area = (x >= BASE_X && 
                          x <= BASE_X + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT &&
                          y >= BASE_Y && y < BASE_Y + HEIGHT);
  assign output_color = (pixel && in_score_area)? 24'hFFFFFF : background;
endmodule

module to_decimal #(parameter BINARY_DIGIT_COUNT = 22,
                              DECIMAL_DIGIT_COUNT = 7,
                              DIGIT_COUNTER_WIDTH = 3)
                   (input logic clk,
                    input logic [21:0] value,
                    output logic [3:0] decimal_digits [6:0]
                    );
  logic [21:0] old_value;
  logic [21:0] leftover_value;
  logic [2:0] digit_counter;
  logic [3:0] temporary_buffer [6:0];
  always_ff @(posedge clk) begin
    if (old_value != value) begin
      leftover_value <= value;
      old_value <= value;
      digit_counter <= 0;
    end else if (leftover_value > 0) begin 
      leftover_value <= leftover_value / 10;
      temporary_buffer[digit_counter] <= leftover_value % 10;
      digit_counter = digit_counter + 1;
    end else begin
      decimal_digits = temporary_buffer;
    end
    
  end
endmodule


module chargenrom(input  logic [7:0] ch,
                  input  logic [2:0] xoff, yoff,
						output logic       pixel);
						
  logic [5:0] charrom[2047:0]; // character generator ROM
  logic [7:0] line;            // a line read from the ROM
  logic [7:0] mouseline;
 
 // initialize ROM with characters from text file 
  initial 
	 $readmemb("charrom.txt", charrom);
	 
  // index into ROM to find line of character
  assign line = {charrom[yoff+{ch, 3'b000}]};
   
  //reverse order of bits
  assign pixel = line[3'd7-xoff];
  
endmodule

// If the slave only need to received data from the master
// Slave reduces to a simple shift register given by following HDL: 
module spi_slave_receive_only(input logic sck, //from master
							input logic sdi, //from master 
							output logic[15:0] xpos, ypos); // data received
	logic [31:0] q; //internal logic shift register q
	always_ff @(negedge sck)
	begin
	q <={q[30:0], sdi}; //shift register
	{xpos, ypos} <= q;
	end
endmodule





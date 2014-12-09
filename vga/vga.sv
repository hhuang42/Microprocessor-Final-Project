// vga.sv
// 20 October 2011 Karl_Wang & David_Harris@hmc.edu
// VGA driver with character generator

module vga_DS(input  logic       clk,
			  input logic sck, sdi, fsync,
			  output logic       vgaclk,						// 25 MHz VGA clock
			  output logic       hsync, vsync, sync_b,	// to monitor & DAC
			  output logic [7:0] r, g, b, // to video DAC
			  output logic [7:0] led);					
 
  logic [9:0] x, y;
  logic [31:0] final_input;
  logic new_from_SPI;
  logic [7:0] r_int, g_int, b_int;
  logic [2:0] startpointer, endpointer;
	
  // Use a PLL to create the 25.175 MHz VGA pixel clock 
  // 25.175 Mhz clk period = 39.772 ns
  // Screen is 800 clocks wide by 525 tall, but only 640 x 480 used for display
  // HSync = 1/(39.772 ns * 800) = 31.470 KHz
  // Vsync = 31.474 KHz / 525 = 59.94 Hz (~60 Hz refresh rate)
  pll	vgapll(.inclk0(clk),	.c0(vgaclk)); 

  // generate monitor timing signals
  vgaController vgaCont(vgaclk, hsync, vsync, sync_b,  
                        r_int, g_int, b_int, r, g, b, x, y);
	
  spi_frm_slave spireceive(sck, clk, sdi, fsync, final_input, new_from_SPI);
	
  // user-defined module to determine pixel color
  videoGen videoGen(x, y, ~vsync, clk, final_input, new_from_SPI, r_int, g_int, b_int,
						  startpointer, endpointer, led);
  
endmodule

module datatypehiscore (input logic pxl_clk,
							  input  logic [31:0] datafromSPI,
							  output logic [3:0] hi_score_digits [6:0]);
  logic valid;
  assign valid = (datafromSPI[31:27] == 5'b10001);
	 always_ff@(posedge pxl_clk) begin
      if (valid) begin
        hi_score_digits[6] <= datafromSPI[26:24];
        hi_score_digits[5] <= datafromSPI[23:20];
        hi_score_digits[4] <= datafromSPI[19:16];
        hi_score_digits[3] <= datafromSPI[15:12];
        hi_score_digits[2] <= datafromSPI[11:8];
        hi_score_digits[1] <= datafromSPI[7:4];
        hi_score_digits[0] <= datafromSPI[3:0];
        
      end
	 end
endmodule

module datatypepath (input logic pxl_clk,
							  input logic [31:0] datafromSPI,
							  input logic        restart,
							  output logic [9:0] start_x, start_y, end_x, end_y,
							  output logic       show_path = 0);
  logic start_valid, end_valid;
  assign start_valid = (datafromSPI[31:29] == 3'b111) && !show_path;
  assign end_valid = (datafromSPI[31:28] == 4'b1001);
	 always_ff@(posedge pxl_clk) begin
     start_x <= start_valid?datafromSPI[28:19]:start_x;
     start_y <= start_valid?datafromSPI[18:9]:start_y;
     
		 show_path <= restart?0:end_valid?datafromSPI[27]:show_path;
		 end_x <= end_valid?datafromSPI[26:17]:end_x;
		 end_y <= end_valid?datafromSPI[16:7]:end_y;
	 end
endmodule

module datatyperoll (input logic pxl_clk,
							  input logic [31:0] datafromSPI,
							  input logic        restart,
							  output logic [9:0] roll_x, roll_y,
							  output logic       show_roll = 0);
  logic valid;
    assign valid = (datafromSPI[31:29] == 3'b101);
  
	 always_ff@(posedge pxl_clk) begin
		 roll_x <= valid?datafromSPI[27:18]:roll_x;
		 roll_y <= valid?datafromSPI[17:8]:roll_y;
		 show_roll <= restart?0:valid?datafromSPI[28]:show_roll;
	 end
endmodule

module datatypeextra (input logic pxl_clk,
							  input logic [31:0] datafromSPI,
							  output logic restart = 1,
							  output logic game_over = 0);
  
	 always_ff@(posedge pxl_clk) begin
		 restart <= (datafromSPI == 32'h80000000);
		 game_over <= restart?0:(datafromSPI == 32'h80000001)?1:game_over;
	 end
endmodule

module datatyperesult (input logic pxl_clk,
							  input logic [31:0] datafromSPI,
							  input logic        restart,
							  output logic [9:0] result_x, result_y,
							  output logic [1:0] result_value,
							  output logic       show_result = 0);
  logic valid;
    assign valid = (datafromSPI[31:29] == 3'b110);
  
	 always_ff@(posedge pxl_clk) begin
		 result_x <= valid?datafromSPI[28:19]:result_x;
		 result_y <= valid?datafromSPI[18:9]:result_y;
		 result_value <= valid?datafromSPI[8:7]:result_value;
		 show_result <= restart?0:valid?1:show_result;
	 end
endmodule

module datatypeoctagon(input logic [31:0] datafromSPI,
                       input logic [7:0] current_time,
							  input logic new_from_SPI,
                       output logic [31:0] datatoringbuffer,
                       output logic buffer_receive);
                       
  always_comb begin
  //octagon state moved to datatoringbuffer
    datatoringbuffer[31] = datafromSPI[29];
	 
	//xcenter of octagon moved to datatoringbuffer
    datatoringbuffer[30:21] = datafromSPI[28:19];
	 
	 //ycenter of octagon moved to datatoringbuffer
    datatoringbuffer[20:11] = datafromSPI[18:9];
	 
	 //target time defined here
    datatoringbuffer[10:3] = current_time + 60;
	 //point value if we're 
	 //checking if the data from the SPI is octagon data or not
      if (datafromSPI[31:30] == 2'b11 && new_from_SPI)
        buffer_receive = 1;
      else
        buffer_receive = 0;
    end
endmodule

module datatypetimer(input logic pxl_clk,
                     input logic [31:0] datafromSPI,
                     output logic [7:0] currenttime,
                     output logic [9:0] mouse_xpos, mouse_ypos);
  logic valid;
    assign valid = (datafromSPI[31:30] == 2'b01);
      
  always_ff@ (posedge pxl_clk) begin
    currenttime <= valid?datafromSPI[29:22]:currenttime;
    mouse_xpos <= valid?datafromSPI[21:12]:mouse_xpos;
    mouse_ypos <= valid?datafromSPI[11:2]:mouse_ypos;
  end
endmodule

module datatypescore(input logic pxl_clk,
                     input logic [31:0] datafromSPI,
                     output logic [7:0] lifebar,
                     output logic [21:0] score);
  logic valid;
    assign valid = (datafromSPI[31:30] == 2'b00);
  
  always_ff@ (posedge pxl_clk) begin
    lifebar <= valid?datafromSPI[29:22]:lifebar;
    score <= valid?datafromSPI[21:0]:score;
  end
 endmodule
 
module ringbuffer(input logic pxl_clk,
						input logic reset,
                  input logic receiving,
                  input logic [31:0] ocdatareceived,
                  output logic [31:0] ring_buffer[7:0],
                  output logic [2:0] startpointer = 0,
						output logic [2:0] endpointer = 0);
  
  //initialize pointers i->endpointer
  //assign startpointer=0;
	
  always_ff@(posedge pxl_clk) begin
    if (reset) begin
		ring_buffer <= '{32'b0,32'b0,32'b0,32'b0,32'b0,32'b0,32'b0,32'b0};
		startpointer <= 0;
		endpointer <= 0;
	 end else begin
		 if ((receiving == 1) && (ocdatareceived[31] == 1)) begin
			ring_buffer[endpointer] <= ocdatareceived;
			endpointer <= endpointer + 1;
		 end
		 else if ((receiving == 1) && (ocdatareceived[31] == 0)) begin
			ring_buffer[startpointer] <= ocdatareceived;
			startpointer <= startpointer + 1;
		 end
	 end
  end
endmodule

module separator(input logic pxl_clk,
                 input logic [7:0] current_time,
                 input logic [31:0] octagon_data,
                 output logic os,
                 output logic [9:0] x_cent, y_cent,
                 output logic [7:0] ring_size);
logic [7:0] target_time;

always_comb begin
  os = octagon_data[31];
  x_cent = octagon_data[30:21];
  y_cent = octagon_data[20:11];
  target_time = octagon_data[10:3];
  ring_size = (target_time - current_time);
end
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

module videoGen(input  logic [9:0] x, y,
                input  logic game_clk,
                input  logic pxl_clk,
					 input logic [31:0] final_input,
					 input logic new_from_SPI,
           		 output logic [7:0] r_int, g_int, b_int,
					 output logic [2:0] startpointer, endpointer,
					 output logic [7:0] led);

 logic [23:0] intermediate_color [32:0];
 logic octagon_state [7:0];
 logic [9:0] x_center[7:0], y_center[7:0];
 logic [7:0] ring_size[7:0];
 logic [31:0] buffer[7:0];
 logic [21:0] score;
 logic [7:0] current_time;
 logic [7:0] lifebar;
 logic [9:0] result_x, result_y;
 logic [9:0] mouse_x, mouse_y;
 logic [9:0] roll_x, roll_y;
 logic [9:0] path_start_x, path_end_x, path_start_y, path_end_y;
 logic [1:0] result_value;
 logic [31:0] data_for_ring_buffer;
 logic [3:0] decimal_digits [6:0];
 logic [3:0] hi_score_digits [6:0];
 logic ring_buffer_enable, roll_enable, path_enable;
 logic show_result;
 logic restart, game_over;
 
 
 //assign intermediate_color[0] = x*y+24'h808080;
 datatypehiscore (pxl_clk,final_input, hi_score_digits);
 datatypeextra(pxl_clk, final_input, restart, game_over);
 
 datatyperoll(pxl_clk, final_input, restart, roll_x, roll_y, roll_enable);
 datatypepath(pxl_clk, final_input, restart, 
              path_start_x, path_start_y, path_end_x, path_end_y, path_enable);
 
 datatypeoctagon(final_input, current_time, new_from_SPI,
                       data_for_ring_buffer,
                       ring_buffer_enable);
 datatyperesult (pxl_clk, final_input, restart, result_x, result_y, result_value, show_result);
 datatypetimer(pxl_clk, final_input,
                     current_time,
                     mouse_x, mouse_y);
 datatypescore(pxl_clk, final_input, lifebar, score);

  // given y position, choose a character to display
  // then look up the pixel value from the character ROM
  // and display it in red or blue
 

 //assign ch = y[8:3]+8'd48;
  //chargenrom chargenromb(ch, x[2:0], y[2:0], pixel);
  
 to_decimal digits(pxl_clk, score, decimal_digits);
 
 ringbuffer ring(pxl_clk, restart, ring_buffer_enable, data_for_ring_buffer, 
                 buffer, startpointer, endpointer);
//module separator(input logic pxl_clk,
//                 input logic [7:0] current_time,
//                 input logic [2:0] startpointer,
//                 input logic [31:0] octagon_data,
//                 output logic os,
//                 output logic [9:0] x_cent, y_cent,
//                 output logic [7:0] ring_size);
 draw_path path(x, y,path_enable, path_start_x, path_start_y,
                            path_end_x, path_end_y, x^y+24'h808080, intermediate_color[0]);
 path_end_octagonv2(path_enable, x, y,  path_end_x, path_end_y, intermediate_color[0], intermediate_color[1]);
 genvar index;
 generate
 for (index=0; index < 8; index=index+1)
   begin: gen_code_label
    separator sep(pxl_clk, current_time, buffer[startpointer + 7 - index], 
                  octagon_state[index], x_center[index], y_center[index], ring_size[index]
    );
		octagonv2 test(octagon_state[index], x, y, x_center[index], y_center[index], ring_size[index], startpointer + 7 - index,
            intermediate_color[index+1], 24'h4040FF, intermediate_color[index+1+1]); 
            
   end
 endgenerate
 roll_octagonv2 roll(roll_enable,x,y,roll_x,roll_y,intermediate_color[9], intermediate_color[10]);
 draw_result result(x,y, result_x, result_y, show_result, result_value, intermediate_color[10], intermediate_color[11]);
 draw_health hp(x,y, lifebar, intermediate_color[11], intermediate_color[12]);
 draw_score sc(x,y, decimal_digits, intermediate_color[12], intermediate_color[13]);
 draw_game_over go(x, y, game_over, hi_score_digits, decimal_digits, current_time, intermediate_color[13], intermediate_color[14]);
 draw_mouse mo(x,y,mouse_x, mouse_y, intermediate_color[14], {r_int, g_int, b_int});
 
 assign led = current_time;
endmodule



module octagonv2( input logic active,
          input logic [9:0] x_pixel_orig, y_pixel_orig, x_cent_orig, y_cent_orig,
					input logic [9:0] ring_align_radius_offset, 
          input logic [2:0] target_order,
				  input logic [23:0] background, shape_color,
				  output logic [23:0] output_color);
logic [9:0] x_pixel, y_pixel, x_cent, y_cent;
logic [9:0] ring_align_radius;
logic [9:0] min_NS, min_EW, max_card, max_diag, max_diag_norm, max;
logic in_number_region, pixel;
assign ring_align_radius = ring_align_radius_offset + 25;

assign x_pixel = x_pixel_orig;
assign y_pixel = y_pixel_orig;
assign x_cent = x_cent_orig;
assign y_cent = y_cent_orig;
assign min_NS = y_pixel > y_cent ? y_pixel - y_cent : y_cent - y_pixel;
assign min_EW = x_pixel > x_cent ? x_pixel - x_cent : x_cent - x_pixel;
assign max_card = min_NS > min_EW ? min_NS : min_EW;
assign max_diag = min_NS+min_EW;
assign max_diag_norm = max_diag - max_diag[9:2] - max_diag[9:5];
assign max = max_card > max_diag_norm ? max_card : max_diag_norm;
						  
							
chargenrom order(target_order + 8'd48 + 1, (x_pixel - x_cent + 8)>>1, (y_pixel - y_cent + 8)>>1, pixel);
assign in_number_region = (x_pixel - x_cent + 8 < 16) && (y_pixel - y_cent + 8 < 16);
always_comb
	begin 
  if (!active || max > 85)
    output_color = background;
  else if (in_number_region && pixel)
    output_color = 24'hFFFFFF;
	else if (max < 25 || (max > ring_align_radius && max < ring_align_radius + 5))
		output_color = shape_color;
	else if (max < 30)
		output_color = 24'hFFFFFF;
	else
		output_color = background;
end
endmodule

module roll_octagonv2( input logic active,
							  input logic [9:0] x_pixel_orig, y_pixel_orig, x_cent_orig, y_cent_orig,
							  input logic [23:0] background, 
				           output logic [23:0] output_color);
logic [9:0] x_pixel, y_pixel, x_cent, y_cent;
logic [9:0] ring_align_radius;
logic [9:0] min_NS, min_EW, max_card, max_diag, max_diag_norm, max;

assign x_pixel = x_pixel_orig;
assign y_pixel = y_pixel_orig;
assign x_cent = x_cent_orig;
assign y_cent = y_cent_orig;
assign min_NS = y_pixel > y_cent ? y_pixel - y_cent : y_cent - y_pixel;
assign min_EW = x_pixel > x_cent ? x_pixel - x_cent : x_cent - x_pixel;
assign max_card = min_NS > min_EW ? min_NS : min_EW;
assign max_diag = min_NS+min_EW;
assign max_diag_norm = max_diag - max_diag[9:2] - max_diag[9:5];
assign max = max_card > max_diag_norm ? max_card : max_diag_norm;
						  
							
always_comb
	begin 
  if (!active)
    output_color = background;
	else if (max < 25)
		output_color = max[2]?24'hFFFF00:24'hFF8000;
  else if (max > 50 && max <=55)
    output_color = 24'hFF8000;
	else
		output_color = background;
end
endmodule

module path_end_octagonv2( input logic active,
							  input logic [9:0] x_pixel_orig, y_pixel_orig, x_cent_orig, y_cent_orig,
							  input logic [23:0] background, 
				           output logic [23:0] output_color);
logic [9:0] x_pixel, y_pixel, x_cent, y_cent;
logic [9:0] min_NS, min_EW, max_card, max_diag, max_diag_norm, max;

assign x_pixel = x_pixel_orig;
assign y_pixel = y_pixel_orig;
assign x_cent = x_cent_orig;
assign y_cent = y_cent_orig;
assign min_NS = y_pixel > y_cent ? y_pixel - y_cent : y_cent - y_pixel;
assign min_EW = x_pixel > x_cent ? x_pixel - x_cent : x_cent - x_pixel;
assign max_card = min_NS > min_EW ? min_NS : min_EW;
assign max_diag = min_NS+min_EW;
assign max_diag_norm = max_diag - max_diag[9:2] - max_diag[9:5];
assign max = max_card > max_diag_norm ? max_card : max_diag_norm;
						  
							
always_comb
	begin 
  if (!active)
    output_color = background;
	else if (max < 30)
		output_color = 24'hFF8000;
	else
		output_color = background;
end
endmodule

module draw_result #(parameter DIGIT_WIDTH_BIT = 16,
										 DECIMAL_DIGIT_COUNT = 3,
										 HEIGHT = 16,
										 BASE_Y = 20,
										 BASE_X = 550
										 )
						 (input logic [9:0] x, y,
						  input logic [9:0] center_x, center_y,
						  input logic show_result,
                    input logic [1:0] value,
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic [23:0] result_color;
  logic in_score_area, pixel;
  logic [7:0] ch;
  logic [9:0] delta_y, delta_x;
  logic [3:0] decimal_digits [2:0];
  assign delta_y = (y-center_y + HEIGHT/2);
  assign delta_x = (x-center_x + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT/2);
  assign decimal_digits[0] = 3'b0;
  assign decimal_digits[1] = 3'b0;
  assign decimal_digits[2] = value;
  assign ch = decimal_digits[2-delta_x[7:4]]+8'd48;
  chargenrom chargenromb(ch, delta_x[3:1], delta_y[3:1], pixel);  
  assign in_score_area = (x + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT/2 >= center_x && 
                          x <= center_x + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT/2 &&
                          y + HEIGHT/2 >= center_y && y < center_y + HEIGHT/2);
  assign result_color = (value[1]? value[0]? 24'h80FF80 : 24'hF0F0FF:
											  value[0]? 24'hFFFF80 : 24'hFF0000);
  assign output_color = (pixel && in_score_area && show_result)? result_color : background;
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
                    input logic [3:0] decimal_digits [6:0],
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic in_score_area, pixel;
  logic [7:0] ch;
  logic [9:0] delta_y, delta_x;
  assign delta_y = (y-BASE_Y);
  assign delta_x = (x-BASE_X);
  assign ch = decimal_digits[6-delta_x[8:3]]+8'd48;
  chargenrom chargenromb(ch, delta_x[2:0], delta_y[2:0], pixel);  
  assign in_score_area = (x >= BASE_X && 
                          x <= BASE_X + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT &&
                          y >= BASE_Y && y < BASE_Y + HEIGHT);
  assign output_color = (pixel && in_score_area)? 24'hFFFFFF : background;
endmodule

module draw_game_over #(parameter DIGIT_WIDTH_BIT = 16,
                              DECIMAL_DIGIT_COUNT = 7,
                              HEIGHT = 32,
                              BASE_Y = 200,
                              BASE_X = 250
                              )
							 (input logic [9:0] x, y,
							  input logic show_game_over,
							  input logic [3:0] hi_score_digits [6:0],
                input logic [3:0] decimal_digits [6:0],
							  input logic [7:0] game_over_timer,
							  input logic [23:0] background,
							  output logic [23:0] output_color);
  logic cover_black;
  logic [9:0] delta_x, delta_y;
  logic [7:0] ch;
  
  logic pixel, in_score_area;
  assign cover_black = (y[9:1] < game_over_timer);
  assign delta_y = (y-BASE_Y + 510 - {game_over_timer,1'b0});
  assign delta_x = (x-BASE_X);
  assign ch = delta_y < 16? decimal_digits[6-delta_x[8:4]]+8'd48: 
                            hi_score_digits[6-delta_x[8:4]]+8'd48;
  chargenrom chargenromb(ch, delta_x[3:1], delta_y[3:1], pixel);
  assign in_score_area = (x >= BASE_X && 
                          x <= BASE_X + DIGIT_WIDTH_BIT*DECIMAL_DIGIT_COUNT &&
                          y >= BASE_Y + {game_over_timer,1'b0} - 510 && 
								  y < BASE_Y + HEIGHT + {game_over_timer,1'b0} - 510);
  assign output_color = ~show_game_over? background : (pixel && in_score_area)? 24'hFFFFFF : 
								 cover_black? {2'b0,background[23:18],2'b0,background[15:10],2'b0, background[7:2]} : background;
  
endmodule

module to_decimal #(parameter BINARY_DIGIT_COUNT = 22,
                              DECIMAL_DIGIT_COUNT = 7,
                              DIGIT_COUNTER_WIDTH = 3)
                   (input logic clk,
                    input logic [21:0] value,
                    output logic [3:0] decimal_digits [6:0]
                    );
  logic [21:0] old_value = 0;
  logic [21:0] leftover_value = 0;
  logic [2:0] digit_counter;
  logic [3:0] temporary_buffer [6:0];
  always_ff @(posedge clk) begin
    if (old_value != value) begin
      leftover_value <= value;
      old_value <= value;
      digit_counter <= 0;
    end else if (digit_counter < DECIMAL_DIGIT_COUNT) begin 
      leftover_value <= leftover_value / 10;
      temporary_buffer[digit_counter] <= leftover_value % 10;
      digit_counter <= digit_counter + 1;
    end else begin
      decimal_digits = temporary_buffer;
    end
    
  end
endmodule

module draw_mouse #(parameter RADIUS = 6,
									   THICKNESS = 1)
                   (input logic [9:0] x, y,
                    input logic [9:0] mouse_x, mouse_y,
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic in_mouse;
  assign in_mouse = (x + RADIUS >= mouse_x && x <= mouse_x + RADIUS &&
                     y + THICKNESS >= mouse_y && y <= mouse_y + THICKNESS) || 
						  (x + THICKNESS >= mouse_x && x <= mouse_x + THICKNESS &&
                     y + RADIUS >= mouse_y && y <= mouse_y + RADIUS);
  assign output_color = in_mouse? 24'hFF4040 : background;
endmodule

module draw_path #(parameter MAX_THICKNESS = 25)
                   (input logic [9:0] x, y,
                    input logic path_enable,
                    input logic [9:0] start_x, start_y,
                    input logic [9:0] end_x, end_y,
                    input logic [23:0] background,
                    output logic [23:0] output_color);
  logic signed [10:0] delta_x_total, delta_y_total;
  logic signed [10:0] delta_x, delta_y;
  logic signed [10:0] delta_long_total, delta_short_total, delta_long, delta_short;
  logic signed [21:0] fixed_compare, high_compare, low_compare;
  logic in_path;
  logic x_orient;
  
  assign delta_x_total = $signed(end_x - start_x);
  assign delta_y_total = $signed(end_y - start_y);
  assign delta_x = $signed(x - start_x);
  assign delta_y = $signed(y - start_y);
  assign x_orient = (delta_x_total > 0 ? delta_x_total: -delta_x_total) > 
                    (delta_y_total > 0 ? delta_y_total: -delta_y_total);
  assign {delta_long,delta_short, delta_long_total,delta_short_total} =
          x_orient ? {delta_x, delta_y, delta_x_total, delta_y_total}:
                     {delta_y, delta_x, delta_y_total, delta_x_total};
  assign fixed_compare = delta_short_total*delta_long;
  assign low_compare = (delta_short-MAX_THICKNESS)*delta_long_total;
  assign high_compare = (delta_short+MAX_THICKNESS)*delta_long_total;
  assign in_path = (((delta_long > 0) ==
                     (delta_long_total > delta_long)) &&
                     ((high_compare > fixed_compare) ==
                      (low_compare  < fixed_compare)));
  assign output_color = (in_path && path_enable)? 
      24'h7F7F00 + {1'b0,background[23:17],1'b0,background[15:9],1'b0, background[7:1]}: 
      background;
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

// reads input from a frame enabled SPI to word_input
// and signals the end of the transmission with finished
module spi_frm_slave #(parameter WIDTH_POWER = 5, WIDTH = 2**WIDTH_POWER)
                      (input  logic               spi_clk,
							  input  logic               pxl_clk,
                       input  logic               serial_input,
                       input  logic               fsync,
                       output logic [(WIDTH-1):0] final_input = 0,
							  output logic               new_from_SPI = 0
                       );
  logic [(WIDTH-1):0] word_input = '0;
  logic [(WIDTH_POWER-1):0] counter = 0;
  logic finished = 1, old_finished = 1;
  always_ff @ (posedge spi_clk) begin
    word_input <= finished ? word_input : 
                            {word_input[(WIDTH-2):0], serial_input};
    counter <= finished ? 1 : counter + 1;
    finished <= ~fsync ? '0 : finished ? '1 : counter == 0;
  end
  
  always_ff @ (posedge pxl_clk) begin
    old_finished <= finished;
	 new_from_SPI <= finished && !old_finished;
	 final_input  <= finished ? word_input : final_input;
  end
    
  
  
endmodule




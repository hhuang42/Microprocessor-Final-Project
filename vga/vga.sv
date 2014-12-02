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
  videoGen videoGen(x, y, r_int, g_int, b_int);
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
           		 output logic [7:0] r_int, g_int, b_int);
	
 logic [23:0] intermediate_color [31:0];
  

  // given y position, choose a character to display
  // then look up the pixel value from the character ROM
  // and display it in red or blue
 

 //assign ch = y[8:3]+8'd48;
  //chargenrom chargenromb(ch, x[2:0], y[2:0], pixel);  
 assign intermediate_color[0] = 24'hBAA0E0;
 genvar index;
 generate
 for (index=1; index < 16; index=index+1)
   begin: gen_code_label
		circles my_circle(x, y, 20*index, 10*index, intermediate_color[index-1], 
								24'hA0A0AA + 10*index, intermediate_color[index]); 
   end
 endgenerate
 assign {r_int, g_int, b_int} = intermediate_color[15];
// circles circle1(x, y, 300, 200, 24'hF0D0D0, 24'h00D0FF,intermediate_color[0]); 
// circles circle2(x, y, 310, 215, intermediate_color[0], 24'hFFD0FF,intermediate_color[1]); 
// circles circle3(x, y, 320, 225, intermediate_color[1], 24'hFFD0FF,intermediate_color[2]); 
// circles circle4(x, y, 330, 235, intermediate_color[2], 24'hFFD0FF,intermediate_color[3]); 
// circles circle5(x, y, 340, 245, intermediate_color[3], 24'hFFD0FF,intermediate_color[4]); 
// circles circle6(x, y, 350, 255, intermediate_color[4], 24'hFFD0FF,intermediate_color[5]); 
// circles circle7(x, y, 360, 265, intermediate_color[5], 24'hFFD0FF,intermediate_color[6]); 
// circles circle8(x, y, 370, 275, intermediate_color[6], 24'hFFD0FF,intermediate_color[7]); 
// circles circle9(x, y, 380, 285, intermediate_color[7], 24'hFFD0FF,intermediate_color[8]); 
// circles circlea(x, y, 390, 295, intermediate_color[8], 24'hFFD0FF,intermediate_color[9]); 
// circles circleb(x, y, 400, 305, intermediate_color[9], 24'hFFD0FF,{r_int, g_int, b_int}); 
  //assign {r_int, g_int, b_int}=((x-300)*(x-300)+(y-200)*(y-200) <= 625 )? {16'h8888, {{8{pixel}}}} : {{{8{pixel}}}, 16'h8888};
endmodule

module circles(input logic [9:0] x, y,
					input logic [9:0] xcenter, ycenter,
					input logic [23:0] background,
					input logic [23:0] circle_color,
					output logic [23:0] output_color);
					
	initial
		$readmemb("circle.txt", circle);
	assign output_color = ((x-xcenter)*(x-xcenter)+(y-ycenter)*(y-ycenter) <= 625 )? 
											 circle_color : 
											 background;

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



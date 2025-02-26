/*
 * SPDX-License-Identifier: Apache-2.0
 */

`default_nettype none

module vgademo(
  input  wire [7:0] ui_in,    // Dedicated inputs
  output wire [7:0] uo_out,   // Dedicated outputs
  input  wire [7:0] uio_in,   // IOs: Input path
  output wire [7:0] uio_out,  // IOs: Output path
  output wire [7:0] uio_oe,   // IOs: Enable path (active high: 0=input, 1=output)
  input  wire       ena,      // always 1 when the design is powered, so you can ignore it
  input  wire       clk,      // clock
  input  wire       rst_n     // reset_n - low to reset
);

  wire audio_out;
  wire [12:0] audio_sample;
  wire [10:0] song_position;  // for sequencing effects/transitions

  wire audio_sample_clk = pix_x == 0;
  wire music_tick_clk = (pix_x == 799) && (pix_y == 524);

  music music(
    .clk(clk),
    .rst_n(rst_n),
    .sample_clk(audio_sample_clk),
    .tick_clk(music_tick_clk),
    .audio_out(audio_out),
    .song_position(song_position),
    .audio_sample(audio_sample)
  );

  parameter H_DISPLAY       = 640; // horizontal display width
  parameter H_BACK          =  48; // horizontal left border (back porch)
  parameter H_FRONT         =  16; // horizontal right border (front porch)
  parameter H_SYNC          =  96; // horizontal sync width
  // vertical constants
  parameter V_DISPLAY       = 480; // vertical display height
  parameter V_TOP           =  33; // vertical top border
  parameter V_BOTTOM        =  10; // vertical bottom border
  parameter V_SYNC          =   2; // vertical sync # lines
  // derived constants
  parameter H_SYNC_START    = H_DISPLAY + H_FRONT;
  parameter H_SYNC_END      = H_DISPLAY + H_FRONT + H_SYNC - 1;
  parameter H_MAX           = H_DISPLAY + H_BACK + H_FRONT + H_SYNC - 1;
  parameter V_SYNC_START    = V_DISPLAY + V_BOTTOM;
  parameter V_SYNC_END      = V_DISPLAY + V_BOTTOM + V_SYNC - 1;
  parameter V_MAX           = V_DISPLAY + V_TOP + V_BOTTOM + V_SYNC - 1;

  parameter LOGO_WIDTH = 128;
  parameter LOGO_HEIGHT = 128;
  parameter LOGO_SPEED = 10'd2;

  reg [9:0] logo_x;
  reg [9:0] logo_y;
  reg logo_dx, logo_dy;

  reg logo_img[0:16383];
  initial $readmemh("../data/logo.hex", logo_img);

  wire [6:0] logo_u = pix_x[6:0] - logo_x[6:0];
  wire [6:0] logo_v = pix_y[6:0] - logo_y[6:0];

  reg [9:0] pix_x;
  reg [9:0] pix_y;

  wire [10:0] dxy = pix_x + pix_y;
  wire [5:0] transition_index = {2'b00, dxy[3:0]} + dxy[10:5];

  reg [12:0] frame_counter;

  //reg signed [5:0] ysqr;
  //reg signed [5:0] xsqr;

  //wire signed [17:0] xoffset = -320 + pix_x;
  //wire signed [18:0] xoffsetsqr = 320*320;
  //wire signed [17:0] yoffset = -240 + pix_y;
  //reg signed [18:0] yoffsetsqr;

  always @(posedge clk) begin
    if (!rst_n) begin
      pix_x <= 0;
      pix_y <= 0;
      logo_x <= 256;
      logo_y <= 128;
      logo_dx <= 0;
      logo_dy <= 1;
      //yoffsetsqr <= 240*248 - 30000;
      frame_counter <= 0;
    end else begin
      if (pix_x == H_MAX) begin
        //xsqr <= xoffsetsqr;
        //ysqr <= ysqr + {yoffset, 1'b1};
        pix_x <= 0;
        if (pix_y == V_MAX) begin
          pix_y <= 0;
          // new frame
          frame_counter <= frame_counter + 1;
          //ysqr <= yoffsetsqr;
          //yoffsetsqr <= yoffsetsqr + {yoffset, 1'b1};
          if (logo_dx && logo_x > 640 - LOGO_WIDTH - 2*LOGO_SPEED) begin
            logo_dx <= 0;
          end else if (!logo_dx && logo_x < 2*LOGO_SPEED) begin
            logo_dx <= 1;
          end
          if (logo_dy && logo_y > 480 - LOGO_HEIGHT - 2*LOGO_SPEED) begin
            logo_dy <= 0;
          end else if (!logo_dy && logo_y < 2*LOGO_SPEED) begin
            logo_dy <= 1;
          end
          logo_x <= logo_x + (logo_dx ? LOGO_SPEED : -LOGO_SPEED);
          logo_y <= logo_y + (logo_dy ? LOGO_SPEED : -LOGO_SPEED);
        end else begin
          pix_y <= pix_y + 1;
        end
      end else begin
        pix_x <= pix_x + 1;
        //xsqr <= xsqr + {xoffset, 1'b1};
      end
    end
  end

  //wire [1:0] incircle = (xsqr + ysqr) >> 14;

  wire logo_on = (pix_x >= logo_x && pix_x < logo_x + LOGO_WIDTH &&
                  pix_y >= logo_y && pix_y < logo_y + LOGO_HEIGHT) &&
                  logo_img[{logo_v, logo_u}];

  wire blank = pix_x >= H_DISPLAY || pix_y >= V_DISPLAY;

  wire [12:0] transition_frame = frame_counter - {7'b0, transition_index};
  wire intro_logo = transition_frame < 640;
  wire [1:0] intrologo_r = logo_on ? 2'b10 : 2'b11;
  wire [1:0] intrologo_g = logo_on ? 2'b00 : 2'b11;
  wire [1:0] intrologo_b = logo_on ? 2'b00 : 2'b11;

  wire circuit_screen = !intro_logo;
  wire circuit_trace = (pix_x[2:0] - pix_y[2:0]) == 0;
  wire [1:0] circuit_r = circuit_trace ? 2'b01 : 2'b00;
  wire [1:0] circuit_g = circuit_trace ? 2'b11 : 2'b01;
  wire [1:0] circuit_b = circuit_trace ? 2'b10 : 2'b01;

  assign R = blank ? 0 : pix_x < audio_sample[12:4] ? 2'b01 : intro_logo ? intrologo_r : circuit_r;
  assign G = blank ? 0 : pix_x < audio_sample[12:4] ? 2'b01 : intro_logo ? intrologo_g : circuit_g;
  assign B = blank ? 0 : pix_x < audio_sample[12:4] ? 2'b10 : intro_logo ? intrologo_b : circuit_b;
  

  // VGA signals
  wire hsync = ~(pix_x>=H_SYNC_START && pix_x<=H_SYNC_END);
  wire vsync = ~(pix_y>=V_SYNC_START && pix_y<=V_SYNC_END);
  wire [1:0] R;
  wire [1:0] G;
  wire [1:0] B;

  // TinyVGA PMOD
  assign uo_out = {hsync, B[0], G[0], R[0], vsync, B[1], G[1], R[1]};

  // Unused outputs assigned to 0.
  assign uio_out[7] = audio_out;
  assign uio_out[6:0] = 0;
  assign uio_oe[7] = 1;
  assign uio_oe[6:0] = 0;

  // Suppress unused signals warning
  wire _unused_ok = &{ena, ui_in, uio_in};

endmodule


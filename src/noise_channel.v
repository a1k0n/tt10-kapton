`default_nettype none

module noise_channel(
    input wire clk,
    input wire rst_n,
    input wire sample_clk,
    input wire tick_clk,
    input wire song_clk,
    input wire note_trigger,
    output wire [12:0] audio_out
);

reg [14:0] noise_lfsr;
reg [3:0] noise_vol;

assign audio_out = noise_vol == 15 ? 0 : {4'b0, noise_lfsr[7:0] >> noise_vol[3:1]};

always @(posedge clk) begin
    if (!rst_n) begin
        noise_lfsr <= 15'h7fff;
        noise_vol <= 4'hf;
    end else if (sample_clk) begin
        noise_lfsr <= {noise_lfsr[0], noise_lfsr[0] ^ noise_lfsr[14], noise_lfsr[13:1]};
    end else if (tick_clk) begin
        if (song_clk && note_trigger) begin
            noise_vol <= 4'h0;
        end else begin
            noise_vol <= (noise_vol == 15) ? 15 : noise_vol + 1;
        end
    end
end

endmodule
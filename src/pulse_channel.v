`default_nettype none

module pulse_channel(
    input wire clk,
    input wire sample_clk,
    input wire tick_clk,
    input wire song_clk,
    input wire rst_n,
    input wire note_on,
    input wire note_trigger,
    input wire [PHASE_BITS-1:0] phase_inc,
    output wire [12:0] audio_out
);

parameter DETUNE = 2;        // Frequency detune amount
parameter PULSE_WIDTH = 1;   // Pulse width (1 = 50% duty cycle)
parameter CARRIER_SHIFT = 1; // Secondary carrier bit shift
parameter RELEASE = 4;       // Envelope decay/release rate
parameter SUSTAIN_LEVEL = 8'h40;    // Sustain amplitude
parameter PHASE_BITS = 18;

reg [PHASE_BITS-1:0] phase1;
reg [PHASE_BITS-1:0] phase2;
reg [7:0] amplitude;

wire [7:0] target_amplitude = note_on ? SUSTAIN_LEVEL : 0;

wire phase1_bit0 = phase1[PHASE_BITS-1];
wire phase1_bit1 = phase1[PHASE_BITS-2];
wire phase2_bit0 = phase2[PHASE_BITS-1];
wire phase2_bit1 = phase2[PHASE_BITS-2];

wire [7:0] phase1_out = (phase1_bit0&phase1_bit1) ? amplitude : 0;
wire [7:0] phase2_out = (phase2_bit0&phase2_bit1) ? amplitude : 0;
assign audio_out = {4'b0, phase1_out} + {4'b0, phase2_out};

always @(posedge clk) begin
    if (!rst_n) begin
        phase1 <= 0;
        phase2 <= 0;
        amplitude <= 0;
    end else if (sample_clk) begin
        phase1 <= phase1 + phase_inc;
        phase2 <= phase2 + (phase_inc << CARRIER_SHIFT) + DETUNE;
    end else if (tick_clk) begin
        // $display("pulse note_trigger: %d, phase1=%d, phase2=%d, amplitude=%d", phase_inc, phase1, phase2, amplitude);
        if (song_clk && note_trigger) begin
            amplitude <= 8'hff;
        end else begin
            amplitude <= amplitude - ((amplitude - target_amplitude + ((1 << RELEASE)-1)) >> RELEASE);
        end
    end
end



endmodule


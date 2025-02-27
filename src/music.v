`default_nettype none

module music(
    input wire clk,
    input wire rst_n,
    input wire sample_clk,
    input wire tick_clk,
    output reg [$clog2(SONG_LENGTH)-1:0] song_position,
    output wire audio_out,
    output wire [12:0] audio_sample
);

parameter SONG_LENGTH = 512;

reg [2:0] tick_counter;  // 5 ticks per song position

wire song_clk = tick_counter == 4;

//wire [12:0] audio_sample;
reg [12:0] audio_dac_accum;
wire [13:0] audio_dac_accum_next = audio_dac_accum + audio_sample;
assign audio_out = audio_dac_accum_next[13];

reg bass_note_on[0:SONG_LENGTH-1];
reg bass_note_trigger[0:SONG_LENGTH-1];
reg [9:0] bass_phase_inc[0:SONG_LENGTH-1];
wire [12:0] bass_audio_out;

reg backup_note_on[0:SONG_LENGTH-1];
reg backup_note_trigger[0:SONG_LENGTH-1];
reg [15:0] backup_phase_inc[0:SONG_LENGTH-1];
wire [12:0] backup_audio_out;

reg melody_note_on[0:SONG_LENGTH-1];
reg melody_note_trigger[0:SONG_LENGTH-1];
reg [9:0] melody_phase_inc[0:SONG_LENGTH-1];
wire [12:0] melody_audio_out;

wire snare_note_trigger = song_position[2:0] == 4;
wire [12:0] snare_audio_out;

assign audio_sample = bass_audio_out + melody_audio_out + backup_audio_out + snare_audio_out;

initial begin
    $readmemh("../data/bass_on.hex", bass_note_on);
    $readmemh("../data/bass_trigger.hex", bass_note_trigger);
    $readmemh("../data/bass_pitch.hex", bass_phase_inc);

    $readmemh("../data/backup_on.hex", backup_note_on);
    $readmemh("../data/backup_trigger.hex", backup_note_trigger);
    $readmemh("../data/backup_pitch.hex", backup_phase_inc);

    $readmemh("../data/melody_on.hex", melody_note_on);
    $readmemh("../data/melody_trigger.hex", melody_note_trigger);
    $readmemh("../data/melody_pitch.hex", melody_phase_inc);
end

pulse_channel #(
    .PHASE_BITS(18),
    .RELEASE(2)
) bass_channel(
    .clk(clk),
    .rst_n(rst_n),
    .sample_clk(sample_clk),
    .tick_clk(tick_clk),
    .song_clk(song_clk),
    .note_on(bass_note_on[song_position]),
    .note_trigger(bass_note_trigger[song_position]),
    .phase_inc({8'b0, bass_phase_inc[song_position]}),
    .audio_out(bass_audio_out)
);

pulse_channel #(
    .PHASE_BITS(14),
    .RELEASE(4)
) melody_channel(
    .clk(clk),
    .rst_n(rst_n),
    .sample_clk(sample_clk),
    .tick_clk(tick_clk),
    .song_clk(song_clk),
    .note_on(melody_note_on[song_position]),
    .note_trigger(melody_note_trigger[song_position]),
    .phase_inc({8'b0, melody_phase_inc[song_position]}),
    .audio_out(melody_audio_out)
);

pulse_channel #(
    .PHASE_BITS(18),
    .PULSE_WIDTH(0),
    .RELEASE(2)
) backup_channel(
    .clk(clk),
    .rst_n(rst_n),
    .sample_clk(sample_clk),
    .tick_clk(tick_clk),
    .song_clk(song_clk),
    .note_on(backup_note_on[song_position]),
    .note_trigger(backup_note_trigger[song_position]),
    .phase_inc(backup_phase_inc[song_position]),
    .audio_out(backup_audio_out)
);

noise_channel snare_channel(
    .clk(clk),
    .rst_n(rst_n),
    .sample_clk(sample_clk),
    .tick_clk(tick_clk),
    .song_clk(song_clk),
    .note_trigger(snare_note_trigger),
    .audio_out(snare_audio_out)
);

always @(posedge clk) begin
    if (!rst_n) begin
        tick_counter <= 0;
        song_position <= 0;
        audio_dac_accum <= 0;
    end else begin
        if (tick_clk) begin
            if (song_clk) begin
                song_position <= song_position + 1;
                $display("song_position: %d", song_position);
                tick_counter <= 0;
            end else begin
                tick_counter <= tick_counter + 1;
            end
        end
        audio_dac_accum <= audio_dac_accum_next;
    end
end

endmodule

-- Testbench for the decoder.vhd

library ieee;
use ieee.std_logic_1164.ALL;
use ieee.numeric_std.all;
 
entity decoder_tb is
end decoder_tb;
 
architecture behave of decoder_tb is
 
  component decoder is

    port (
    i_Clk : IN  std_logic; -- clock
    i_Rst : IN  std_logic; -- reset
    i_Sample : IN  std_logic; -- RX signal to sample
    o_data : OUT std_logic_vector(31 downto 0);  -- 32 bits of output data
    o_RecordingBits : OUT std_logic
    );
  end component decoder;

   
  -- Test Bench uses a 140 MHz Clock
 
  constant c_PULSE_PERIOD : time := 11.35 ns;
  constant c_PIXEL_PERIOD : time := c_PULSE_PERIOD * 24;
  -- the MSB of c_raw_samples_1 is the first sample received (in time)
  constant c_raw_samples_1 : std_logic_vector := "01101101100100110111000111100011100011011011001001101101111000111000111000110110010011011011011110001110001110001100100110110110110011100011100011110001101101101101100100111000111100011100011011011011001001101110001111000111000110110010010011011011110001110001110001100100110110110110011100011100011110001001101101101100100111000111000111100011011011011001001101110001111000111000110110110010011011011110001110001110001101100100110110110111100011100011110001001101101101101100111000111000111100011011011011011001001110001111000111000110110110010011011011100011110001110001101100100110110110111100011100011100011001001101101101100111000111000111100010011011011011001001110001111000111000110110110110010011011100011110001110001101100100100110110111100011100011100011001001101101101100111000111000111100010011011011011001001110001110001111000110110110110010011011100011110001110001101101100100110110111100011100011100011011001001101101101111000111000111100010010011011011011001110001110001111000110110110110110010011100011110001110001101101100100100110111000111100011100011011001001101101101111000111000111000110010011011011011001110001110001111000100110110110110010011100011100011110001101101101100100110111000111100011100011011011001001101101111000111000111000110110010011011011011110001110001111000100100110110110110011100011100011110001101101101101100100111000111100011100011011011001001001101110001111000111000110110010011011011011110001110001110001100100110110110110011100011100011110001001101101101100100111000111100111100011011011011001001101110001111000111000110110110010011011011110001110001110001100100110110110110111100011100011110001001101101101101100111000111000111100011011011011001001101110001111000111000110110110010011011011110001110001110001101100100110110110111100011100011110001001001101101101100111000111000111100011011011011011001001110001111000111000110110110010010011011100011110001110001101100100110110110111100011100011100011001001101101101100111000111000111100010011011011011001001110001111001111000110110110110010011011100";
   
  signal r_CLOCK     : std_logic                    := '0';
  signal r_RESET     : std_logic                    := '0';
  signal r_data     : std_logic_vector(31 downto 0)  := (others => '0');

  signal r_SERIAL_CAM    : std_logic := '1';
  signal r_Buffer_index : unsigned(7 downto 0) := (others => '0');
  
  -- this is purely for diagnostics to show where we are up to in the sim
  type T_STATE is
    (CONTINUOUS_ZERO, CONTINUOUS_ONE, SYNC, DATA, RANDOM12BITS, RAWSAMPLES);
  signal STATE : T_STATE;
  
  signal r_RecordingBits : std_logic := '0';
  
procedure GEN_CONTINUOUS_ZERO (
    signal o_serial : out std_logic) is
	begin
 
		o_serial <= '0';
		wait for c_PIXEL_PERIOD * 4;
    
end GEN_CONTINUOUS_ZERO;

procedure GEN_CONTINUOUS_ONE (
    signal o_serial : out std_logic) is
	begin
 
		o_serial <= '1';
		wait for c_PIXEL_PERIOD * 505;
    
end GEN_CONTINUOUS_ONE;

procedure GEN_SYNC_PATTERN (
    signal o_serial : out std_logic) is
  begin
 
    -- Send Data
    for ii in 1 to 2988 loop 	--249*12
		o_serial <= '1';
		wait for c_PULSE_PERIOD;
		o_serial <= '0';
		wait for c_PULSE_PERIOD;
    end loop;  -- ii
    
end GEN_SYNC_PATTERN;

 procedure GEN_BITS (
    i_data_in       : in  std_logic_vector(11 downto 0);
    signal o_serial : out std_logic) is
  begin
  
 
    -- Send Data
    for ii in 0 to 11 loop
      o_serial <= i_data_in(ii) xor '1';
      wait for c_PULSE_PERIOD;
	  o_serial <= i_data_in(ii) xor '0';
      wait for c_PULSE_PERIOD;
    end loop;  -- ii
    
  end GEN_BITS;
   
    procedure USE_RAW_SAMPLES (
    i_data_in       : in  std_logic_vector(2047 downto 0);
    signal o_serial : out std_logic) is
  begin

    -- Send Data
    for ii in 0 to 2046 loop
      o_serial <= i_data_in(2046-ii);
      wait until rising_edge(r_CLOCK);
    end loop;  -- ii
    
  end USE_RAW_SAMPLES;
   
begin
 
  -- Instantiate decoder
  decoder_inst : decoder
    port map (
    i_Clk       => r_CLOCK, -- clock
    i_Rst       => r_RESET,   -- reset
    i_Sample    => r_SERIAL_CAM, -- RX signal to sample
    o_data      => r_data,        -- 32 bits of output data
	o_RecordingBits => r_recordingbits
    --i_Read_Buffer_index => r_Buffer_index
      );

  r_CLOCK <= not r_CLOCK after 3.5 ns;
   
   
  process is
  begin

    r_SERIAL_CAM <= '0';
    r_Buffer_index <= "00000000";
    wait until rising_edge(r_CLOCK);
    r_RESET <= '0';
    wait until rising_edge(r_CLOCK);
    r_RESET <= '1';
	STATE <= CONTINUOUS_ZERO;
	GEN_CONTINUOUS_ZERO(r_SERIAL_CAM);
	STATE <= CONTINUOUS_ONE;
	GEN_CONTINUOUS_ONE(r_SERIAL_CAM);
	STATE <= SYNC;
	GEN_SYNC_PATTERN(r_SERIAL_CAM);
	STATE <= RAWSAMPLES;
	USE_RAW_SAMPLES(c_raw_samples_1,r_SERIAL_CAM);
	
	STATE <= RANDOM12BITS;
	-- REMEMBER LSB is sent first!
    GEN_BITS("000010000011", r_SERIAL_CAM);
	STATE <= DATA;
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101010101010", r_SERIAL_CAM);
	GEN_BITS("111111111111", r_SERIAL_CAM);
	GEN_BITS("101101000001", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	wait until rising_edge(r_CLOCK);
    r_RESET <= '0';
    wait until rising_edge(r_CLOCK);
    r_RESET <= '1';
	
	-- second frame -it will resync
	STATE <= CONTINUOUS_ZERO;
	GEN_CONTINUOUS_ZERO(r_SERIAL_CAM);
	STATE <= CONTINUOUS_ONE;
	GEN_CONTINUOUS_ONE(r_SERIAL_CAM);
	STATE <= SYNC;
	GEN_SYNC_PATTERN(r_SERIAL_CAM);
	STATE <= RANDOM12BITS;
	-- REMEMBER LSB is sent first!
    GEN_BITS("000010000011", r_SERIAL_CAM);
	STATE <= DATA;
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101010101010", r_SERIAL_CAM);
	GEN_BITS("111111111111", r_SERIAL_CAM);
	GEN_BITS("101101000001", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
	GEN_BITS("101111001011", r_SERIAL_CAM);
    -- for i in 1 to 1 loop -- set to 250 for full frame
		-- for j in 1 to 3 loop
			-- GEN_BITS("000000000000", r_SERIAL_CAM);
		-- end loop;
		-- GEN_BITS("111111111110", r_SERIAL_CAM);
		-- for j in 1 to 248 loop
			-- GEN_BITS("100110101010", r_SERIAL_CAM);
		-- end loop;
	-- end loop;
    wait until rising_edge(r_CLOCK);
    
    assert false report "Tests Complete" severity failure;	-- just to end the simulation
     
  end process;
   
end behave;